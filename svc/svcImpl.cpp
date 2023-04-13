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
#include "MeasureUnit.h"
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
#include "SpreadSheetHelper.h"
#include "BoxName.h"
#include "string-helper.h"

using namespace odb::core;
using grpc::StatusCode;
using odb::query;

const int DEF_LIST_SIZE = 1000;
const std::string ERR_SVC_INVALID_ARGS = "Invalid arguments";

// default list size
#define DEF_LABEL_LIST_SIZE		100
#define MAX_LABEL_LIST_SIZE		10000
#define LOG(x) std::cerr

typedef uint64_t NOTIFICATION_TYPE;

static google::protobuf::util::JsonPrintOptions printJSONOptions;

static odb::database *odbconnect(
    struct ServiceConfig *config
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
        LOG(ERROR) << "load object " << id << " error: " << e.what();
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
        ss << " cause(" << cause << ")";
    if (!what.empty())
        ss << " what(" << what << ")";

    if (msg) {
        std::string s;
        google::protobuf::util::MessageToJsonString(*msg, &s, printJSONOptions);
        ss << " message: " << s;
    }
    return ss.str();
}

#define CHECK_PERMISSION(permission) \
	UserIds uids; \
	int flags = getAuthUser(context, &uids); \
	if ((flags == 0) && (permission != 0)) \
		return Status(StatusCode::PERMISSION_DENIED, ERR_SVC_PERMISSION_DENIED);

#define BEGIN_GRPC_METHOD(signature, requestMessage, transact) \
		transaction transact; \
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

const grpc::string ERR_NO_GRANTS("No grants to call");
const grpc::string ERR_NOT_IMPLEMENTED("Not implemented");

const grpc::Status& RcrImpl::STATUS_NO_GRANTS = grpc::Status(StatusCode::PERMISSION_DENIED, ERR_NO_GRANTS);
const grpc::Status& RcrImpl::STATUS_NOT_IMPLEMENTED = grpc::Status(StatusCode::UNIMPLEMENTED, ERR_NOT_IMPLEMENTED);

uint64_t VERSION_MAJOR = 0x010000;

RcrImpl::RcrImpl(struct ServiceConfig *config)
{
    srand(time(nullptr));    //
	mConfig = config;
	mDb = odbconnect(config);

    printJSONOptions.add_whitespace = true;
    printJSONOptions.always_print_primitive_fields = true;
    printJSONOptions.preserve_proto_field_names = true;
}

RcrImpl::~RcrImpl()
{
    delete mDb;
}

struct ServiceConfig *RcrImpl::getConfig()
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
    response->set_success(checkCredentialsNSetToken(t, mDb, response->mutable_user()));
    response->set_version(VERSION_MAJOR);
    response->set_version_name(getRandomName());
    END_GRPC_METHOD("login", request, response, t)
    return grpc::Status::OK;
}

::grpc::Status RcrImpl::chPropertyType(
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
    response->set_id(1);
    response->set_code(2);
    response->set_description(request->value().description());
    END_GRPC_METHOD("chPropertyType", request, response, t)
    return true ? grpc::Status::OK : grpc::Status(StatusCode::NOT_FOUND, "");
}

::grpc::Status RcrImpl::cardQuery(
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
    rcr::DictionariesResponse dictionaries;
    r = loadDictionaries(&dictionaries);
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
            RCQueryProcessor p(q);
            rcr::CardQueryResponse qr;
            uint64_t cnt = 0;
            uint64_t sum = 0;
            p.exec(mDb, &t, &dictionaries, request->list(),
                   response->mutable_rslt(), response->mutable_cards(),
                   componentFlags,
                   cnt, sum);
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
    r = loadDictionaries(response);
    END_GRPC_METHOD("getDictionaries", request, response, t)
    return ((r == 0) ? grpc::Status::OK : grpc::Status(StatusCode::UNKNOWN, ""));
}

