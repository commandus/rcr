//
// Created by andrei on 17.03.23.
//

#include <algorithm>
#include "RCQueryProcessor.h"
#include "svc/svcImpl.h"

// ODB ORM
#include "gen/rcr.pb-odb.hxx"
#include "string-helper.h"
#include "gen/odb-views-odb.hxx"

#ifdef ENABLE_SQLITE
#include <odb/sqlite/database.hxx>
#include <sstream>

#endif
#ifdef ENABLE_PG
#include <odb/pgsql/database.hxx>
#endif

#define LOG(x) std::cerr

const std::string EMPTY_STRING = "";

RCQueryProcessor::RCQueryProcessor()
    : query(nullptr)
{

}

RCQueryProcessor::RCQueryProcessor(
    const RCQuery &aQuery
)
{
    query = &aQuery;
}

void RCQueryProcessor::exec(
    odb::database *db,
    odb::transaction *t,
    const rcr::DictionariesResponse *dictionaries,
    const rcr::List &list,
    rcr::OperationResponse *operationResponse,
    rcr::CardResponse *cards,
    uint32_t componentFlags,
    size_t &count,
    size_t &sum
)
{
    if (query == nullptr || operationResponse == nullptr || cards == nullptr)
        return;
    // set default
    operationResponse->set_id(0);
    operationResponse->set_code(0);
    operationResponse->set_description("");

    if (query->code == SO_NONE)
        return;

    switch (query->code) {
        case SO_LIST_NO_BOX:
        case SO_LIST:
            count = loadCards(db, t, dictionaries, cards, query, componentFlags, list, false);
            if (count == 0) {
                if (query->hasNominal()) {
                    count = loadCards(db, t, dictionaries, cards, query, componentFlags, list, true);
                } else {
                    // try wildcards name
                    if ((query->componentName.find('*') == std::string::npos)
                        &&
                        (query->componentName.find('%') == std::string::npos)
                        &&
                        (query->componentName.find('_') == std::string::npos)) {
                        RCQuery q2 = *query;
                        q2.componentName = q2.componentName + "*";
                        count = loadCards(db, t, dictionaries, cards, &q2, componentFlags, list, false);
                    }
                }
            }
            break;
        case SO_SET:
        case SO_ADD:
        case SO_SUB:
        case SO_MOV:
        case SO_COUNT:
        case SO_SUM:
            count = setCards(db, t, dictionaries, query, componentFlags, &sum);
            break;
    }
}

static std::string mkCardQueryClause(
    odb::database *db,
    const rcr::DictionariesResponse *dictionaries,
    uint64_t symbId,
    const RCQuery *query,
    bool extendNominal
)
{
    odb::result<rcr::CardQuery> r;
    std::stringstream ss;
    std::string cn = toUpperCase(query->componentName);

    // check box
    if (query->boxes) {
        uint64_t startBox = query->boxes;
        int dp;
        uint64_t finishBox = StockOperation::maxBox(query->boxes, dp);
#ifdef ENABLE_SQLITE
        // Sqlite support int64 not uint64
        if (finishBox == std::numeric_limits<uint64_t>::max())
            finishBox = std::numeric_limits<int64_t>::max();
#endif
        ss << "id in (select p.card_id from Package p where p.box >= "
           << startBox << " and p.box <= " << finishBox << ") and ";
    }
    // check properties
    if (!query->properties.empty()) {
        for (std::pair<std::string, std::string> p: query->properties) {
            uint64_t ptid = 0;
            for (auto it = dictionaries->property_type().begin(); it != dictionaries->property_type().end(); it++) {
                if (it->key() == p.first)
                    ptid = it->id();
                break;
            }
            // check does it exists
            if (!ptid)
                continue;
            ss << "id in (select p.card_id from Property p where p.property_type_id = "
               << ptid << ") and ";
        }
    }

    std::string nominalClause;
    std::stringstream ssNominal;
    ssNominal << " and ";
    if (extendNominal) {
        ssNominal << "nominal >= " << query->nominal;
    } else {
        ssNominal << "nominal = " << query->nominal;
    }
    nominalClause = ssNominal.str();

    if (cn.empty() || cn == "*") {  // just all
        if (symbId)
            ss << "symbol_id = " << symbId << nominalClause;
        else
        if (extendNominal)
            ss << "nominal >= " << query->nominal;
        else
            ss << "nominal = " << query->nominal;
    } else {
        if (cn.find("*") != std::string::npos) {    // LIKE 'K%'
            std::replace(cn.begin(), cn.end(), '*', '%');
            if (symbId)
                ss << "uname LIKE '" << cn << "' and symbol_id = " << symbId << nominalClause;
            else
                ss << "uname LIKE '" << cn << "'" << nominalClause;
        } else {
            if (symbId)
                ss << "uname = '" << cn << "' and symbol_id = " << symbId << nominalClause;
            else
                ss << "uname = '" << cn << "'" << nominalClause;
        }
    }
#if CMAKE_BUILD_TYPE == Debug
    std::cerr << "Card query: " << ss.str() << std::endl;
#endif

    return ss.str();
}

