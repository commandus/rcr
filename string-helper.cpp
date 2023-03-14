//
// Created by andrei on 14.03.23.
//
#include "string-helper.h"

#include <unicode/unistr.h>

std::string toUpperCase(
    MEASURE_LOCALE locale,
    const std::string &value
)
{
    std::string r;
    icu::UnicodeString::fromUTF8(value).toUpper().toUTF8String(r);
    return r;
}
