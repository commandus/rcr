//
// Created by andrei on 14.03.23.
//

#include <sstream>
#include <limits>
#include <iostream>
#include "StockOperation.h"
#include "platform.h"

StockOperation::StockOperation(
    const std::string &value
)
{
    size_t position = 0;
    parseString(value, position);
}

int StockOperation::parse(
    const std::string &value,
    size_t &position,
    STOCK_OPERATION_CODE &retCode,
    int &retBoxBlocks,
    uint64_t &retBoxes,
    // operation count
    size_t &retCount,
    // operation destination
    uint64_t &retDestinationBox
)
{
    // set default
    retCode = SO_LIST;
    retCount = 0;

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

    // try read retBoxes first
    for (auto p = start; p < eolp; p++) {
        if (!(isdigit(value[p]) || (std::ispunct(value[p])))) {
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
        if (hasFirstPart) { // has boxes
            retBoxBlocks = parseBoxes(retBoxes, value, start, finish);
            if (retBoxBlocks == 0) {
                return -1;
            }
        } else {
            retBoxBlocks = 0;
            retBoxes = 0;
        }
        retCode = parseCommand(retCount, retDestinationBox, value, startSecond, finishSecond);
        if (retCode == SO_NONE)
            retCode = SO_LIST;
    } else {
        // no boxes specified
        retCode = parseCommand(retCount, retDestinationBox, value, start, finish);
        // return SO_NONE- invalid command
        if (retCode == SO_NONE) {
            retBoxBlocks = parseBoxes(retBoxes, value, start, finish);
            if (retBoxBlocks == 0) {
                // invalid retBoxes, invalid command
                retCode = SO_LIST_NO_BOX;
            } else
                retCode = SO_LIST;
        } else {
            retBoxBlocks = 0;
            retBoxes = 0;
            if (retCode == SO_COUNT || retCode == SO_SUM || retCode == SO_RM) {
                // list, sum, rerCount, rm are valid commands without retBoxes
            } else {
                retCode = SO_NONE; // invalid command
            }
        }
    }
    return 0;
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
int StockOperation::parseString(
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
        if (!(isdigit(value[p]) || (std::ispunct(value[p])))) {
            finish = p;
            break;
        }
    }

    // bool hasFirstPart = (finish > start);

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
        boxBlocks = parseBoxes(boxes, value, start, finish);
        if (boxBlocks == 0) {
            return -1;
        }
        code = parseCommand(count, destinationBox, value, startSecond, finishSecond);
        if (code == SO_NONE)
            code = SO_LIST;
    } else {
        code = parseCommand(count, destinationBox,value, start, finish);
        // return SO_NONE- invalid command
        if (code == SO_NONE) {
            boxBlocks = parseBoxes(boxes, value, start, finish);
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
    uint64_t &retBoxes,
    const std::string &value,
    size_t start,
    size_t finish
)
{
    retBoxes = 0;
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
            retBoxes |= (b & 0xffff) << ((3 - block) * 16);
            blocks++;
        } catch (std::exception &) {

        }
        s = f;
    }
    return blocks;
}

uint64_t StockOperation::parseBoxes(
    const std::string &value
)
{
    uint64_t r;
    parseBoxes(r, value, 0, value.size());
    return r;
}

/**
 * Parse command +1, -1, =1, sum, count, rm
 * @param value
 * @param start
 * @param eolp
 * @return
 */
STOCK_OPERATION_CODE StockOperation::parseCommand(
    size_t &retCount,
    uint64_t &retDestinationBox,
    const std::string &value,
    size_t start,
    size_t eolp
)
{
    if (eolp <= start)
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
        case '/':
            r = SO_MOV;
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
    if (r >= SO_SET && r <= SO_MOV) {
        // try to read number
        start++;    // command takes 1 byte
        size_t finish = eolp;
        try {
            // skip separator(s)
            for (auto p = start; p < finish; p++) {
                if (std::isdigit(value[p])) {
                    start = p;
                    break;
                }
            }
            // find out end of the number block
            for (auto p = start; p < finish; p++) {
                if (!std::isdigit(value[p])) {
                    finish = p;
                    break;
                }
            }
            retCount = std::stoull(value.substr(start, finish - start));
            start = finish;
            if (r == SO_MOV) {
                // try get destination box
                int blocks = parseBoxes(retDestinationBox, value, start, eolp);
                if (blocks == 0)
                    r = SO_NONE;    // invalid command
            }
        } catch (std::exception&) {
            retCount = 0;
            r = SO_NONE;    // invalid command
        }
    }
    return r;
}

/**
 * Return count of box depth 4..1 (0 if no one)
 * @param boxes if boxes == 0 return 0
 * @return
 */
