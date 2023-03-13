//
// Created by andrei on 13.03.23.
//

#include "RCQuery.h"

RCQuery::RCQuery(
    MEASURE_LOCALE aLocale,
    const std::string &aValue
)
{
    size_t p = 0;
    parse(aLocale, aValue, p);
}

int RCQuery::parse(
    MEASURE_LOCALE aLocale,
    const std::string &value,
    size_t &position
)
{
    return 0;
}
