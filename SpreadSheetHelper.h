//
// Created by andrei on 20.03.23.
//

#ifndef RCR_SPREADSHEETHELPER_H
#define RCR_SPREADSHEETHELPER_H


#include <string>
#include <vector>
#include <map>

class SheetRow {
public:
    int id;
    std::string name;
    std::vector <std::string> properties;
    int qty;
    std::string remarks;
};

class SpreadSheetHelper {
private:
    std::string fileName;
public:
    explicit SpreadSheetHelper(const std::string &fileName, uint64_t box);
    int load(const std::string &fileName, uint64_t box);
    std::map<int, int> boxItemCount;
    std::vector <SheetRow> items;
};

#endif //RCR_SPREADSHEETHELPER_H
