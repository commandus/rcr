/*
 * SvcImpl.cpp
 */
#include <iostream>
#include <sstream>
#include <ctime>

#include <grpc++/server_context.h>
#include <grpc++/security/credentials.h>
#include <grpc++/security/server_credentials.h>

// JSON
#include <google/protobuf/util/json_util.h>

#include "svcImpl.h"
#include "passphrase.h"
#include "RCQuery.h"
#include "RCQueryProcessor.h"

#include <odb/database.hxx>
#include <odb/transaction.hxx>

#ifdef ENABLE_SQLITE
#include <odb/sqlite/database.hxx>
#endif
#ifdef ENABLE_PG
#include <odb/pgsql/database.hxx>
#endif
#include "gen/rcr.pb-odb.hxx"
#include "gen/odb-views-odb.hxx"
#include "SpreadSheetHelper.h"
#include "BoxName.h"
#include "string-helper.h"

#include <xlnt/workbook/workbook.hpp>

// i18n
#include <libintl.h>
#define _(String) gettext (String)

using grpc::StatusCode;
using odb::query;

const int DEF_LIST_SIZE = 1000;
const std::string ERR_SVC_INVALID_ARGS = _("Invalid arguments");
const std::string ERR_SVC_PERMISSION_DENIED = _("Permission denied");
#define ERR_CODE_SVC_INVALID_ARGS -505

// default list size
#define DEF_LABEL_LIST_SIZE		100
#define MAX_LABEL_LIST_SIZE		10000
#define LOG(x) std::cerr

typedef uint64_t NOTIFICATION_TYPE;

static google::protobuf::util::JsonPrintOptions printJSONOptions;

static odb::database *odbconnect(
    ServiceConfig *config
)
{
#ifdef ENABLE_PG
    return new odb::pgsql::database(
            std::string(config->dbuser),
            std::string(config->dbpassword),
            std::string(config->dbname),
            std::string(config->dbhost),
            config->dbport);
#endif
#ifdef ENABLE_SQLITE
    return new odb::sqlite::database(config->sqliteDbName);
#endif
}

template <class T>
std::unique_ptr<T> RcrImpl::load(
    uint64_t id
)
{
    try
    {
        std::unique_ptr<T> r(mDb->load<T>(id));
        return r;
    }
    catch (const std::exception &e)
    {
        LOG(ERROR) << _("Loading object error, id: ") << id << e.what();
        // return nullptr
        std::unique_ptr<T> r;
        return r;
    }
}

/**
 * gRPC published methods error catch log function
 * Parameters:
 * 	signature	method name
 * 	cause		ODB, stdlib or other exception ane
 * 	what		Error description
 * 	msg			Protobuf incoming message
 */
std::string logString (
    const std::string &signature,
    const std::string &cause,
    const std::string &what,
    const ::google::protobuf::Message *msg)
{
    std::stringstream ss;
    if (!signature.empty())
        ss << signature;

    if (!cause.empty())
        ss << _(" cause(") << cause << ")";
    if (!what.empty())
        ss << _(" what(") << what << ")";

    if (msg) {
        std::string s;
        google::protobuf::util::MessageToJsonString(*msg, &s, printJSONOptions);
        ss << _(" message: ") << s;
    }
    return ss.str();
}

#define CHECK_PERMISSION(db, user, lvl) \
    int rights = checkUserRights(nullptr, db, user); \
    if (rights < lvl) { \
        LOG(ERROR) << ERR_SVC_PERMISSION_DENIED << " id: " << user.id() << " name: " << user.name() \
        << " password: " << user.password() << " token: " << user.token(); \
        return grpc::Status(StatusCode::PERMISSION_DENIED, ERR_SVC_PERMISSION_DENIED); \
    }

#define BEGIN_GRPC_METHOD(signature, requestMessage, transact) \
		odb::transaction transact; \
		try { \
			LOG(INFO) << logString(signature, "", "", requestMessage); \
	        t.reset(mDb->begin());

#define END_GRPC_METHOD(signature, requestMessage, responseMessage, transaction) \
		transaction.commit(); \
        if (responseMessage) LOG(INFO) << logString(signature, "", "", responseMessage); \
	} \
	catch (const odb::exception& oe) { \
		LOG(ERROR) << logString(signature, "odb::exception", oe.what(), requestMessage); \
		transaction.rollback(); \
	} \
	catch (const std::exception& se) { \
		LOG(ERROR) << logString(signature, "std::exception", se.what(), requestMessage); \
		transaction.rollback(); \
	} \
	catch (...) { \
		LOG(ERROR) << logString(signature, "unknown exception", "??", requestMessage); \
		transaction.rollback(); \
	}

#define INIT_CHECK_RANGE(list, r, sz, ofs)  \
	int sz = list.size(); \
	if (sz <= 0) \
		sz = DEF_LIST_SIZE; \
	int ofs = list.start(); \
	if (ofs < 0) \
		ofs = 0; \
	sz += ofs; \
	int r = 0;


#define CHECK_RANGE(r, sz, ofs) \
	if (r >= sz) \
		break; \
	if (r < ofs) \
	{ \
		r++; \
		continue; \
	}

// --------------------- RcrImpl ---------------------

const grpc::string ERR_NO_GRANTS(_("No grants to call"));
const grpc::string ERR_NOT_IMPLEMENTED(_("Not implemented"));

const grpc::Status& RcrImpl::STATUS_NO_GRANTS = grpc::Status(StatusCode::PERMISSION_DENIED, ERR_NO_GRANTS);
const grpc::Status& RcrImpl::STATUS_NOT_IMPLEMENTED = grpc::Status(StatusCode::UNIMPLEMENTED, ERR_NOT_IMPLEMENTED);

uint64_t VERSION_MAJOR = 0x010000;

RcrImpl::RcrImpl(ServiceConfig *config)
{
    srand((unsigned ) time(nullptr));    //
	mConfig = config;
	mDb = odbconnect(config);

    printJSONOptions.add_whitespace = true;
    printJSONOptions.always_print_primitive_fields = true;
    printJSONOptions.preserve_proto_field_names = true;

    int cnt = loginPlugins.load(config->pluginDirPath, &config->pluginOptions);
    LOG(INFO) << _("login plugins loaded: ") << cnt << _(" from directory ") << config->pluginDirPath << std::endl;

}

RcrImpl::~RcrImpl()
{
    delete mDb;
}

