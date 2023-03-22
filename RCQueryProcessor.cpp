//
// Created by andrei on 17.03.23.
//

#include "RCQueryProcessor.h"
#include "svc/svcImpl.h"

// ODB ORM
#include "gen/rcr.pb-odb.hxx"
#ifdef ENABLE_SQLITE
#include <odb/sqlite/database.hxx>
#endif
#ifdef ENABLE_PG
#include <odb/pgsql/database.hxx>
#endif

#define LOG(x) std::cerr

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
    cards->set_count(0);

    if (query->code == SO_NONE)
        return;

    switch (query->code) {
        case SO_LIST_NO_BOX:
            loadCards(db, t, cards, query, list);
            break;
        case SO_LIST:
            break;
        case SO_SET:

            break;
    }
}

void RCQueryProcessor::loadCards(
    odb::database *db,
    odb::transaction *t,
    rcr::CardResponse *retCards,
    const RCQuery *query,
    const rcr::ListRequest &list
) {

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
    copyKnownProperties(card.mutable_properties(), cardRequest.properties(), dictionaries);

    // find out card
    bool found = findCardByNameNominalProperties(card, db, t);

    // Check does operation supported
    const rcr::Operation *oper = findOperation(dictionaries, cardRequest.operation_symbol());
    if (oper) {
        if (oper->symbol() == "+") {
            if (found) {
                db->update(card);
            } else {
                // add a new card
                card.set_id(db->persist(card));
                // set properties
                setProperties(db, t, card);
            }
            // increment qty in specified box
            uint64_t packageId;
            uint64_t qPrevious = getQuantity(db, t, packageId, card.id(), cardRequest.box());
            setQuantity(db, t, packageId, card.id(), cardRequest.box(), qPrevious + cardRequest.qty());
        }
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

bool RCQueryProcessor::findCardByNameNominalProperties(
    rcr::Card &card,
    odb::database *db,
    odb::transaction *transaction
)
{
    bool r = false;
    try {
        odb::result<rcr::Card> q(db->query<rcr::Card>(
            odb::query<rcr::Card>::name == card.name()
            &&
            odb::query<rcr::Card>::nominal == card.nominal()
            &&
            odb::query<rcr::Card>::symbol_id == card.symbol_id()
        ));
        for (odb::result<rcr::Card>::iterator itCard(q.begin()); itCard != q.end(); itCard++) {
            bool foundAllproperties = true;
            for (auto p = card.properties().begin(); p != card.properties().end(); p++) {
                if (std::find_if(itCard->properties().begin(), itCard->properties().end(),
                        [p](auto v) { return p->id() == v.id(); } )
                        == itCard->properties().end()) {
                    foundAllproperties = false;
                    break;
                }
            }
            if (foundAllproperties) {
                r = true;   // found
                card.set_id(itCard->id());
                break;
            }
        }
    } catch (const odb::exception &e) {
        LOG(ERROR) << "findCardByNameNominalProperties error: " << e.what();
    } catch (...) {
        LOG(ERROR) << "findCardByNameNominalProperties unknown error";
    }
    return r;
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
        if (packageId == 0)
            packageId = db->persist(p);
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
    const rcr::Card &card
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
            rcr::Property prop(*p);
            db->persist(prop);
        } catch (const odb::exception &e) {
            LOG(ERROR) << "Set property error: " << e.what();
        } catch (...) {
            LOG(ERROR) << "Set property unknown error";
        }
    }

}
