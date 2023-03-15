//
// Created by andrei on 14.03.23.
//

#ifndef RCR_STOCKOPERATION_H
#define RCR_STOCKOPERATION_H

#include <string>

typedef enum {
    SO_NONE = 0,
    SO_LIST = 1,
    SO_LIST_NO_BOX = 2,
    SO_COUNT = 3,
    SO_SUM = 4,
    SO_SET = 5,
    SO_ADD = 6,
    SO_SUB = 7,
    SO_RM = 8
} STOCK_OPERATION_CODE;

class StockOperation {
protected:
    /**
     * Parse box
     * box examples:
     *  "219", "219-1", "219-10-2", "221-1-2-3"
     * @param value
     * @param start
     * @param finish
     * @return blocks count: 0..4
     */
    static int parseBoxes(
        uint64_t &retBoxes,
        const std::string &value,
        size_t start,
        size_t finish
    );

    /**
     * Parse command +1, -1, -1, sum, count, rm
     * @param value
     * @param start
     * @param finish
     * @return
     */
    static STOCK_OPERATION_CODE parseCommand(
        size_t &retCount,
        const std::string &value,
        size_t start,
        size_t finish
    );

public:
    STOCK_OPERATION_CODE code;
    int boxBlocks;
    uint64_t boxes;
    size_t count;

    StockOperation(const std::string &value);
    StockOperation() = default;
    virtual ~StockOperation() = default;

    static int parse(
        const std::string &value,
        size_t &position,
        STOCK_OPERATION_CODE &code,
        int &boxBlocks,
        uint64_t &boxes,
        size_t &count
    );
    /**
     * Parse [box]<SEPARATOR>[cmd][value]
     *
     * bix examples:
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
    int parseString(
        const std::string &value,
        size_t &position
    );

};


#endif //RCR_STOCKOPERATION_H
