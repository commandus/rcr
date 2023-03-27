//
// Created by andrei on 17.03.23.
//

#include "RCQueryProcessor.h"
#include "svc/svcImpl.h"

// ODB ORM
#include "gen/rcr.pb-odb.hxx"
#include "string-helper.h"

#ifdef ENABLE_SQLITE
#include <odb/sqlite/database.hxx>
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
    const rcr::ListRequest &list,
    rcr::OperationResponse *operationResponse,
    rcr::CardResponse *cards
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
            loadCards(db, t, dictionaries,cards, query, list);
            break;
        case SO_SET:

            break;
    }
}

void RCQueryProcessor::loadCards(
    odb::database *db,
    odb::transaction *t,
    const rcr::DictionariesResponse *dictionaries,
    rcr::CardResponse *retCards,
    const RCQuery *query,
    const rcr::ListRequest &list
) {
    size_t cnt = 0;
    size_t sz = 0;
    try {
        odb::result<rcr::Card> q(db->query<rcr::Card>(
            odb::query<rcr::Card>::uname == toUpperCase(query->componentName)
            &&
            odb::query<rcr::Card>::nominal == query->nominal
            &&
            odb::query<rcr::Card>::symbol_id == measure2symbolId(dictionaries, query->measure)
        ));
        for (odb::result<rcr::Card>::iterator itCard(q.begin()); itCard != q.end(); itCard++) {
            if (!hasAllProperties(db, t, query->properties, itCard->id(), dictionaries))
                continue;
            cnt++;
            if (cnt - 1 < list.offset())
                continue;
            sz++;
            if (sz > list.size())
                break;
            //
            rcr::CardNPropetiesPackages *c = retCards->mutable_cards()->Add();
            c->mutable_card()->CopyFrom(*itCard);
            loadPackages(db, t, itCard->id(), c->mutable_packages());
            loadPropertiesWithName(db, t, itCard->id(), c->mutable_properties(), dictionaries);
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "findCardByNameNominalProperties error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "findCardByNameNominalProperties unknown error";
    }
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
        }
    }
    return 0;
}

int RCQueryProcessor::updateBox1(
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
        LOG(ERROR) << "findCardByNameNominalProperties error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "findCardByNameNominalProperties unknown error";
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
    uint64_t r;
    retPackageId = 0;
    try {
        odb::result<rcr::Package> q(db->query<rcr::Package>(
                odb::query<rcr::Package>::card_id == cardId
                &&
                odb::query<rcr::Package>::box == box
        ));
        odb::result<rcr::Package>::iterator it(q.begin());
        if (it == q.end())
            return 0;
        retPackageId = it->id();
        r = it->qty();
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
            updateBox1(db, transaction, box, "");
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
     const MEASURE measure
) {
    if (!dictionaries)
        return 0;

    const std::string &mn = MeasureUnit::sym(ML_RU, measure);
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

void RCQueryProcessor::loadPackages(
    odb::database *db,
    odb::transaction *transaction,
    uint64_t cardId,
    google::protobuf::RepeatedPtrField<rcr::Package> *retPackages
) {
    try {
        odb::result<rcr::Package> q(db->query<rcr::Package>(
                odb::query<rcr::Package>::card_id == cardId
        ));
        for (odb::result<rcr::Package>::iterator it(q.begin()); it != q.end(); it++) {
            auto p = retPackages->Add();
            p->CopyFrom(*it);
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "loadPackages error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "loadPackages unknown error";
    }
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
