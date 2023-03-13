//
// Created by andrei on 13.03.23.
//

#ifndef RCR_RCQUERY_H
#define RCR_RCQUERY_H

#include "MeasureUnit.h"

class RCQuery {
public:
    MEASURE_LOCALE locale;
    MEASURE measure;
    uint64_t nominal;
    std::string componentName;

    RCQuery() = default;
    RCQuery(const RCQuery &value) = default;
    RCQuery(MEASURE_LOCALE aLocale, const std::string &value);
protected:
    int parse(MEASURE_LOCALE aLocale, const std::string &value, size_t &position);
};


#endif //RCR_RCQUERY_H
