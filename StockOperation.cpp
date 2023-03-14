//
// Created by andrei on 14.03.23.
//

#include "StockOperation.h"

StockOperation::StockOperation(
    const std::string &value
)
{
    size_t position = 0;
    parse(value, position);
}

/**
 * Parse [box]<SEPARATOR>[cmd-value]
 *
 * box examples:
 *  "219", "219-1", "219-10-2", "221-1-2-3"
 *
 * [cmd][value] examples:
 *  "" -> SO_LIST
 *  "+1" -> SO_ADD 1
 *  "-1" -> SO_SUB 1
 *  "=1" -> SO_SET 1
 *  "sum" -> SO_SUM
 *  "count" -> SO_COUNT
 * @param value
 * @param position
 * @return 0- success
 */
int StockOperation::parse(
    const std::string &value,
    size_t &position
)
{
    // set default
    code = SO_LIST;
    count = 0;

    size_t start = position;
    size_t eolp = value.length();
    size_t finish = eolp;

    // skip spaces if exists
    for (auto p = start; p < eolp; p++) {
        if (!std::isspace(value[p])) {
            start = p;
            break;
        }
    }

    // try read boxes first
    for (auto p = start; p < eolp; p++) {
        if (!isdigit(value[p])) {
            finish = p;
            break;
        }
    }

    bool hasFirstPart = (finish > start);

    size_t startSecond = finish;
    size_t finishSecond = eolp;
    // skip spaces if exists
    for (auto p = startSecond; p < eolp; p++) {
        if (!std::isspace(value[p])) {
            startSecond = p;
            break;
        }
    }
    bool hasSecondPart = (finishSecond > startSecond);

    if (hasSecondPart) {
        boxBlocks = parseBoxes(value, start, finish);
        if (boxBlocks == 0) {
            return -1;
        }
        code = parseCommand(value, startSecond, finishSecond);
        if (code == SO_NONE)
            code = SO_LIST;
    } else {
        code = parseCommand(value, start, finish);
        // return SO_NONE- invalid command
        if (code == SO_NONE) {
            boxBlocks = parseBoxes(value, start, finish);
            if (boxBlocks == 0) {
                // invalid boxes, invalid command
                return -2;
            } else
                code = SO_LIST;
        } else {
            boxBlocks = 0;
            boxes = 0;
            if (code == SO_COUNT || code == SO_SUM || code == SO_RM) {
                // list, sum, count, rm are valid commands without boxes
            } else {
                code = SO_NONE; // invalid command
            }
        }
    }
    return 0;
}

/**
 * Parse box
 * box examples:
 *  "219", "219-1", "219-10-2", "221-1-2-3"
 * @param value
 * @param start
 * @param finish
 * @return blocks count: 0..4
 */
int StockOperation::parseBoxes(
    const std::string &value,
    size_t start,
    size_t finish
)
{
    boxes = 0;
    // skip spaces if exists
    size_t s = start;

    int blocks = 0;
    for(int block = 0; block < 4; block++) {
        size_t f = finish;
        // skip separator(s)
        for (auto p = s; p < f; p++) {
            if (std::isdigit(value[p])) {
                s = p;
                break;
            }
        }
        // find out end of the number block
        for (auto p = s; p < f; p++) {
            if (!std::isdigit(value[p])) {
                f = p;
                break;
            }
        }
        // nothing found
        if (f <= s)
            break;
        // has box number, try to read
        try {
            uint64_t b = std::stoull(value.substr(s, f - s));
            boxes |= (b & 0xffff) << ((3 - block) * 16);
            blocks++;
        } catch (std::exception &e) {

        }
        s = f;
    }
    return blocks;
}

/**
 * Parse command +1, -1, =1, sum, count, rm
 * @param value
 * @param start
 * @param finish
 * @return
 */
STOCK_OPERATION_CODE StockOperation::parseCommand(
    const std::string &value,
    size_t start,
    size_t finish
)
{
    if (finish <= start)
        return SO_NONE;

    STOCK_OPERATION_CODE r;
    switch(value[start]) {
        case '+':
            r = SO_ADD;
            break;
        case '-':
            r = SO_SUB;
            break;
        case '=':
            r = SO_SET;
            break;
        case 'C':
        case 'c':
            r = SO_COUNT;
            break;
        case 'R':
        case 'r':
            r = SO_RM;
            break;
        case 'S':
        case 's':
            r = SO_SUM;
            break;
        default:
            r = SO_NONE;
    }
    if (r >= SO_SET && r <= SO_SUB) {
        // try to read number
        start++;    // command takes 1 byte
        try {
            count = std::stoull(value.substr(start, finish - start));
        } catch (std::exception &e) {
            count = 0;
            r = SO_NONE;    // invalid command
        }
    }
    return r;
}