static inline int getBoxDepth(uint64_t boxes)
{
    int boxCnt = 0;
    if (boxes & 0xffff)
        boxCnt = 4;
    else
    if (boxes & 0xffff0000)
        boxCnt = 3;
    else
    if (boxes & 0xffff00000000)
        boxCnt = 2;
    else
    if (boxes & 0xffff000000000000)
        boxCnt = 1;
    return boxCnt;
}

std::string StockOperation::boxes2string(
    uint64_t boxes
)
{
    std::stringstream ss;
    int shift = 6 * 8;
    ss << (boxes >> shift);
    for (int i = getBoxDepth(boxes); i > 1; i--) {
        shift -= 16;
        ss << '-' << ((boxes >> shift) & 0xffff);
    }
    return ss.str();
}

uint64_t StockOperation::maxBox(
    uint64_t minBox,
    int &depth
)
{
    depth = getBoxDepth(minBox);    // 0..4
    switch (depth) {
        case 1:
            return minBox | 0xffffffffffff;
        case 2:
            return minBox | 0xffffffff;
        case 3:
            return minBox | 0xffff;
        case 4:
            return minBox;
        default:    // 0
            return std::numeric_limits<uint64_t>::max();
    }
}

uint64_t StockOperation::renameBox(
    uint64_t src,
    uint64_t srcRoot,
    uint64_t destRoot
)
{
    int srcBoxDepth = getBoxDepth(srcRoot);
    int destBoxDepth = getBoxDepth(destRoot);
    // remove previous root by shift
    uint64_t r = src << (srcBoxDepth * 16);
    // add a new root
    return destRoot | (r >> (destBoxDepth * 16));
}

/**
 * "Put" box in outer boxes
 * @param boxes outer boxes
 * @param box box inside outer boxes
 * @return boxes identifier
 */
uint64_t StockOperation::boxAppendBox(
    uint64_t boxes,
    int box
)
{
    box &= 0xffff;
    int boxCnt = 3;
    if (boxes & 0xffff)
        return boxes;   // no room
    else
    if (boxes & 0xffff0000)
        boxCnt = 0;
    else
    if (boxes & 0xffff00000000)
        boxCnt = 1;
    else
    if (boxes & 0xffff000000000000)
        boxCnt = 2;
    return boxes | ((uint64_t) box << (boxCnt * 16));
}

/**
 * Return true- innerBox is outerBox or innerBox is in outerBox
 * @param innerBox box id
 * @param outerBox box id
 * @return true- innerBox is outerBox or innerBox is in outerBox
 */
bool StockOperation::isBoxInBoxes(
    uint64_t innerBox,
    uint64_t outerBox
) {
    if (!outerBox)
        return true;
    uint64_t out4 = outerBox & 0xffff000000000000;
    uint64_t out3 = outerBox & 0x0000ffff00000000;
    uint64_t out2 = outerBox & 0x00000000ffff0000;
    uint64_t out1 = outerBox & 0x000000000000ffff;
    return
        (out4 == 0 || ((innerBox & 0xffff000000000000) == out4))
        &&
        (out3 == 0 || ((innerBox & 0x0000ffff00000000) == out3))
        &&
        (out2 == 0 || ((innerBox & 0x00000000ffff0000) == out2))
        &&
        (out1 == 0 || ((innerBox & 0x000000000000ffff) == out1));
}


int StockOperation::box2Array(
    BoxArray& retVal,
    uint64_t boxId
)
{
    // retVal.a[0] - outer box, retVal.a[3]- inner box
#if BYTE_ORDER == BIG_ENDIAN
    retVal.a[0] = ((BoxArray*)&boxId)->a[3];
    retVal.a[1] = ((BoxArray*)&boxId)->a[2];
    retVal.a[2] = ((BoxArray*)&boxId)->a[1];
    retVal.a[3] = ((BoxArray*)&boxId)->a[0];
#else
    retVal.b = boxId;
#endif
    if (retVal.a[0] == 0)
        return 0;
    if (retVal.a[1] == 0)
        return 1;
    if (retVal.a[2] == 0)
        return 2;
    if (retVal.a[3] == 0)
        return 3;
    return 4;
}

std::string StockOperation::toString()
{
    std::stringstream ss;
    ss << boxes2string(boxes);
    switch (code) {
        case SO_COUNT:
            ss << " count";
            break;
        case SO_SUM:
            ss << " sum";
            break;
        case SO_SET:
            ss << "=" << count;
            break;
        case SO_ADD:
            ss << "+" << count;
            break;
        case SO_SUB:
            ss << "-" << count;
            break;
        case SO_RM:
            ss << " rm";
            break;
        default:
            break;
    }
    return ss.str();
}

uint64_t StockOperation::lastBox(
    const uint64_t &box
) {
    if (box & 0xffff)
        return box;
    else
    if (box & 0xffff0000)
        return box | 0xffff;
    else
    if (box & 0xffff00000000)
        return box | 0xffffffff;
    else
    if (box & 0xffff000000000000)
        return box | 0xffffffffffff;
    return 0xffffffffffffffff;
}
