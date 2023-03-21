//
// Created by andrei on 17.03.23.
//

#include "RCQueryProcessor.h"
#include "svc/svcImpl.h"

// ODB ORM
#include "gen/rcr.pb-odb.hxx"

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
    const rcr::Card &card
)
{
    return 0;
}