ServiceConfig *RcrImpl::getConfig()
{
	return mConfig;
}

::grpc::Status RcrImpl::login(
    ::grpc::ServerContext* context,
    const ::rcr::LoginRequest* request,
    ::rcr::LoginResponse* response
)
{
    BEGIN_GRPC_METHOD("login", request, t)
    if (response == nullptr) {
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    }
    *response->mutable_user() = request->user();
    bool logged = checkCredentialsNSetToken(t, mDb, response->mutable_user());
    if (!logged) {
        if (loginPlugins.login(response->user().name(), response->user().password())) {
            setCredentialsNSetToken(t, mDb, response->mutable_user());
            logged = true;
        }
    }
    if (response->user().token() == 0) {
        // user account is created but user is banned!
        logged = false;
    }
    response->set_success(logged);
    response->set_version(VERSION_MAJOR);
    response->set_version_name(getRandomName());
    END_GRPC_METHOD("login", request, response, t)
    return grpc::Status::OK;
}

grpc::Status RcrImpl::chUser(
    grpc::ServerContext* context,
    const rcr::UserRequest* request,
    rcr::OperationResponse* response
)
{
    if (request == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    if (response == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    BEGIN_GRPC_METHOD("chUser", request, t)
    CHECK_PERMISSION(mDb, request->user(), 1)
        char op;
        if (request->operationsymbol().empty())
            op = 'l';   // '=' ?!!
        else
            op = request->operationsymbol()[0];

        try {
            odb::result<rcr::User> qs;
            if (request->value().id())
                qs = mDb->query<rcr::User>(odb::query<rcr::User>::id == request->value().id());
            else
                qs = mDb->query<rcr::User>(odb::query<rcr::User>::name == request->value().name());

            odb::result<rcr::User>::iterator it(qs.begin());
            switch (op) {
                case '=':
                    if (it == qs.end()) {
                        rcr::User user = request->value();
                        response->set_id(mDb->persist(user));
                    } else {
                        it->set_password(request->value().password());
                        it->set_name(request->value().name());
                        it->set_token(request->value().token());
                        it->set_rights(request->value().rights());
                        mDb->update(*it);
                    }
                    response->set_code(0);
                    break;
                case '+':
                    if (it != qs.end()) {
                        response->set_code(-3);
                        response->set_description(_("User already exists"));
                    } else {
                        rcr::User user = request->value();
                        user.set_token(generateNewToken());
                        // user.set_rights(0);
                        response->set_id(mDb->persist(user));
                        response->set_code(0);
                    }
                    break;
                case '-':
                    if (it == qs.end()) {
                        response->set_code(-3);
                        response->set_description(_("User does not exists"));
                    } else {
                        mDb->erase(*it);
                        response->set_code(0);
                    }
                    break;
                default:
                    response->set_code(-2);
                    response->set_description(_("Invalid operation"));
            }
        } catch (const odb::exception &e) {
            LOG(ERROR) << _("change user error: ") << e.what();
        } catch (...) {
            LOG(ERROR) << _("change user unknown error");
        }

    END_GRPC_METHOD("chUser", request, response, t)
    return true ? grpc::Status::OK : grpc::Status(StatusCode::NOT_FOUND, "");
}

grpc::Status RcrImpl::chPropertyType(
    ::grpc::ServerContext* context,
    const ::rcr::ChPropertyTypeRequest* request,
    ::rcr::OperationResponse* response
)
{
    if (request == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    if (response == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    BEGIN_GRPC_METHOD("chPropertyType", request, t)
    CHECK_PERMISSION(mDb, request->user(), 1)
    char op;
    if (request->operationsymbol().empty())
        op = 'l';   // '=' ?!!
    else
        op = request->operationsymbol()[0];

    try {
        odb::result<rcr::PropertyType> qs;
        if (request->value().id())
            qs = mDb->query<rcr::PropertyType>(odb::query<rcr::PropertyType>::id == request->value().id());
        else
            qs = mDb->query<rcr::PropertyType>(odb::query<rcr::PropertyType>::key == request->value().key());

        odb::result<rcr::PropertyType>::iterator it(qs.begin());
        switch (op) {
            case '=':
                if (it == qs.end()) {
                    rcr::PropertyType pt = request->value();
                    response->set_id(mDb->persist(pt));
                } else {
                    it->set_key(request->value().key());
                    it->set_description(request->value().description());
                    mDb->update(*it);
                }
                response->set_code(0);
                break;
            case '+':
                if (it != qs.end()) {
                    response->set_code(-3);
                    response->set_description(_("Property already exists"));
                } else {
                    rcr::PropertyType pt = request->value();
                    response->set_id(mDb->persist(pt));
                    response->set_code(0);
                }
                break;
            case '-':
                if (it == qs.end()) {
                    response->set_code(-3);
                    response->set_description(_("Property does not exists"));
                } else {
                    removePropertyFromCards(mDb, t, it->id());
                    mDb->erase(*it);
                    response->set_code(0);
                }
                break;
            default:
                response->set_code(-2);
                response->set_description(_("Invalid operation"));
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << _("change property error: ") << e.what();
    } catch (...) {
        LOG(ERROR) << _("change property unknown error");
    }

    END_GRPC_METHOD("chPropertyType", request, response, t)
    return true ? grpc::Status::OK : grpc::Status(StatusCode::NOT_FOUND, "");
}

grpc::Status RcrImpl::getCard(
    grpc::ServerContext* context,
    const rcr::GetItemRequest* request,
    rcr::CardNPropetiesPackages* response
)
{
    if (request == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    if (response == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    if (request->id() == 0)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    BEGIN_GRPC_METHOD("getCard", request, t)
    rcr::DictionariesResponse dictionaries;
    loadDictionaries(&dictionaries, ML_INTL);

    try {
        odb::result<rcr::Card> qc = mDb->query<rcr::Card>(odb::query<rcr::Card>::id == request->id());
        odb::result<rcr::Card>::iterator it(qc.begin());
        if (it == qc.end())
            return grpc::Status::OK;
        *response->mutable_card() = *it;
        // properties
        odb::result<rcr::Property> qp = mDb->query<rcr::Property>(odb::query<rcr::Property>::card_id == request->id());
        for (odb::result<rcr::Property>::iterator itp(qp.begin()); itp != qp.end(); itp++) {
            rcr::PropertyWithName *e = response->mutable_properties()->Add();
            e->set_id(itp->id());
            const std::string &s = RCQueryProcessor::findPropertyTypeName(&dictionaries, itp->property_type_id());
            if (s.empty())
                continue;
            e->set_property_type(s);
            e->set_value(itp->value());
        }
        // packages
        odb::result<rcr::Package> qb = mDb->query<rcr::Package>(odb::query<rcr::Package>::card_id == request->id());
        for (odb::result<rcr::Package>::iterator itb(qb.begin()); itb != qb.end(); itb++) {
            rcr::Package *p = response->mutable_packages()->Add();
            *p = *itb;
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << _("get card error: ") << e.what();
    } catch (...) {
        LOG(ERROR) << _("get card unknown error");
    }
    END_GRPC_METHOD("getCard", request, response, t)
    return grpc::Status::OK;
}

grpc::Status RcrImpl::chCard(
    grpc::ServerContext* context,
    const rcr::ChCardRequest* request,
    rcr::OperationResponse* response
)
{
    if (request == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    if (response == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    int r = 0;
    BEGIN_GRPC_METHOD("chCard", request, t)
    CHECK_PERMISSION(mDb, request->user(), 1)
    char op = 'L';
    if (!request->operationsymbol().empty()) {
        op = request->operationsymbol()[0];
    }
    try {
        switch (op) {
            case '+': {
                rcr::Card v = request->value();
                if (v.id())
                    v.clear_id();
                v.set_uname(toUpperCase(v.name()));

                // check does card exists
                rcr::DictionariesResponse dictionaries;
                r = loadDictionaries(&dictionaries, ML_INTL);

                uint64_t card_id = RCQueryProcessor::findCardByNameNominalProperties(
                    request->value().name(),
                    request->value().symbol_id(),
                    request->value().nominal(),
                    request->properties(),
                    mDb,
                    &dictionaries
                );

                if (card_id == 0) {
                    // a new one
                    card_id = mDb->persist(v);

                    for (auto pr = request->properties().begin(); pr != request->properties().end(); pr++) {
                        rcr::Property prv = *pr;
                        prv.set_card_id(card_id);
                        mDb->persist(prv);
                    }
                    for (auto pack = request->packages().begin(); pack != request->packages().end(); pack++) {
                        rcr::Package p = *pack;
                        p.set_card_id(card_id);
                        mDb->persist(p);
                    }
                } else {
                    // card exists
                    // just add a new package
                    addPackages(card_id, request->packages(), request->user(), mDb);
                }
                response->set_id(card_id);
                response->set_code(0);
                }
                break;
            case '-':
                if (request->package_id()) {
                    // remove only package not a card
                    if (removePackage(mDb, t, request))
                        removeCard(mDb, t, request);
                } else
                    removeCard(mDb, t, request);
                response->set_code(0);
                break;
            case '=': {
                rcr::Card v = request->value();
                uint64_t id = v.id();
                if (!id)
                    return grpc::Status(StatusCode::INVALID_ARGUMENT, "");
                v.set_uname(toUpperCase(v.name()));
                mDb->update(v);
                // update property if changed
                odb::result<rcr::Property> q(mDb->query<rcr::Property>(odb::query<rcr::Property>::card_id == request->value().id()));
                std::vector <uint64_t> updatedPropertyIds;
                for (odb::result<rcr::Property>::iterator it(q.begin()); it != q.end(); it++) {
                    auto qit = std::find_if(request->properties().begin(), request->properties().end(),
                        [it] (auto v) {
                        return it->id() == v.id();
                    });
                    if (qit == request->properties().end()) {
                        // delete from database, it removed
                        mDb->erase(*it);
                    } else {
                        updatedPropertyIds.push_back(qit->card_id());
                        // update if it changed
                        if (!(it->card_id() == qit->card_id()
                            && it->property_type_id() == qit->property_type_id()
                            && it->value() == qit->value())) {
                            // update
                            mDb->update(*qit);
                        }
                    }
                }
                // insert a new ones
                for (auto it = request->properties().begin(); it != request->properties().end(); it++) {
                    auto alreadyIt = std::find_if(updatedPropertyIds.begin(), updatedPropertyIds.end(),
                        [it] (auto v) {
                            return it->id() == v;
                        });
                    if (alreadyIt == updatedPropertyIds.end()) {
                        // not updatedPropertyIds yet, insert a new one
                        rcr::Property p = *it;
                        uint64_t pid = mDb->persist(p);
                    }
                }

                // update packages if changed or delete if no more exists
                if (request->package_id())
                    updateCardPackage(mDb, t, request, request->package_id());  // only selected
                else
                    updateCardPackages(mDb, t, request);    // all
                response->set_id(id);
                response->set_code(0);
            }
            break;
        default:
            response->set_code(-4);
            response->set_description(_("Invalid operation"));

        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << _("chCard error: ") << e.what();
    } catch (...) {
        LOG(ERROR) << _("chCard unknown error");
    }
    END_GRPC_METHOD("chCard", request, response, t)
    return r == 0 ? grpc::Status::OK : grpc::Status(StatusCode::INTERNAL, "");
}

grpc::Status RcrImpl::cardQuery(
    ::grpc::ServerContext* context,
    const ::rcr::CardQueryRequest* request,
    ::rcr::CardQueryResponse* response
)
{
    if (request == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    if (response == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    int r = 0;
    BEGIN_GRPC_METHOD("cardQuery", request, t)
    uint64_t userId;
    int rights = checkUserRights(&userId, mDb, request->user());
    rcr::DictionariesResponse dictionaries;
    r = loadDictionaries(&dictionaries, ML_INTL);
    if (!r) {
        const rcr::Symbol *measureSym = RCQueryProcessor::findSymbol(&dictionaries, request->measure_symbol());
        uint32_t componentFlags;
        if (measureSym)
            componentFlags = FLAG_COMPONENT(measureSym->id() - 1);
        else
            componentFlags = FLAG_ALL_COMPONENTS;

        size_t position = 0;
        RCQuery q;
        int r = q.parse(ML_RU, request->query(), position, firstComponentInFlags(componentFlags));
        if (!r) {
#if CMAKE_BUILD_TYPE == Debug
            // LOG(INFO) << "card query: " << q.toString() << std::endl;
#endif
            RCQueryProcessor p(q);
            rcr::CardQueryResponse qr;
            uint64_t cnt = 0;
            uint64_t sum = 0;
            p.exec(mDb, &t, userId, rights, &dictionaries, request->list(),
               response->mutable_rslt(), response->mutable_cards(),
               componentFlags,
               cnt, sum
            );
            response->mutable_rslt()->set_count(cnt);
            response->mutable_rslt()->set_sum(sum);
        } else {
            response->mutable_rslt()->set_code(r);
        }
    }
    END_GRPC_METHOD("cardQuery", request, response, t)
    return r == 0 ? grpc::Status::OK : grpc::Status(StatusCode::INTERNAL, "");
}

::grpc::Status RcrImpl::getDictionaries(
    ::grpc::ServerContext* context,
    const ::rcr::DictionariesRequest* request,
    ::rcr::DictionariesResponse* response
)
{
    if (request == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    if (response == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    int r;
    BEGIN_GRPC_METHOD("getDictionaries", request, t)
    r = loadDictionaries(response, (MEASURE_LOCALE) request->locale_id());
    END_GRPC_METHOD("getDictionaries", request, response, t)
    return ((r == 0) ? grpc::Status::OK : grpc::Status(StatusCode::UNKNOWN, ""));
}

grpc::Status RcrImpl::cardPush(
    grpc::ServerContext* context,
    grpc::ServerReader<::rcr::CardRequest>* reader,
    rcr::OperationResponse* response
)
{
    if (reader == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    if (response == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    int r;
    BEGIN_GRPC_METHOD("cardPush", response, t)
    rcr::DictionariesResponse dictionaries;
    r = loadDictionaries(&dictionaries, ML_INTL);
    rcr::CardRequest cardRequest;
    while (reader->Read(&cardRequest)) {
        uint64_t userId;
        int rights = checkUserRights(&userId, mDb, cardRequest.user());
	    if (rights < 0)
            break;
        RCQueryProcessor p;
        r = p.saveCard(mDb, &t, userId, cardRequest, &dictionaries);
        if (r)
            break;
    }
    response->set_id(0);
    response->set_code(r);
    response->set_description("");
    END_GRPC_METHOD("cardPush", response, response, t)
    return (r == 0) ? grpc::Status::OK : grpc::Status(StatusCode::UNKNOWN, "Save card error " + std::to_string(r));
}

/**
 * Load dictionaries. Return 0- success
 * @param retVal
 * @param locale
 * @return 0- success
 */
int RcrImpl::loadDictionaries(
    rcr::DictionariesResponse *retVal,
    MEASURE_LOCALE locale
) {
    try {
        odb::result<rcr::Operation> qs(mDb->query<rcr::Operation>(odb::query<rcr::Operation>::id != 0));
        for (odb::result<rcr::Operation>::iterator i(qs.begin()); i != qs.end(); i++) {
            rcr::Operation *op = retVal->add_operation();
            op->CopyFrom(*i);
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << _("list operations error: ") << e.what();
    } catch (...) {
        LOG(ERROR) << _("list operations unknown error");
    }

    try {
        odb::result<rcr::Symbol> qs(mDb->query<rcr::Symbol>(odb::query<rcr::Symbol>::id != 0));
        for (odb::result<rcr::Symbol>::iterator i(qs.begin()); i != qs.end(); i++) {
            rcr::Symbol *sym = retVal->add_symbol();
            sym->CopyFrom(*i);
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << _("list symbols error: ") << e.what();
    } catch (...) {
        LOG(ERROR) << _("list symbols unknown error");
    }

    try {
        odb::result<rcr::PropertyType> qs(mDb->query<rcr::PropertyType>(odb::query<rcr::PropertyType>::id != 0));
        for (odb::result<rcr::PropertyType>::iterator i(qs.begin()); i != qs.end(); i++) {
            rcr::PropertyType *op = retVal->add_property_type();
            op->CopyFrom(*i);
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << _("list property error: ") << e.what();
    } catch (...) {
        LOG(ERROR) << _("list property unknown error");
    }
    return 0;
}

grpc::Status RcrImpl::getBox(
    grpc::ServerContext* context,
    const rcr::BoxRequest* request,
    rcr::BoxResponse* response
) {
    if (request == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    if (response == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    int r = 0;
    BEGIN_GRPC_METHOD("getBox", request, t)

    uint64_t startBox = request->start();
    int dp;
    uint64_t finishBox = StockOperation::maxBox(request->start(), dp);
#ifdef ENABLE_SQLITE
    // Sqlite support int64 not uint64
    if (finishBox == std::numeric_limits<uint64_t>::max())
        finishBox = std::numeric_limits<int64_t>::max();
#endif
    int depth = request->depth();
    if (depth <= 0)
        depth = 1;
    if (depth > 4)
        depth = 4;
    size_t offset = request->list().offset();
    size_t size = request->list().size();
    if (size == 0)
        size = DEF_LIST_SIZE;
    size_t cnt = 0;
    size_t sz = 0;

    try {
        odb::result<rcr::BoxCount> qc(mDb->query<rcr::BoxCount>(true));
        odb::result<rcr::BoxCount>::iterator itc(qc.begin());
        size_t cnt = 0;
        size_t totalCount = 0;
        if (itc != qc.end())
            totalCount = itc->count;
        response->mutable_rslt()->set_count(totalCount);

        odb::result<rcr::Box> qs(mDb->query<rcr::Box>(
            odb::query<rcr::Box>::box_id >= startBox
            &&
            odb::query<rcr::Box>::box_id <= finishBox
        ));
        for (odb::result<rcr::Box>::iterator i(qs.begin()); i != qs.end(); i++) {
            cnt++;
            if (cnt - 1 < offset)
                continue;
            sz++;
            if (sz > size)
                break;
            rcr::Box *op = response->add_box();
            op->CopyFrom(*i);
        }
    } catch (const odb::exception &e) {
        r = -1;
        LOG(ERROR) << _("list box error: ") << e.what();
    } catch (...) {
        r = -2;
        LOG(ERROR) << _("list box unknown error");
    }
    END_GRPC_METHOD("getBox", response, response, t)
    return (r == 0) ? grpc::Status::OK : grpc::Status(StatusCode::UNKNOWN, "");
}


grpc::Status RcrImpl::importExcel(
    grpc::ServerContext* context,
    const rcr::ImportExcelRequest* request,
    rcr::OperationResponse* response
)
{
    if (request == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    if (response == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    int r = 0;
    BEGIN_GRPC_METHOD("importExcel", request, t)
    CHECK_PERMISSION(mDb, request->user(), 1)
    rcr::DictionariesResponse dictionaries;
    loadDictionaries(&dictionaries, ML_INTL);

    size_t cnt = 0;
    size_t r = 0;
    for (auto f = request->file().begin(); f != request->file().end(); f++) {
        r += importExcelFile(t, mDb, request->user().id(), request->symbol(), *f, request->prefix_box(),
            request->number_in_filename(), request->operation(), dictionaries);
        cnt++;
    }
    response->set_count(cnt);   // files
    response->set_sum(r);     // entries
    END_GRPC_METHOD("importExcel", response, response, t)
    return (r == 0) ? grpc::Status::OK : grpc::Status(StatusCode::UNKNOWN, "");
}

grpc::Status RcrImpl::exportExcel(
    grpc::ServerContext* context,
    const rcr::ExportExcelRequest* request,
    rcr::ExportExcelResponse* response
)
{
    if (request == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    if (response == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    int r = 0;
    BEGIN_GRPC_METHOD("exportExcel", request, t)
    CHECK_PERMISSION(mDb, request->user(), 1)
    exportExcelFiles(response, t, mDb, request->symbol_name(), request->query());
    END_GRPC_METHOD("exportExcel", response, response, t)
    return (r == 0) ? grpc::Status::OK : grpc::Status(StatusCode::UNKNOWN, "");
}

int RcrImpl::exportExcelFiles(
    rcr::ExportExcelResponse *retVal,
    odb::transaction &t,
    odb::database *db,
    const std::string &symbolName,
    const std::string &query
)
{
    rcr::DictionariesResponse dictionaries;
    int r = loadDictionaries(&dictionaries, ML_INTL);
    if (!r) {
        time_t ct = time(nullptr);
        r = exportExcelFile(retVal, t, mDb, query, symbolName, ct, &dictionaries);
    }
    return r;
}

size_t RcrImpl::importExcelFile(
    odb::transaction &t,
    odb::database *db,
    uint64_t userId,
    const std::string &symbol,
    const rcr::ExcelFile &file,
    uint64_t prefixBox,
    bool numberInFilename,
    const std::string &operationSymbol,
    rcr::DictionariesResponse &dictionaries
) {
    std::string pb;
    uint64_t box;
    if (numberInFilename) {
        if (prefixBox)
            pb = StockOperation::boxes2string(prefixBox) + " ";
        box = BoxName::extractFromFileName(pb + file.name()); //  <- add if filename contains boxes
    } else {
        box = prefixBox;
    }
    SpreadSheetHelper spreadSheet(file.name(), file.content(), symbol);

    // LOG(INFO) << "user id: " << userId << std::endl;
    // LOG(INFO) << spreadSheet.toJsonString();

    for (auto item = spreadSheet.items.begin(); item != spreadSheet.items.end(); item++) {
        rcr::CardRequest cardRequest;
        item->toCardRequest(operationSymbol, symbol, box, cardRequest);
        RCQueryProcessor p;
        p.saveCard(mDb, &t, userId, cardRequest, &dictionaries);
    }
    return spreadSheet.total;   // total count of items
}

int RcrImpl::checkUserRights(
    uint64_t *retUserId,
    odb::database *db,
    const rcr::User &user
) {
    try {
        odb::result<rcr::User> qs(mDb->query<rcr::User>(
            odb::query<rcr::User>::name == user.name()
            &&
            odb::query<rcr::User>::password == user.password()
        ));
        odb::result<rcr::User>::iterator it(qs.begin());
        if (it != qs.end()) {
            if (retUserId)
                *retUserId = it->id();
            return it->rights();
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << _("Check credentials error: ") << e.what();
    } catch (...) {
        LOG(ERROR) << _("Check credentials unknown error");
    }
    return -1;
}

bool RcrImpl::loadUser(
    rcr::User *retVal,
    odb::database *db,
    uint64_t id
) {
    odb::result<rcr::User> qu(mDb->query<rcr::User>(odb::query<rcr::User>::id == id));
    odb::result<rcr::User>::iterator itu(qu.begin());
    if (itu != qu.end()) {
        if (retVal)
            *retVal = *itu;
        return true;
    } else
        return false;
}

bool RcrImpl::setCredentialsNSetToken(
    odb::transaction &t,
    odb::database *db,
    rcr::User *retVal
) {
    if (!retVal)
        return false;
    try {
        retVal->set_token(generateNewToken());
        odb::result<rcr::User> qu(mDb->query<rcr::User>(odb::query<rcr::User>::name == retVal->name()));
        odb::result<rcr::User>::iterator itu(qu.begin());
        if (itu != qu.end()) {
            db->update(*retVal);
        } else {
            db->persist(*retVal);   // save token
        }
        return true;
    } catch (const odb::exception &e) {
        LOG(ERROR) << _("Set credentials error: ") << e.what();
    } catch (...) {
        LOG(ERROR) << _("Set credentials unknown error");
    }
    return false;
}

bool RcrImpl::checkCredentialsNSetToken(
    odb::transaction &t,
    odb::database *db,
    rcr::User *retVal
) {
    if (!retVal)
        return false;
    try {
        odb::result<rcr::User> qs(mDb->query<rcr::User>(odb::query<rcr::User>::name == retVal->name()
        &&
        odb::query<rcr::User>::password == retVal->password()
        ));
        odb::result<rcr::User>::iterator it(qs.begin());
        if (it == qs.end())
            return false;
        *retVal = *it;
        retVal->set_token(generateNewToken());
        db->update(*retVal);   // save token
        return true;
    } catch (const odb::exception &e) {
        LOG(ERROR) << _("Check credentials & set token error: ") << e.what();
    } catch (...) {
        LOG(ERROR) << _("Check credentials & set token unknown error");
    }
    return false;
}

/**
 * token is not 0.
 * If token = 0- it ius banned user
 * @return
 */
uint64_t RcrImpl::generateNewToken() {
    uint64_t r = 0;
    while (r == 0) {
        r = rand();
    }
    return r;
}

grpc::Status RcrImpl::lsUser(
    grpc::ServerContext* context,
    const rcr::UserRequest* request,
    rcr::UserResponse* response
)
{
    int r = 0;
    BEGIN_GRPC_METHOD("lsUser", request, t)
    // std::cerr << pb2JsonString(params->user()) << std::endl;
    rcr::User u;
    int rights = checkUserRights(nullptr, mDb, request->user());
    try {
        odb::result<rcr::User> qs(mDb->query<rcr::User>(
            odb::query<rcr::User>::id != 0
        ));
        size_t c = 0;
        size_t sz = 0;
        size_t size = request->list().size();
        if (size == 0 || size > 10000) {
            size = DEF_LIST_SIZE;
        }
        for (odb::result<rcr::User>::iterator it(qs.begin()); it != qs.end(); it++) {
            if (c < request->list().offset()) {
                c++;
                continue;
            }
            if (sz >= size)
                break;
            sz++;
            u = *it;
            if (rights != 1) {
                u.set_token(0);
                u.set_password("");
            }
            rcr::User *ru = response->add_user();
            *ru = u;
        }
    } catch (const odb::exception &e) {
        r = 1;
        LOG(ERROR) << _("list user error: ") << e.what();
    } catch (...) {
        r = 2;
        LOG(ERROR) << _("list user unknown error");
    }

    END_GRPC_METHOD("lsUser", request, &u, t)
    return (r == 0) ? grpc::Status::OK : grpc::Status(StatusCode::UNKNOWN, "");
}

void RcrImpl::removePropertyFromCards(
    odb::database *db,
    odb::transaction &t,
    uint64_t property_type_id
) {
    try {
        odb::result<rcr::Property> qs(mDb->query<rcr::Property>(odb::query<rcr::Property>::property_type_id == property_type_id));
        for (odb::result<rcr::Property>::iterator it(qs.begin()); it != qs.end(); it++) {
            db->erase(*it);
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << _("remove property error: ") << e.what();
    } catch (...) {
        LOG(ERROR) << _("remove property unknown error");
    }
}

grpc::Status RcrImpl::chBox(
    grpc::ServerContext* context,
    const rcr::ChBoxRequest* request,
    rcr::OperationResponse* response
)
{
    if (request == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    if (response == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    BEGIN_GRPC_METHOD("chBox", request, t)
    CHECK_PERMISSION(mDb, request->user(), 1)
        char op;
        if (request->operationsymbol().empty())
            op = 'l';
        else
            op = request->operationsymbol()[0];

        uint64_t startBox = request->value().box_id();
        int depth;
        uint64_t finishBox = StockOperation::maxBox(startBox, depth);
        try {
            odb::result<rcr::Box> qs;
            qs = mDb->query<rcr::Box>(odb::query<rcr::Box>::box_id >= startBox
                    &&
                odb::query<rcr::Box>::box_id <= finishBox
            );

            odb::result<rcr::Box>::iterator it(qs.begin());
            switch (op) {
                case '/':   // rename (move) boxes
                {
                    uint64_t destRoot = request->value().id();  // sorry!
                    for (; it != qs.end(); it++) {
                        uint64_t srcBox = it->box_id();
                        uint64_t dstBox = StockOperation::renameBox(srcBox, startBox, destRoot);

                        // check does destination exists
                        odb::result<rcr::Box> qTarget = mDb->query<rcr::Box>(
                                odb::query<rcr::Box>::box_id == dstBox
                        );
                        odb::result<rcr::Box>::iterator itTarget(qTarget.begin());
                        // if not, create a new one
                        if (itTarget == qTarget.end()) {
                            rcr::Box b;
                            b.set_box_id(dstBox);
                            b.set_name(request->value().name());
                            mDb->persist(b);
                        }

                        // move to a new box
                        changePackageBox(mDb, t, srcBox, dstBox);

                        // remove old
                        mDb->erase(*it);
                    }
                    response->set_code(0);
                }
                    break;
                case '=':
                    if (it == qs.end()) {
                        // no box exist. create a new one
                        rcr::Box box = request->value();
                        box.set_uname(toUpperCase(request->value().name()));
                        response->set_id(mDb->persist(box));
                    } else {
                        // box exists
                        for (; it != qs.end(); it++) {
                            it->set_name(request->value().name());
                            // uppercase to search
                            it->set_uname(toUpperCase(request->value().name()));
                            mDb->update(*it);
                        }
                    }
                    response->set_code(0);
                    break;
                case '+':
                    if (it != qs.end()) {
                        // box exist already, nothing to do
                        response->set_code(-3);
                        response->set_description(_("Box already exists"));
                    } else {
                        // no box exist, add
                        rcr::Box box = request->value();
                        box.set_uname(toUpperCase(request->value().name()));
                        response->set_id(mDb->persist(box));
                        response->set_code(0);
                    }
                    break;
                case '-':
                    if (it == qs.end()) {
                        // no box exist, report error
                        response->set_code(-3);
                        response->set_description(_("Box does not exists"));
                    } else {
                        removePackagesFromBox(mDb, t, startBox);
                        for (; it != qs.end(); it++) {
                            mDb->erase(*it);
                        }
                        response->set_code(0);
                    }
                    break;
                default:
                    response->set_code(-2);
                    response->set_description(_("Invalid operation"));
            }
        } catch (const odb::exception &e) {
            LOG(ERROR) << _("change box error: ") << e.what();
        } catch (...) {
            LOG(ERROR) << _("change box unknown error");
        }
    END_GRPC_METHOD("chBox", request, response, t)
    return true ? grpc::Status::OK : grpc::Status(StatusCode::NOT_FOUND, "");
}

int RcrImpl::removePackagesFromBox(
    odb::database *db,
    odb::transaction &t,
    uint64_t startBox
) {
    int depth;
    int r = 0;
    uint64_t finishBox = StockOperation::maxBox(startBox, depth);
#ifdef ENABLE_SQLITE
    // Sqlite support int64 not uint64
    if (finishBox == std::numeric_limits<uint64_t>::max())
        finishBox = std::numeric_limits<int64_t>::max();
#endif

    try {
        odb::result<rcr::Package> qs(mDb->query<rcr::Package>(
                odb::query<rcr::Package>::box >= startBox
                &&
                odb::query<rcr::Package>::box <= finishBox
        ));
        for (odb::result<rcr::Package>::iterator it(qs.begin()); it != qs.end(); it++) {
            db->erase(*it);
        }
    } catch (const odb::exception &e) {
        r = -1;
        LOG(ERROR) << _("list box error: ") << e.what();
    } catch (...) {
        r = -2;
        LOG(ERROR) << _("list box unknown error");
    }
    return r;
}

int RcrImpl::changePackageBox(
    odb::database *db,
    odb::transaction &t,
    uint64_t oldBox,
    uint64_t newBox
) {
    int r = 0;
    try {
        odb::result<rcr::Package> qs(mDb->query<rcr::Package>(
            odb::query<rcr::Package>::box == oldBox
        ));
        LOG(INFO) << "Move package "
                  << StockOperation::boxes2string(oldBox) << " to "
                  << StockOperation::boxes2string(newBox) << std::endl;
        for (odb::result<rcr::Package>::iterator it(qs.begin()); it != qs.end(); it++) {
            LOG(INFO) << "=== " << it->id() << " card " << it->card_id() << std::endl;
            it->set_box(newBox);
            db->update(*it);
        }
    } catch (const odb::exception &e) {
        r = -1;
        LOG(ERROR) << _("change package box error: ") << e.what();
    } catch (...) {
        r = -2;
        LOG(ERROR) << _("change package box unknown error");
    }
    return r;
}

void RcrImpl::updateCardPackages(
    odb::database *db,
    odb::transaction &t,
    const rcr::ChCardRequest *request
) {
    odb::result<rcr::Package> qPackage(mDb->query<rcr::Package>(odb::query<rcr::Package>::card_id == request->value().id()));
    std::vector <uint64_t> updatedPackageIds;
    updatedPackageIds.reserve(8);
    for (odb::result<rcr::Package>::iterator it(qPackage.begin()); it != qPackage.end(); it++) {
        auto qit = std::find_if(request->packages().begin(), request->packages().end(),
                                [it] (auto v) {
                                    return it->id() == v.id();
                                });
        if (qit == request->packages().end()) {
            // delete from database, because it removed
            mDb->erase(*it);
        } else {
            updatedPackageIds.push_back(qit->id());
            // remove if qty = 0
            if (qit->qty() == 0) {
                mDb->erase(*qit);
            } else {
                // update if it changed
                if (!(it->card_id() == qit->card_id()
                      && it->box() == qit->box()
                      && it->qty() == qit->qty()
                      && it->box_name() == qit->box_name())) {
                    // update
                    mDb->update(*qit);
                }
            }
        }
    }
    // insert a new ones
    for (auto it = request->packages().begin(); it != request->packages().end(); it++) {
        auto alreadyIt = std::find_if(updatedPackageIds.begin(), updatedPackageIds.end(),
                                      [it] (auto v) {
                                          return it->id() == v;
                                      });
        if (alreadyIt == updatedPackageIds.end()) {
            // not updatedPackageIds yet, insert a new one if qty is not zero
            rcr::Package p = *it;
            if (p.qty() != 0) {
                uint64_t pid = mDb->persist(p);
            }
        }
    }

    /*
    for (auto pack = request->packages().begin(); pack != request->packages().end(); pack++) {
        rcr::Package p = *pack;
        p.set_card_id(request->value().id());
        if (p.id())
            mDb->update(p);
        else
            mDb->persist(p);
    }
     */
}

void RcrImpl::updateCardPackage(
    odb::database *db,
    odb::transaction &t,
    const rcr::ChCardRequest *request,
    uint64_t packageId
) {
    if (request->packages_size() == 0)
        return; // nothing to update.
    // find out old one
    odb::result<rcr::Package> qPackage(mDb->query<rcr::Package>(
            odb::query<rcr::Package>::card_id == request->value().id()
            &&
            odb::query<rcr::Package>::id == packageId
            ));
    // trying to update
    odb::result<rcr::Package>::iterator it(qPackage.begin());
    if (it != qPackage.end()) {
        *it = *request->packages().begin(); // just one package
        it->set_card_id(request->value().id());
        mDb->update(*it);
        return;
    }

    // insert if it does not exists yet
    auto p1(request->packages().begin());
    rcr::Package p = *p1;
    p.set_card_id(request->value().id());
    uint64_t pid = mDb->persist(p);
}

bool RcrImpl::loadPackage(
    rcr::Package *retval,
    odb::database *db,
    uint64_t id
)
{
    odb::result<rcr::Package> qp(mDb->query<rcr::Package>(odb::query<rcr::Package>::id == id));
    odb::result<rcr::Package>::iterator itp(qp.begin());
    if (itp != qp.end()) {
        if (retval)
            *retval = *itp;
        return true;
    } else
        return false;
}

bool RcrImpl::loadCard(
    rcr::Card *retval,
    odb::database *db,
    uint64_t id
)
{
    odb::result<rcr::Card> qp(mDb->query<rcr::Card>(odb::query<rcr::Card>::id == id));
    odb::result<rcr::Card>::iterator itp(qp.begin());
    if (itp != qp.end()) {
        if (retval)
            *retval = *itp;
        return true;
    } else
        return false;
}

bool RcrImpl::removeCard(
    odb::database *db,
    odb::transaction &t,
    const rcr::ChCardRequest *request
) {
    // remove if exists
    if (request->value().id() == 0)
        return false;  // nothing to delete
    // cascade delete may not work
    odb::result<rcr::Property> q(mDb->query<rcr::Property>(odb::query<rcr::Property>::card_id == request->value().id()));
    for (odb::result<rcr::Property>::iterator it(q.begin()); it != q.end(); it++) {
        mDb->erase(*it);
    }
    odb::result<rcr::Package> qp(mDb->query<rcr::Package>(odb::query<rcr::Package>::card_id == request->value().id()));
    for (odb::result<rcr::Package>::iterator itp(qp.begin()); itp != qp.end(); itp++) {
        mDb->erase(*itp);
    }
    mDb->erase(request->value());
    return true;
}

/**
 * Remove package not a card.
 * Return true if all packages has been deleted and card may to delete
 * @param db
 * @param t
 * @param request
 * @return
 */
bool RcrImpl::removePackage(
    odb::database *db,
    odb::transaction &t,
    const rcr::ChCardRequest *request
) {
    odb::result<rcr::Package> qp(mDb->query<rcr::Package>(
        odb::query<rcr::Package>::card_id == request->value().id()
        &&
        odb::query<rcr::Package>::id == request->package_id()
    ));
    for (odb::result<rcr::Package>::iterator itp(qp.begin()); itp != qp.end(); itp++) {
        mDb->erase(*itp);
    }
    // check does card has any packages
    odb::result<rcr::Package> qExists(mDb->query<rcr::Package>(
        odb::query<rcr::Package>::card_id == request->value().id()
    ));
    odb::result<rcr::Package>::iterator itExists(qExists.begin());
    return (itExists == qExists.end());
}

grpc::Status RcrImpl::lsJournal(
    grpc::ServerContext* context,
    const rcr::JournalRequest* request,
    rcr::JournalResponse* response
)
{
    if (request == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    if (response == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    bool r;
    BEGIN_GRPC_METHOD("lsJournal", request, t)
    CHECK_PERMISSION(mDb, request->user(), 1)
    rcr::DictionariesResponse dictionaries;
    loadDictionaries(&dictionaries, ML_INTL);
    try {
        odb::result<rcr::JournalCount> qc(mDb->query<rcr::JournalCount>(true));
        odb::result<rcr::JournalCount>::iterator itc(qc.begin());
        size_t cnt = 0;
        if (itc != qc.end())
            cnt = itc->count;

        response->mutable_rslt()->set_count(cnt);

        odb::result<rcr::JournalQuery> qs(mDb->query<rcr::JournalQuery>(true));
        size_t c = 0;
        size_t sz = 0;
        size_t size = request->list().size();
        if (size == 0 || size > 10000) {
            size = DEF_LIST_SIZE;
        }
        for (odb::result<rcr::JournalQuery>::iterator it(qs.begin()); it != qs.end(); it++) {
            if (c < request->list().offset()) {
                c++;
                continue;
            }
            if (sz >= size)
                break;
            sz++;
            rcr::Log *l = response->add_log() ;
            l->set_id(it->id);
            l->set_dt(it->dt);
            l->set_value(it->value);

            // load user
            if (loadUser(l->mutable_user(), mDb, it->user_id)) {
                l->mutable_user()->clear_password();
                l->mutable_user()->clear_rights();
                l->mutable_user()->clear_token();
            } else
                l->mutable_user()->set_id(it->user_id);

            // load package
            if (loadPackage(l->mutable_package(), mDb, it->package_id)) {
                l->mutable_package()->clear_box_name();
            } else
                l->mutable_package()->set_id(it->package_id);

            // load card
            if (loadCard(l->mutable_card(), mDb, l->mutable_package()->card_id())) {
                l->mutable_card()->clear_uname();
            } else
                l->mutable_card()->set_id(l->mutable_package()->card_id());

            // set operation
            l->mutable_operation()->set_symbol(it->operation_symbol);
        }
        r = true;
    } catch (const odb::exception &e) {
        r = false;
        LOG(ERROR) << _("list journal error: ") << e.what();
    } catch (...) {
        r = false;
        LOG(ERROR) << _("list journal unknown error");
    }

    END_GRPC_METHOD("lsJournal", request, response, t)
    return r ? grpc::Status::OK : grpc::Status(StatusCode::NOT_FOUND, "");
}

int RcrImpl::exportExcelFile(
    rcr::ExportExcelResponse *retVal,
    odb::transaction &t,
    odb::database *db,
    const std::string &query,
    const std::string &symbolName,
    time_t timeStamp,
    rcr::DictionariesResponse *dictionaries
) {
    const rcr::Symbol *measureSym = RCQueryProcessor::findSymbol(dictionaries, symbolName);
    uint32_t componentFlags;
    if (measureSym)
        componentFlags = FLAG_COMPONENT(measureSym->id() - 1);
    else
        componentFlags = FLAG_ALL_COMPONENTS;

    size_t position = 0;
    RCQuery q;
    COMPONENT c = firstComponentInFlags(componentFlags);
    int r = q.parse(ML_RU, query, position, c);
    if (r)
        return ERR_CODE_SVC_INVALID_ARGS;
    RCQueryProcessor p(q);
    rcr::List list;
    std::string name = dateStamp(timeStamp);

    if (componentFlags == FLAG_ALL_COMPONENTS)
        name += "A_Z";
    else
        name += MeasureUnit::sym(c);

    size_t ofs = 0;
    int fileCount = 1;
    while(true) {
        auto f = retVal->add_file();
        auto pname = name + "_" + std::to_string(fileCount);
        f->set_name(pname + ".xlsx");
        rcr::List list;
        list.set_offset(ofs);
        list.set_size(32768);
        rcr::OperationResponse operationResponse;
        rcr::CardResponse cards;
        uint64_t cnt = 0;
        uint64_t sum = 0;
        p.exec(mDb, &t, 0, 0, dictionaries, list, &operationResponse, &cards, componentFlags, cnt, sum);

        SpreadSheetHelper spreadSheetHelper;
        xlnt::workbook book;
        book.title(pname);
        spreadSheetHelper.loadCards(book, cards);
        f->set_content(spreadSheetHelper.toString(book));

        if (cnt < list.size())
            break;
        ofs += 32768;
        fileCount++;
    }

    return 0;
}

void RcrImpl::addPackages(
    uint64_t cardId,
    const google::protobuf::RepeatedPtrField<::rcr::Package> &packages,
    const rcr::User &user,
    odb::database *db
) {

    odb::result<rcr::Package> qPackage(db->query<rcr::Package>(odb::query<rcr::Package>::card_id == cardId));
    std::vector <uint64_t> updatedPackageIds;
    updatedPackageIds.reserve(8);
    for (odb::result<rcr::Package>::iterator it(qPackage.begin()); it != qPackage.end(); it++) {
        auto qit = std::find_if(packages.begin(), packages.end(),
            [it] (auto v) {
                return it->box() == v.box();
        });

        if (qit != packages.end()) {
            updatedPackageIds.push_back(qit->id());
            // remove if qty = 0
            if (qit->qty() == 0) {
                db->erase(*qit);
            } else {
                rcr::Package p;
                p.set_id(it->id());
                p.set_card_id(cardId);
                p.set_box(qit->box());
                p.set_qty(qit->qty());
                db->update(p);
            }
        }
    }
    // insert a new ones
    for (auto it = packages.begin(); it != packages.end(); it++) {
        auto alreadyIt = std::find_if(updatedPackageIds.begin(), updatedPackageIds.end(),
            [it] (auto v) {
                return it->id() == v;
        });
        if (alreadyIt == updatedPackageIds.end()) {
            // not updatedPackageIds yet, insert a new one if qty is not zero
            rcr::Package p = *it;
            p.set_card_id(cardId);
            if (p.qty() != 0) {
                uint64_t pid = db->persist(p);
                LOG(INFO) << _("Added package id: ") << pid
                    << _(" to card id: ") << p.card_id()
                    << _(" box: ") << p.box()
                    << _(" qty: ") << p.qty()
                    << std::endl;
            }
        }
    }
}
