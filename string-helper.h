//
// Created by andrei on 14.03.23.
// @see https://stackoverflow.com/questions/3152241/case-insensitive-stdstring-find
//
#ifndef RCR_STRING_HELPER_H
#define RCR_STRING_HELPER_H

#include <string>
#include <vector>
#include <google/protobuf/message.h>

#include "MeasureUnit.h"

std::string toUpperCase(const std::string &value);

std::vector<std::string> split(
    const std::string &s,
    char delim
);

std::string pb2JsonString(
    const google::protobuf::Message &message
);

std::string nextWord(
    const std::string &line,
    size_t &start
);

std::string nextNumber(
    const std::string &line,
    size_t &start
);

std::string remainText(
    const std::string &line,
    size_t &start
);

std::string dateStamp(time_t value);

#endif //RCR_STRING_HELPER_H
