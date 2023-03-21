//
// Created by andrei on 17.03.23.
//

#ifndef RCR_RCQUERYPROCESSOR_H
#define RCR_RCQUERYPROCESSOR_H

#include "RCQuery.h"
#include "gen/rcr.pb.h"

class RCQueryProcessor {
private:
    const RCQuery *query;
public:
    RCQueryProcessor();
    RCQueryProcessor(const RCQuery &query);
    void exec(
        odb::database *db,
        odb::transaction *t,
        const rcr::ListRequest &list,
        rcr::OperationResponse *operationResponse,
        rcr::CardResponse *cards
    );

    void loadCards(
        odb::database *db,
        odb::transaction *t,
        rcr::CardResponse *retCards,
        const RCQuery *query,
        const rcr::ListRequest &list
    );

    int saveCard(
        odb::database *db,
        odb::transaction *t,
        const rcr::Card &card
    );
};

#endif //RCR_RCQUERYPROCESSOR_H
