//
// Created by andrei on 13.03.23.
//

#ifndef RCR_RCQUERY_H
#define RCR_RCQUERY_H

#include "MeasureUnit.h"
#include "QueryProperties.h"
#include "StockOperation.h"

class RCQuery {
public:
    // MeasureUnit
    MEASURE_LOCALE locale;
    COMPONENT measure;
    uint64_t nominal;
    std::string componentName;

    // properties
    std::map<std::string, std::string> properties;

    // operation
    STOCK_OPERATION_CODE code;
    int boxBlocks;
    uint64_t boxes;
    size_t count;

    RCQuery() = default;

    RCQuery(
        MEASURE_LOCALE locale,
        COMPONENT measure,
        uint64_t nominal, const std::string &componentName,
        const std::map<std::string, std::string> &properties,
        STOCK_OPERATION_CODE code,
        uint64_t boxes,
        size_t count
    );

    RCQuery(const RCQuery &value) = default;
    RCQuery(
        MEASURE_LOCALE aLocale,
        const std::string &value,
        const COMPONENT defaultMeasure = COMPONENT_D
    );

    int parse(
        MEASURE_LOCALE aLocale,
        const std::string &value,
        size_t &position,
        COMPONENT defaultComponent = COMPONENT_D
    );

    std::string toString();
};

#endif //RCR_RCQUERY_H
