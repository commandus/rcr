//
// Created by andrei on 13.03.23.
//

#include <sstream>
#include "RCQuery.h"

RCQuery::RCQuery(
    MEASURE_LOCALE aLocale,
    const std::string &aValue,
    COMPONENT defaultMeasure
)
{
    size_t p = 0;
    parse(aLocale, aValue, p, defaultMeasure);
}

RCQuery::RCQuery(
    const RCQuery &value
) {
    locale = value.locale;
    measure = value.measure;
    nominal = value.nominal;
    componentName = value.componentName;

    // properties
    properties = value.properties;

    // operation
    code = value.code;
    boxes = value.boxes;
    count = value.count;
    destinationBox = value.destinationBox;
}

int RCQuery::parse(
    MEASURE_LOCALE aLocale,
    const std::string &value,
    size_t &position,
    COMPONENT defaultComponent
)
{
    MeasureUnit::parse(aLocale, value, position, nominal, measure, componentName, defaultComponent);
    int r = QueryProperties::parse(value, position, properties);
    if (r)
        return r;
    r = StockOperation::parse(value, position, code, boxBlocks, boxes, count, destinationBox);
    if (r)
        return r;
    return r;
}

RCQuery::RCQuery(
    MEASURE_LOCALE locale,
    COMPONENT measure,
    uint64_t nominal,
    const std::string &componentName,
    const std::map<std::string, std::string> &properties,
    STOCK_OPERATION_CODE code,
    uint64_t boxes,
    size_t count
)
    : locale(locale), measure(measure), nominal(nominal),
    componentName(componentName), properties(properties), code(code),
    boxBlocks(boxBlocks), boxes(boxes), count(count)
{

}

std::string RCQuery::toString()
{
    std::stringstream ss;
    if (!componentName.empty()) {
        ss << componentName;
        if (measure)
            ss << " ";
    }
    if (nominal)
        ss << MeasureUnit::value(ML_RU, measure, nominal);

    for (auto p(properties.begin()); p != properties.end(); p++) {
        ss << " " << p->first << ":" << p->second;
    }
    if (boxes) {
        ss << " " << StockOperation::boxes2string(boxes);
    }

    switch (code) {
        case SO_COUNT:
            ss << " count";
            break;
        case SO_SUM:
            ss << " sum";
            break;
        case SO_SET:
            ss << "=" << count;
            break;
        case SO_ADD:
            ss << "+" << count;
            break;
        case SO_SUB:
            ss << "-" << count;
            break;
        case SO_RM:
            ss << " rm";
            break;
        default: // SO_NONE SO_LIST SO_LIST_NO_BOX:
            break;
    }
    return ss.str();
}

bool RCQuery::hasNominal() const {
    return MeasureUnit::hasNominal(measure);
}
