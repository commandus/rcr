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

int RCQuery::parse(
    MEASURE_LOCALE aLocale,
    const std::string &value,
    size_t &position,
    COMPONENT defaultComponent
)
{
    int r = MeasureUnit::parse(aLocale, value, position, nominal, measure, componentName, defaultComponent);
    if (r)
        return r;
    r = QueryProperties::parse(value, position, properties);
    if (r)
        return r;
    r = StockOperation::parse(value, position, code, boxBlocks, boxes, count);
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
    if (!componentName.empty())
        ss << componentName;
    else
        ss << MeasureUnit::value(ML_RU, measure, nominal);
    return ss.str();
}
