//
// Created by andrei on 15.03.23.
//

#include <sstream>
#include "QueryProperties.h"

int QueryProperties::parse(
    const std::string &value,
    size_t &position,
    std::map<std::string, std::string> &retKeyValues
)
{
    size_t start = position;
    size_t eolp = value.length();
    size_t finish = eolp;

    size_t saveStart;
    while (true) {
        // skip spaces if exists
        for (auto p = start; p < eolp; p++) {
            if (!std::isspace(value[p])) {
                start = p;
                break;
            }
        }
        saveStart = position;
        bool kvFound = false;
        // try find out ':'
        for (auto p = start; p < eolp; p++) {
            if (value[p] == ':') {
                finish = p;
                kvFound = true;
                break;
            }
        }
        if (!kvFound) {
            position = saveStart;
            break;
        }
        // skip spaces before ':' if exists
        size_t finishKey = finish;
        for (auto p = finish - 1; p > start; p--) {
            if (!std::isspace(value[p])) {
                finish = p + 1;
                break;
            }
        }

        std::string key = value.substr(start, finish - start);
        // find out end of value
        start = finishKey + 1;
        finish = eolp;

        // skip spaces if exists
        for (auto p = start; p < eolp; p++) {
            if (!std::isspace(value[p])) {
                start = p;
                break;
            }
        }
        // find out value end by space
        for (auto p = start; p < eolp; p++) {
            if (std::isspace(value[p])) {
                finish = p;
                break;
            }
        }
        std::string v = value.substr(start, finish - start);
        retKeyValues[key] = v;
        start = finish;
        position = finish;
    }
    return 0;
}

std::string QueryProperties::toString(
    const std::map<std::string, std::string> &value
) {
    std::stringstream ss;
    bool first = true;
    for (auto mit = value.begin(); mit != value.end(); mit++) {
        if (first)
            first = false;
        else
            ss << " ";
        ss << mit->first << ":" << mit->second;
    }
    return ss.str();
}
