//
// Created by andrei on 15.03.23.
//

#ifndef RCR_QUERYPROPERTIES_H
#define RCR_QUERYPROPERTIES_H

#include <string>
#include <map>

class QueryProperties {
public:
    static int parse(
        const std::string &value,
        size_t &position,
        std::map<std::string, std::string> &retKeyValues
    );

    static std::string toString(
        const std::map<std::string, std::string> &value
    );
};

#endif //RCR_QUERYPROPERTIES_H
