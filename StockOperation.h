//
// Created by andrei on 14.03.23.
//

#ifndef RCR_STOCKOPERATION_H
#define RCR_STOCKOPERATION_H

#include <string>
#include <cinttypes>

typedef struct {
    union {
        uint16_t a[4];
        uint64_t b;
    };
} BoxArray;

typedef enum {
    SO_NONE = 0,
    SO_LIST = 1,
    SO_LIST_NO_BOX = 2,
    SO_COUNT = 3,
    SO_SUM = 4,
    SO_SET = 5,
    SO_ADD = 6,
    SO_SUB = 7,
    SO_MOV = 8,
    SO_RM = 9
} STOCK_OPERATION_CODE;

class StockOperation {
protected:
    /**
     * Parse command +1, -1, -1, sum, count, rm
     * @param value
     * @param start
     * @param eolp
     * @return
     */
    static STOCK_OPERATION_CODE parseCommand(
        size_t &retCount,
        uint64_t &retDestinationBox,
        const std::string &value,
        size_t start,
        size_t eolp
    );

public:
    STOCK_OPERATION_CODE code;
    int boxBlocks;
    uint64_t boxes;
    size_t count;
    uint64_t destinationBox;

    StockOperation(const std::string &value);
    StockOperation() = default;
    virtual ~StockOperation() = default;

    static int parse(
        const std::string &value,
        size_t &position,
        STOCK_OPERATION_CODE &code,
        int &boxBlocks,
        uint64_t &boxes,
        // operation count
        size_t &count,
        // operation destination
        uint64_t &destinationBox
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
     *  "/1" -> SO_MOV 1
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
    static uint64_t parseBoxes(
        const std::string &value
    );

    static uint64_t boxAppendBox(
        uint64_t boxes,
        int box
    );
    static uint64_t renameBox(
            uint64_t src,
            uint64_t srcRoot,
            uint64_t destRoot
    );
    static std::string boxes2string(uint64_t boxes);
    /**
     * @param minBox
     * @param depth Return 0 or 1..4
     * @return
     */
    static uint64_t maxBox(
        uint64_t minBox,
        int &depth
    );

    static bool isBoxInBoxes(uint64_t innerBox, uint64_t outerBox);

    static int box2Array(BoxArray& retVal, uint64_t boxId);

    std::string toString();

    /**
     * Return last nested box in the box
     * @param box forst box
     * @return last nested box in the box
     */
    static uint64_t lastBox(const uint64_t &box);
};

#endif //RCR_STOCKOPERATION_H
