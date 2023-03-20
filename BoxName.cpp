//
// Created by andrei on 20.03.23.
//

#include "BoxName.h"
#include "StockOperation.h"

uint64_t BoxName::extractFromFileName(
    const std::string &delimitedNumbers
)
{
    uint64_t r;
    StockOperation::parseBoxes(r, delimitedNumbers, 0, delimitedNumbers.size());
    return r;
}
