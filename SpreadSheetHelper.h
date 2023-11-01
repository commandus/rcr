//
// Created by andrei on 20.03.23.
//

#ifndef RCR_SPREADSHEETHELPER_H
#define RCR_SPREADSHEETHELPER_H

#include <string>
#include <vector>
#include <map>
#include <xlnt/worksheet/worksheet.hpp>

#include "gen/rcr.pb.h"

class SheetRow {
public:
    char symbol;                            // 0- default
    int id;                                 // column A
    std::string name;                       // B
    uint64_t nominal;
    std::map <std::string, std::string> properties;   // B after ','
    int qty;                                // C
    std::string property_dip;               // D
    int property_v;                         // B
    std::string remarks;                    // E

    SheetRow() = default;
    void toCardRequest(
        const std::string &operation,
        const std::string &componentSymbol,
        uint64_t box,
        rcr::CardRequest &retval
    ) const;

};

class SpreadSheetHelper {
private:
    static char getSymbolFromSheetName(
        const xlnt::worksheet &worksheet
    );
    static bool parseR(
        SheetRow &retVal,
        const std::string &value
    );
    static bool parseC(
        SheetRow &retVal,
        const std::string &value
    );
    static bool parseL(
        SheetRow &retVal,
        const std::string &value
    );

public:
    explicit SpreadSheetHelper();
    explicit SpreadSheetHelper(
        const std::string &fileName,
        const std::string &symbol
    );
    explicit SpreadSheetHelper(
        const std::string &fileName,
        const std::string &content,
        const std::string &symbol
    );
    int loadFile(
        const std::string &fileName,
        const std::string &symbol
    );
    int loadString(
        const std::string &content,
        const std::string &symbol
    );
    static int loadCards(
        xlnt::workbook &book,
        const rcr::CardResponse &cards
    );
    static std::string toString(
        xlnt::workbook &book
    );
    // result
    std::vector <SheetRow> items;
    // statistics
    size_t total;   // total items count
    std::map<int, size_t> boxItemCount;

    std::string toJsonString();
};

#endif //RCR_SPREADSHEETHELPER_H
