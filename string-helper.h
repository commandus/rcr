//
// Created by andrei on 14.03.23.
// @see https://stackoverflow.com/questions/3152241/case-insensitive-stdstring-find
//
#ifndef RCR_STRING_HELPER_H
#define RCR_STRING_HELPER_H

#include <string>
#include <google/protobuf/message.h>

#include "MeasureUnit.h"

std::string toUpperCase(const std::string &value);

std::string pb2JsonString(
    const google::protobuf::Message &message
);

#endif //RCR_STRING_HELPER_H
