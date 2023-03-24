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
        const rcr::DictionariesResponse *dictionaries,
        const rcr::ListRequest &list,
        rcr::OperationResponse *operationResponse,
        rcr::CardResponse *cards
    );

    void loadCards(
        odb::database *db,
        odb::transaction *t,
        const rcr::DictionariesResponse *dictionaries,
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
        const rcr::DictionariesResponse *dictionaries,
        const std::string &sym
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

    uint64_t findCardByNameNominalProperties(
        const rcr::CardRequest &cardRequest,
        odb::database *db,
        odb::transaction *transaction,
        const rcr::DictionariesResponse *dictionaries
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
        const rcr::CardRequest &card,
        uint64_t cardId,
        const rcr::DictionariesResponse *dictionaries
    );

    uint64_t measure2symbolId(
        const rcr::DictionariesResponse *dictionaries,
        const MEASURE measure
    );

    bool hasAllProperties(
        odb::database *db,
        odb::transaction *transaction,
        const std::map<std::string, std::string> &what,
        uint64_t cardIdWhere,
        const rcr::DictionariesResponse *dictionaries
    );

    bool hasAllProperties2(
        odb::database *db,
        odb::transaction *transaction,
        const google::protobuf::RepeatedPtrField<rcr::PropertyRequest> &what,
        uint64_t cardIdWhere,
        const rcr::DictionariesResponse *dictionaries
    );

    void loadPackages(odb::database *db, odb::transaction *transaction, uint64_t cardId,
                      google::protobuf::RepeatedPtrField<rcr::Package> *retPackages);

    void loadProperties(odb::database *db, odb::transaction *transaction, uint64_t id,
                        google::protobuf::RepeatedPtrField<rcr::Property> *retProperties);
};

#endif //RCR_RCQUERYPROCESSOR_H
