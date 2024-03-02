//
// Created by andrei on 14.03.23.
//
#include "string-helper.h"

#include <sstream>
#include <unicode/unistr.h>
#include <google/protobuf/util/json_util.h>

#ifdef _MSC_VER
 #include <windows.h>
#endif

std::string toUpperCase(const std::string &value)
{
#ifdef _MSC_VER
    std::string r = value;
    CharUpperA((LPSTR) r.c_str());
#else
    std::string r;
    icu::UnicodeString::fromUTF8(value).toUpper().toUTF8String(r);
#endif        
    if (r.empty())
        return value;
    else
        return r;    
}

//
// Split strings
// https://stackoverflow.com/questions/236129/how-do-i-iterate-over-the-words-of-a-string
//
template <typename Out>
void split(const std::string &s, char delim, Out result) {
    std::istringstream iss(s);
    std::string item;
    while (std::getline(iss, item, delim)) {
        size_t sz = item.size();
        if (sz == 0 || (sz == 1 && item[0] == delim))
            continue;
        *result++ = item;
    }
}

std::vector<std::string> split(
    const std::string &s,
    char delim
)
{
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
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

std::string nextWord(
    const std::string &line,
    size_t &start
) {
    size_t eolp = line.size();
    size_t finish = eolp;
    // skip spaces
    for (auto p = start; p < eolp; p++) {
        if (!std::isspace(line[p])) {
            start = p;
            break;
        }
    }
    // try read word
    for (auto p = start; p < eolp; p++) {
        if (std::isspace(line[p])) {
            finish = p;
            break;
        }
    }
    std::string r = line.substr(start, finish - start);
    start = finish;
    return r;
}

std::string nextNumber(
    const std::string &line,
    size_t &start
) {
    size_t eolp = line.size();
    size_t finish = eolp;
    // skip spaces
    for (auto p = start; p < eolp; p++) {
        if (!std::isspace(line[p])) {
            start = p;
            break;
        }
    }
    // try read digits
    for (auto p = start; p < eolp; p++) {
        if (!std::isdigit(line[p])) {
            finish = p;
            break;
        }
    }
    std::string r = line.substr(start, finish - start);
    start = finish;
    return r;
}

std::string remainText(
    const std::string &line,
    size_t &start
) {
    size_t eolp = line.size();
    size_t finish = eolp;
    // skip spaces
    for (auto p = start; p < eolp; p++) {
        if (!std::isspace(line[p])) {
            start = p;
            break;
        }
    }
    // try read to the end
    std::string r = line.substr(start, eolp - start);
    start = finish;
    return r;
}

std::string dateStamp(
    time_t value
) {
    char buffer[80];
    struct tm *lt = localtime(&value);
    strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", lt);
    return std::string(buffer);
}
