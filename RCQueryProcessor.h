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
        const rcr::CardRequest &card,
        const rcr::DictionariesResponse *dictionaries
    );

    // return nullptr if not found
    static const rcr::Symbol* findSymbol(
        const rcr::DictionariesResponse *dictionaries, const std::string &sym
    );
    // return nullptr if not found
    static const rcr::PropertyType* findPropertyType(
        const rcr::DictionariesResponse *dictionaries,
        const std::string &key
    );
    // return nullptr if not found
    static const rcr::Operation* findOperation(
        const rcr::DictionariesResponse *dictionaries,
        const std::string &symbol
    );

    void copyKnownProperties(
        google::protobuf::RepeatedPtrField<rcr::Property> *retVal,
        const google::protobuf::RepeatedPtrField<::rcr::PropertyRequest> &from,
        const rcr::DictionariesResponse *dictionaries
    );

    bool findCardByNameNominalProperties(
        rcr::Card &card,
        odb::database *db,
        odb::transaction *transaction
    );

    uint64_t getQuantity(
        odb::database *db,
        odb::transaction *transaction,
        uint64_t &retPackageId,
        uint64_t cardId,
        uint64_t box
    );

    uint64_t setQuantity(
        odb::database *db,
        odb::transaction *transaction,
        uint64_t packageId,
        uint64_t cardId,
        uint64_t box,
        uint64_t qty
    );

    void setProperties(
        odb::database *db,
        odb::transaction *transaction,
        const rcr::Card &card
    );
};

#endif //RCR_RCQUERYPROCESSOR_H
