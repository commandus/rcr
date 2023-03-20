//
// Created by andrei on 20.03.23.
//

#ifndef RCR_BOXNAME_H
#define RCR_BOXNAME_H


#include <cstdint>
#include <string>

/**
 * Helper class to extract box numbers
 */
class BoxName {
public:
    static uint64_t extractFromFileName(
        const std::string &delimitedNumbers
    );
};

#endif //RCR_BOXNAME_H
