//
// Created by andrei on 14.03.23.
//
#include "string-helper.h"

#include <unicode/unistr.h>
#include <google/protobuf/util/json_util.h>

std::string toUpperCase(const std::string &value)
{
    std::string r;
    icu::UnicodeString::fromUTF8(value).toUpper().toUTF8String(r);
    return r;
}

std::string pb2JsonString(
        const google::protobuf::Message &message
)
{
    google::protobuf::util::JsonPrintOptions formattingOptions;
    formattingOptions.add_whitespace = true;
    formattingOptions.always_print_primitive_fields = true;
    formattingOptions.preserve_proto_field_names = true;
    std::string r;
    google::protobuf::util::MessageToJsonString(message, &r, formattingOptions);
    return r;
}