static size_t countCards(
    odb::database *db,
    uint64_t symbId,
    const rcr::DictionariesResponse *dictionaries,
    const RCQuery *query,
    bool extendNominal
)
{
    odb::result<rcr::CardCount> r;
    std::string clause = mkCardQueryClause(db, dictionaries, symbId, query, extendNominal);
    return db->query_value<rcr::CardCount>(clause).count;
}

size_t RCQueryProcessor::loadCards(
    odb::database *db,
    odb::transaction *t,
    const rcr::DictionariesResponse *dictionaries,
    rcr::CardResponse *retCards,
    const RCQuery *query,
    uint32_t componentFlags,
    const rcr::List &list,
    bool extendNominal
) {
    size_t cnt = 0;
    size_t sz = 0;
    size_t r = 0;
    try {
        uint64_t symbId;
        if (componentFlags == FLAG_ALL_COMPONENTS)
            symbId = 0;
        else
            symbId = RCQueryProcessor::measure2symbolId(dictionaries, query->measure);
        r = countCards(db, symbId, dictionaries, query, extendNominal);
        std::stringstream ssClause;
        ssClause << mkCardQueryClause(db, dictionaries, symbId, query, extendNominal);
        std::string clause = ssClause.str();
        odb::result<rcr::CardQuery> q = odb::result<rcr::CardQuery>(db->query<rcr::CardQuery>(clause));
        for (odb::result<rcr::CardQuery>::iterator itCard(q.begin()); itCard != q.end(); itCard++) {
            if (!hasAllProperties(db, t, query->properties, itCard->id, dictionaries))
                continue;
            google::protobuf::RepeatedPtrField<rcr::Package> pkgs;
            if (!loadPackages(db, t, query->boxes, itCard->id, &pkgs))
                continue;
            // emulate LIMIT OFFSET clause
            cnt++;
            if (cnt - 1 < list.offset())
                continue;
            sz++;
            if (sz > list.size())
                break;
            rcr::CardNPropetiesPackages *c = retCards->mutable_cards()->Add();
            c->mutable_card()->set_id(itCard->id);
            c->mutable_card()->set_name(itCard->name);
            c->mutable_card()->set_uname(toUpperCase(itCard->name));
            c->mutable_card()->set_nominal(itCard->nominal);
            c->mutable_card()->set_symbol_id(itCard->symbol_id);
            c->mutable_packages()->CopyFrom(pkgs);
            loadPropertiesWithName(db, t, itCard->id, c->mutable_properties(), dictionaries);
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "loadCards error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "loadCards unknown error";
    }
    return r;
}

int RCQueryProcessor::saveCard(
    odb::database *db,
    odb::transaction *t,
    const rcr::CardRequest &cardRequest,
    const rcr::DictionariesResponse *dictionaries
)
{
    rcr::Card card;
    const rcr::Symbol *symbolId = findSymbol(dictionaries, cardRequest.symbol_name());
    if (!symbolId)
        return grpc::StatusCode::INVALID_ARGUMENT;
    card.set_symbol_id(symbolId->id());
    card.set_nominal(cardRequest.nominal());
    card.set_name(cardRequest.name());
    card.set_uname(toUpperCase(cardRequest.name()));

    // find out card
    uint64_t foundCardId = findCardByNameNominalProperties(cardRequest, db, t, dictionaries);

    // Check does operation supported
    const rcr::Operation *oper = findOperation(dictionaries, cardRequest.operation_symbol());
    if (oper) {
        if (oper->symbol() == "+") {
            if (foundCardId) {
                card.set_id(foundCardId);
                db->update(card);
            } else {
                // add a new card
                uint64_t cid = db->persist(card);
                // set properties
                setProperties(db, t, cardRequest, cid, dictionaries);
            }
            // increment qty in specified box
            uint64_t packageId;
            uint64_t qPrevious = getQuantity(db, t, packageId, card.id(), cardRequest.box());
            setQuantity(db, t, packageId, card.id(), cardRequest.box(), qPrevious + cardRequest.qty());
            updateBoxOnInsert(db, t, cardRequest.box(), "");
        }
    }
    return 0;
}

/**
 * Update box reference
 * @param db database
 * @param t transaction
 * @param boxId box to be updated
 * @param name name of the box
 * @return 0- success, != 0 - error
 */
int RCQueryProcessor::updateBoxOnInsert(
    odb::database *db,
    odb::transaction *t,
    uint64_t boxId,
    const std::string &name
)
{
    try {
        odb::result<rcr::Box> q(db->query<rcr::Box>(
            odb::query<rcr::Box>::box_id == boxId
        ));
        odb::result<rcr::Box>::iterator itBox(q.begin());
        if (itBox == q.end()) {
            rcr::Box box;
            box.set_box_id(boxId);
            box.set_name(name);
            box.set_uname(toUpperCase(name));
            db->persist(box);
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "updateBoxOnInsert error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "updateBoxOnInsert unknown error";
    }
    return 0;
}

/**
 * On package remove check is box empty.
 * Empty box to be deleted.
 * @param db
 * @param t
 * @param boxId
 * @return
 */
int RCQueryProcessor::updateBoxOnRemove(
    odb::database *db,
    odb::transaction *t,
    uint64_t boxId
)
{
    try {
        odb::result<rcr::Package> q(db->query<rcr::Package>(
                odb::query<rcr::Package>::box == boxId
        ));
        odb::result<rcr::Package>::iterator itPackage(q.begin());
        if (itPackage == q.end()) {
            // remove from reference if exists
            odb::result<rcr::Box> qb(db->query<rcr::Box>(
                    odb::query<rcr::Box>::box_id == boxId
            ));
            odb::result<rcr::Box>::iterator itBox(qb.begin());
            if (itBox != qb.end())
                db->erase(*itBox);
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "updateBoxOnRemove error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "updateBoxOnRemove unknown error";
    }
    return 0;
}

// return nullptr if not found
const rcr::Symbol* RCQueryProcessor::findSymbol(
    const rcr::DictionariesResponse *dictionaries,
    const std::string &sym
)
{
    if (!dictionaries)
        return nullptr;
    for (auto it = dictionaries->symbol().begin(); it != dictionaries->symbol().end(); it++) {
        if (it->sym() == sym)
            return &*it;
    }
    return nullptr;
}

// return nullptr if not found
const rcr::PropertyType* RCQueryProcessor::findPropertyType(
    const rcr::DictionariesResponse *dictionaries,
    const std::string &key
)
{
    if (!dictionaries)
        return nullptr;
    for (auto it = dictionaries->property_type().begin(); it != dictionaries->property_type().end(); it++) {
        if (it->key() == key)
            return &*it;
    }
    return nullptr;
}

// return nullptr if failed
const rcr::PropertyType* RCQueryProcessor::findPropertyTypeOrAdd(
    rcr::DictionariesResponse *dictionaries,
    odb::database *db,
    odb::transaction *transaction,
    const std::string &key
)
{
    const rcr::PropertyType* r = findPropertyType(dictionaries, key);
    if (r)
        return r;
    rcr::PropertyType pt;
    pt.set_key(key);
    pt.set_description("");
    pt.set_id(db->persist(pt));
    rcr::PropertyType *ra = dictionaries->add_property_type();
    *ra = pt;
    return ra;
}

const std::string& RCQueryProcessor::findPropertyTypeName(
    const rcr::DictionariesResponse *dictionaries,
    uint64_t propertyId
)
{
    if (!dictionaries)
        return EMPTY_STRING;
    for (auto it = dictionaries->property_type().begin(); it != dictionaries->property_type().end(); it++) {
        if (it->id() == propertyId)
            return it->key();
    }
    return EMPTY_STRING;
}

// return nullptr if not found
const rcr::Operation* RCQueryProcessor::findOperation(
    const rcr::DictionariesResponse *dictionaries,
    const std::string &symbol
)
{
    if (!dictionaries)
        return nullptr;
    for (auto it = dictionaries->operation().begin(); it != dictionaries->operation().end(); it++) {
        if (it->symbol() == symbol)
            return &*it;
    }
    return nullptr;
}

void RCQueryProcessor::copyKnownProperties(
    google::protobuf::RepeatedPtrField<rcr::Property> *retVal,
    const google::protobuf::RepeatedPtrField<::rcr::PropertyRequest> &from,
    const rcr::DictionariesResponse *dictionaries
) {
    if (!dictionaries || !retVal)
        return;
    for (auto it = from.begin(); it != from.end(); it++) {
        auto pt = findPropertyType(dictionaries, it->property_type_name());
        if (!pt)
            continue;   // unknown
        auto p = retVal->Add();
        p->set_property_type_id(pt->id());
        p->set_value(it->value());
    }
}

uint64_t RCQueryProcessor::findCardByNameNominalProperties(
    const rcr::CardRequest &cardRequest,
    odb::database *db,
    odb::transaction *transaction,
    const rcr::DictionariesResponse *dictionaries
)
{
    const rcr::Symbol *symbolId = findSymbol(dictionaries, cardRequest.symbol_name());
    if (!symbolId)
        return 0;
    try {
        odb::result<rcr::Card> q(db->query<rcr::Card>(
            odb::query<rcr::Card>::name == cardRequest.name()
            &&
            odb::query<rcr::Card>::nominal == cardRequest.nominal()
            &&
            odb::query<rcr::Card>::symbol_id == symbolId->id()
        ));
        for (odb::result<rcr::Card>::iterator itCard(q.begin()); itCard != q.end(); itCard++) {
            bool foundAllproperties = hasAllProperties2(db, transaction, cardRequest.properties(), itCard->id(), dictionaries);
            if (foundAllproperties) {
                return itCard->id();
            }
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "findCardByNameNominalProperties error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "findCardByNameNominalProperties unknown error";
    }
    return 0;
}

uint64_t RCQueryProcessor::getQuantity(
    odb::database *db,
    odb::transaction *transaction,
    uint64_t &retPackageId,
    uint64_t cardId,
    uint64_t box
) {
    uint64_t r = 0;
    uint64_t lastBox = StockOperation::lastBox(box);
#ifdef ENABLE_SQLITE
    // SQLite3 does not support uimt64 but int64
    if (lastBox == 0xffffffffffffffff)
        lastBox = 0x7fffffffffffffff;
#endif
    retPackageId = 0;
    try {
        odb::result<rcr::Package> q(db->query<rcr::Package>(
            odb::query<rcr::Package>::card_id == cardId
            &&
            odb::query<rcr::Package>::box >= box
            &&
            odb::query<rcr::Package>::box <= lastBox
        ));
        odb::result<rcr::Package>::iterator it(q.begin());
        if (it != q.end()) {
            retPackageId = it->id();
            r = it->qty();
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "Get box qty error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "Get box qty unknown error";
    }
    return r;
}

uint64_t RCQueryProcessor::setQuantity(
    odb::database *db,
    odb::transaction *transaction,
    uint64_t packageId,
    uint64_t cardId,
    uint64_t box,
    uint64_t qty
)
{
    rcr::Package p;
    p.set_id(packageId);
    p.set_card_id(cardId);
    p.set_box(box);
    p.set_qty(qty);

    try {
        if (packageId == 0) {
            packageId = db->persist(p);
        }
        else
            db->update(p);
    } catch (const odb::exception &e) {
        LOG(ERROR) << "Set box qty error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "Set box qty unknown error";
    }
    return packageId;
}

void RCQueryProcessor::setProperties(
    odb::database *db,
    odb::transaction *transaction,
    const rcr::CardRequest &card,
    uint64_t cardId,
    const rcr::DictionariesResponse *dictionaries
) {
    // delete old ones if exists
    try {
        odb::result<rcr::Property> qs(db->query<rcr::Property>(odb::query<rcr::Property>::card_id == card.id()));
        for (odb::result<rcr::Property>::iterator i(qs.begin()); i != qs.end(); i++) {
            db->erase(*i);
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "Remove property error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "Remove property unknown error";
    }
    // set a new ones
    for (auto p = card.properties().begin(); p != card.properties().end(); p++) {
        try {
            rcr::Property prop;
            prop.set_card_id(cardId);
            auto pti = findPropertyType(dictionaries, p->property_type_name());
            if (!pti)
                continue;   // no property type found
            prop.set_property_type_id(pti->id());
            prop.set_value(p->value());
            db->persist(prop);
        } catch (const odb::exception &e) {
            LOG(ERROR) << "Set property error: " << e.what();
        } catch (...) {
            LOG(ERROR) << "Set property unknown error";
        }
    }
}

uint64_t RCQueryProcessor::measure2symbolId(
     const rcr::DictionariesResponse *dictionaries,
     const COMPONENT measure
) {
    if (!dictionaries)
        return 0;

    const std::string &mn = MeasureUnit::sym(measure);
    for (auto it = dictionaries->symbol().begin(); it != dictionaries->symbol().end(); it++) {
        if (it->sym() == mn)
            return it->id();
    }
    return 0;
}

bool RCQueryProcessor::hasAllProperties(
    odb::database *db,
    odb::transaction *transaction,
    const std::map<std::string, std::string> &props,
    uint64_t cardIdWhere,
    const rcr::DictionariesResponse *dictionaries
) {
    int cnt = 0;
    try {
        odb::result<rcr::Property> q(db->query<rcr::Property>(
            odb::query<rcr::Property>::card_id == cardIdWhere
        ));
        for (odb::result<rcr::Property>::iterator itProperty(q.begin()); itProperty != q.end(); itProperty++) {
            auto fit = std::find_if(props.begin(), props.end(),
            [itProperty, dictionaries](auto v) {
                    const rcr::PropertyType* pt = findPropertyType(dictionaries, v.first);
                    if (!pt)
                        return false;
                    return itProperty->property_type_id() == pt->id();
                } );
            if (fit != props.end()) {
                if (itProperty->value() == fit->second)
                    cnt++;
                break;
            }
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "hasAllProperties error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "hasAllProperties unknown error";
    }
    return cnt == props.size();
}

bool RCQueryProcessor::hasAllProperties2(
    odb::database *db,
    odb::transaction *transaction,
    const google::protobuf::RepeatedPtrField<rcr::PropertyRequest> &props,
    uint64_t cardIdWhere,
    const rcr::DictionariesResponse *dictionaries
)
{
    int cnt = 0;
    try {
        odb::result<rcr::Property> q(db->query<rcr::Property>(
                odb::query<rcr::Property>::card_id == cardIdWhere
        ));
        for (odb::result<rcr::Property>::iterator itProperty(q.begin()); itProperty != q.end(); itProperty++) {
            auto fit = std::find_if(props.begin(), props.end(),
                [itProperty, dictionaries](auto v) {
                    const rcr::PropertyType* pt = findPropertyType(dictionaries, v.property_type_name());
                    if (!pt)
                        return false;
                    return itProperty->property_type_id() == pt->id();
                } );
            if (fit != props.end()) {
                if (itProperty->value() == fit->value())
                    cnt++;
                break;
            }
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "hasAllProperties error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "hasAllProperties unknown error";
    }
    return cnt == props.size();
}

bool RCQueryProcessor::loadPackages(
    odb::database *db,
    odb::transaction *transaction,
    uint64_t boxId, // 0- all
    uint64_t cardId,
    google::protobuf::RepeatedPtrField<rcr::Package> *retPackages
) {
    bool r = false;
    try {
        odb::result<rcr::Package> q(db->query<rcr::Package>(
                odb::query<rcr::Package>::card_id == cardId
        ));
        for (odb::result<rcr::Package>::iterator it(q.begin()); it != q.end(); it++) {
            if (!StockOperation::isBoxInBoxes(it->box(), boxId))
                continue;
            auto p = retPackages->Add();
            p->CopyFrom(*it);
#if CMAKE_BUILD_TYPE == Debug
            p->set_box_name(StockOperation::boxes2string(it->box()));
#endif
            r = true;
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "loadPackages error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "loadPackages unknown error";
    }
    return r;
}

void RCQueryProcessor::loadProperties(
    odb::database *db,
    odb::transaction *transaction,
    uint64_t cardId,
    google::protobuf::RepeatedPtrField<rcr::Property> *retProperties
) {
    try {
        odb::result<rcr::Property> q(db->query<rcr::Property>(
                odb::query<rcr::Property>::card_id == cardId
        ));
        for (odb::result<rcr::Property>::iterator it(q.begin()); it != q.end(); it++) {
            auto p = retProperties->Add();
            p->CopyFrom(*it);
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "load properties error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "load properties unknown error";
    }
}

void RCQueryProcessor::loadPropertiesWithName(
    odb::database *db,
    odb::transaction *transaction,
    uint64_t cardId,
    google::protobuf::RepeatedPtrField<rcr::PropertyWithName> *retProperties,
    const rcr::DictionariesResponse *dictionaries
) {
    try {
        odb::result<rcr::Property> q(db->query<rcr::Property>(
            odb::query<rcr::Property>::card_id == cardId
        ));
        for (odb::result<rcr::Property>::iterator it(q.begin()); it != q.end(); it++) {
            auto p = retProperties->Add();
            p->set_id(it->id());
            const std::string &ptn = findPropertyTypeName(dictionaries, it->property_type_id());
            p->set_property_type(ptn);
            p->set_value(it->value());
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "load properties error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "load properties unknown error";
    }
}

size_t RCQueryProcessor::setCards(
    odb::database *db,
    odb::transaction *t,
    const rcr::DictionariesResponse *dictionaries,
    const RCQuery *query,
    uint32_t componentFlags,
    size_t *sum
) {
    size_t cnt = 0;
    try {
        uint64_t symbId;
        if (componentFlags == FLAG_ALL_COMPONENTS)
            symbId = 0;
        else
            symbId = RCQueryProcessor::measure2symbolId(dictionaries, query->measure);

        std::string clause = mkCardQueryClause(db, dictionaries, symbId, query, false);
        odb::result<rcr::CardQuery> q = odb::result<rcr::CardQuery>(db->query<rcr::CardQuery>(clause));
        odb::result<rcr::CardQuery>::iterator itCard(q.begin());
        for (; itCard != q.end(); itCard++) {
            if (!hasAllProperties(db, t, query->properties, itCard->id, dictionaries))
                continue;
            //
            uint64_t packageId;
            // get packageId, if not, ret 0
            uint64_t q = getQuantity(db, t, packageId, itCard->id, query->boxes);
            if (sum)
                *sum += q;

            switch(query->code) {
                case SO_SET:
                    setQuantity(db, t, packageId, itCard->id, query->boxes, query->count);
                    updateBoxOnInsert(db, t, query->boxes, "");
                    break;
                case SO_ADD:
                    setQuantity(db, t, packageId, itCard->id, query->boxes, q + query->count);
                    updateBoxOnInsert(db, t, query->boxes, "");
                    break;
                case SO_SUB:
                    setQuantity(db, t, packageId, itCard->id, query->boxes,
                    q > query->count ? q - query->count : 0);
                    updateBoxOnRemove(db, t, query->boxes);
                    break;
                case SO_MOV: {
                    // remove from the source box
                    setQuantity(db, t, packageId, itCard->id, query->boxes,
                        q > query->count ? q - query->count : 0);
                    updateBoxOnRemove(db, t, query->boxes);
                    // put to destination box
                    uint64_t destCardId = itCard->id; // same card
                    // get destination packageId, if not, ret 0
                    uint64_t qDest = getQuantity(db, t, packageId, destCardId, query->destinationBox);
                    setQuantity(db, t, packageId, destCardId, query->destinationBox, qDest + query->count);
                    updateBoxOnInsert(db, t, query->destinationBox, "");
                }
                    break;
                default:
                    break;
            }
            cnt++;
        }
        if (cnt == 0 && itCard == q.end() && query->code == SO_SET) {
            // special case, "=" if no exists
            rcr::Card card;
            uint64_t symbId = RCQueryProcessor::measure2symbolId(dictionaries, query->measure);
            card.set_symbol_id(symbId);
            card.set_name(query->componentName);
            card.set_nominal(query->nominal);
            uint64_t cid = db->persist(card);

            // set properties
            for (std::map <std::string, std::string>::const_iterator it = query->properties.begin(); it != query->properties.end(); it++) {
                rcr::Property property;
                property.set_card_id(cid);
                const rcr::PropertyType *pt = findPropertyType(dictionaries, it->first);
                if (!pt)
                    continue;   // unknown
                property.set_property_type_id(pt->id());
                property.set_value(it->second);
                uint64_t ppid = db->persist(property);
            }

            rcr::Package package;
            package.set_card_id(cid);
            package.set_box(query->boxes);
            package.set_qty(query->count);
            uint64_t pid = db->persist(package);
            // update bi=ox reference
            updateBoxOnInsert(db, t, query->boxes, "");
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "setCards error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "setCards unknown error";
    }
    return cnt;
}
