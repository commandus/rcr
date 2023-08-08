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
        uint64_t userId,
        int rights,
        const rcr::DictionariesResponse *dictionaries,
        const rcr::List &list,
        rcr::OperationResponse *operationResponse,
        rcr::CardResponse *cards,
        uint32_t componentFlags,
        size_t &count,
        size_t &sum
    );

    /**
     * Return total found record count
     * @param db
     * @param t
     * @param dictionaries
     * @param retCards
     * @param query
     * @param componentFlags
     * @param list offset, count of records
     * @param extendNominal extend nominal range
     * @return
     */
    size_t loadCards(
        odb::database *db,
        odb::transaction *t,
        const rcr::DictionariesResponse *dictionaries,
        rcr::CardResponse *retCards,
        const RCQuery *query,
        uint32_t componentFlags,
        const rcr::List &list,
        bool extendNominal
    );

    int saveCard(
        odb::database *db,
        odb::transaction *t,
        uint64_t userId,
        const rcr::CardRequest &card,
        const rcr::DictionariesResponse *dictionaries
    );

    int updateBoxOnInsert(
        odb::database *db,
        odb::transaction *t,
        uint64_t boxId,
        const std::string &name
    );

    int updateBoxOnRemove(
        odb::database *db,
        odb::transaction *t,
        uint64_t boxId
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

    // return empty string if not found
    static const std::string& findPropertyTypeName(
        const rcr::DictionariesResponse *dictionaries,
        uint64_t key
    );

    // return nullptr if failed
    static const rcr::PropertyType* findPropertyTypeOrAdd(
        rcr::DictionariesResponse *dictionaries,
        odb::database *db,
        odb::transaction *transaction,
        const std::string &key
    );

    // return nullptr if not found
    static const rcr::Operation* findOperation(
        const rcr::DictionariesResponse *dictionaries,
        const std::string &symbol
    );

    // return nullptr if not found
    static const rcr::Operation* findOperationById(
        const rcr::DictionariesResponse *dictionaries,
        const uint64_t &id
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

    static uint64_t measure2symbolId(
        const rcr::DictionariesResponse *dictionaries,
        const COMPONENT measure
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

    bool loadPackages(
        odb::database *db,
        odb::transaction *transaction,
        uint64_t boxId, // 0- all
        uint64_t cardId,
        google::protobuf::RepeatedPtrField<rcr::Package> *retPackages
    );

    void loadProperties(
        odb::database *db,
        odb::transaction *transaction,
        uint64_t cardId,
        google::protobuf::RepeatedPtrField<rcr::Property> *retProperties
    );

    void loadPropertiesWithName(
        odb::database *db,
        odb::transaction *transaction,
        uint64_t cardId,
        google::protobuf::RepeatedPtrField<rcr::PropertyWithName> *retVal,
        const rcr::DictionariesResponse *dictionaries
    );

    size_t setCards(
        odb::database *db,
        odb::transaction *t,
        uint64_t userId,
        const rcr::DictionariesResponse *dictionaries,
        const RCQuery *query,
        uint32_t componentFlags,
        size_t *sum
    );

    void add2log(
        odb::database *db,
        const std::string &operationSymbol,
        uint64_t userId,
        uint64_t packageId,
        int64_t qty
    );
};

#endif //RCR_RCQUERYPROCESSOR_H
