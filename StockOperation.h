//
// Created by andrei on 14.03.23.
//

#ifndef RCR_STOCKOPERATION_H
#define RCR_STOCKOPERATION_H

#include <string>

typedef enum {
    SO_NONE = 0,
    SO_LIST = 1,
    SO_COUNT = 2,
    SO_SUM = 3,
    SO_SET = 4,
    SO_ADD = 5,
    SO_SUB = 6,
    SO_RM = 7
} STOCK_OPERATION_CODE;

class StockOperation {
public:
    STOCK_OPERATION_CODE code;
    int boxBlocks;
    uint64_t boxes;
    size_t count;
    StockOperation(const std::string &value);
    StockOperation() = default;
    virtual ~StockOperation() = default;

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
    int parse(
        const std::string &value,
        size_t &position
    );

    /**
     * Parse box
     * box examples:
     *  "219", "219-1", "219-10-2", "221-1-2-3"
     * @param value
     * @param start
     * @param finish
     * @return blocks count: 0..4
     */
    int parseBoxes(const std::string &value, size_t start, size_t finish);

    /**
     * Parse command +1, -1, -1, sum, count, rm
     * @param value
     * @param start
     * @param finish
     * @return
     */
    STOCK_OPERATION_CODE parseCommand(
        const std::string &value,
        size_t start,
        size_t finish
    );
};


#endif //RCR_STOCKOPERATION_H