::grpc::Status RcrImpl::cardPush(
    ::grpc::ServerContext* context,
    ::grpc::ServerReader<::rcr::CardRequest>* reader,
    ::rcr::OperationResponse* response
)
{
    if (reader == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    if (response == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    int r;
    BEGIN_GRPC_METHOD("cardPush", response, t)
    rcr::DictionariesResponse dictionaries;
    r = loadDictionaries(&dictionaries);
    rcr::CardRequest cardRequest;
    while (reader->Read(&cardRequest)) {
        RCQueryProcessor p;
        r = p.saveCard(mDb, &t, cardRequest, &dictionaries);
        if (r)
            break;
    }
    response->set_id(0);
    response->set_code(r);
    response->set_description("");
    END_GRPC_METHOD("cardPush", response, response, t)
    return (r == 0) ? grpc::Status::OK : grpc::Status(StatusCode::UNKNOWN, "");
}

int RcrImpl::loadDictionaries(
    rcr::DictionariesResponse *retVal
) {
    try {
        odb::result<rcr::Operation> qs(mDb->query<rcr::Operation>(odb::query<rcr::Operation>::id != 0));
        for (odb::result<rcr::Operation>::iterator i(qs.begin()); i != qs.end(); i++) {
            rcr::Operation *op = retVal->add_operation();
            op->CopyFrom(*i);
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "list operations error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "list operations unknown error";
    }

    try {
        odb::result<rcr::Symbol> qs(mDb->query<rcr::Symbol>(odb::query<rcr::Symbol>::id != 0));
        for (odb::result<rcr::Symbol>::iterator i(qs.begin()); i != qs.end(); i++) {
            rcr::Symbol *sym = retVal->add_symbol();
            sym->CopyFrom(*i);
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "list symbols error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "list symbols unknown error";
    }

    try {
        odb::result<rcr::PropertyType> qs(mDb->query<rcr::PropertyType>(odb::query<rcr::PropertyType>::id != 0));
        for (odb::result<rcr::PropertyType>::iterator i(qs.begin()); i != qs.end(); i++) {
            rcr::PropertyType *op = retVal->add_property_type();
            op->CopyFrom(*i);
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "list property type error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "list property type unknown error";
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
#if CMAKE_BUILD_TYPE == Debug
            if (op->name().empty())
                op->set_name(StockOperation::boxes2string(op->box_id()));
#endif
        }
    } catch (const odb::exception &e) {
        r = -1;
        LOG(ERROR) << "list box error: " << e.what();
    } catch (...) {
        r = -2;
        LOG(ERROR) << "list box unknown error";
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
    rcr::DictionariesResponse dictionaries;
    loadDictionaries(&dictionaries);

    size_t cnt = 0;
    size_t r = 0;
    for (auto f = request->file().begin(); f != request->file().end(); f++) {
        r += importExcelFile(t, mDb, request->symbol(), *f, request->prefix_box(),
            dictionaries);
        cnt++;
    }
    response->set_count(cnt);   // files
    response->set_sum(r);     // entries
    END_GRPC_METHOD("importExcel", response, response, t)
    return (r == 0) ? grpc::Status::OK : grpc::Status(StatusCode::UNKNOWN, "");
}

size_t RcrImpl::importExcelFile(
    odb::transaction &t,
    odb::database *db,
    const std::string &symbol,
    const rcr::ExcelFile &file,
    uint64_t prefixBox,
    rcr::DictionariesResponse &dictionaries
) {
    std::string pb;
    if (prefixBox)
        pb = StockOperation::boxes2string(prefixBox) + " ";
    uint64_t box = BoxName::extractFromFileName(pb + file.name()); //  <- add if filename contains boxes
    SpreadSheetHelper spreadSheet(file.name(), file.content());

    for (auto item = spreadSheet.items.begin(); item != spreadSheet.items.end(); item++) {
        rcr::CardRequest cardRequest;
        item->toCardRequest("+", symbol, box, cardRequest);
        RCQueryProcessor p;
        p.saveCard(mDb, &t, cardRequest, &dictionaries);
    }
    return spreadSheet.total;   // total count of items
}

int RcrImpl::checkUserRights(
    odb::transaction &t,
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
            return it->rights();
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "Check credentials error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "Check credentials unknown error";
    }
    return -1;
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
        db->persist(*retVal);   // save token
        return true;
    } catch (const odb::exception &e) {
        LOG(ERROR) << "Check credentials & set token error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "Check credentials & set token unknown error";
    }
    return false;
}

uint64_t RcrImpl::generateNewToken() {
    return rand();
}

grpc::Status RcrImpl::lsUser(
    grpc::ServerContext* context,
    const rcr::UserRequest* request,
    grpc::ServerWriter< rcr::User>* writer)
{
    int r = 0;
    BEGIN_GRPC_METHOD("lsUser", request, t)
    // std::cerr << pb2JsonString(request->user()) << std::endl;
    int rights = checkUserRights(t, mDb, request->user());
    try {
        odb::result<rcr::User> qs(mDb->query<rcr::User>(
            odb::query<rcr::User>::id != 0
        ));
        for (odb::result<rcr::User>::iterator it(qs.begin()); it != qs.end(); it++) {
            rcr::User u = *it;
            if (rights != 1) {
                u.set_token(0);
                u.set_password("");
            }
            writer->Write(u);
        }
    } catch (const odb::exception &e) {
        r = 1;
        LOG(ERROR) << "list user error: " << e.what();
    } catch (...) {
        r = 2;
        LOG(ERROR) << "list user unknown error";
    }

    END_GRPC_METHOD("lsUser", request, nullptr, t)
    return (r == 0) ? grpc::Status::OK : grpc::Status(StatusCode::UNKNOWN, "");
}

