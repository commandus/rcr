//
// Created by andrei on 20.03.23.
//

#include "SpreadSheetHelper.h"

#include <xlnt/xlnt.hpp>

SpreadSheetHelper::SpreadSheetHelper(
    const std::string &aFileName,
    uint64_t box
)
{
    int r = load(aFileName, box);
}

/**
 * A    B        C  D
 * -    name     qty remark
 *      SSH6N70  10	 KOREA 810
 * @param aFileName
 * @return
 */
int SpreadSheetHelper::load(
    const std::string &aFileName,
    uint64_t box
) {
    xlnt::workbook book;
    book.load(aFileName);
    int boxId = 1;
    for (size_t i = 0; i < book.sheet_count(); i++) {
        xlnt::worksheet wsheet = book.sheet_by_index(i);
        for (auto row : wsheet.rows()) {
            SheetRow sr;
            sr.id = boxId;
            for (auto cell: row) {
                auto col = cell.column().index;
                switch (col) {
                    case 1:
                        // id
                        if (cell.has_value()) {
                            boxId = std::strtoull(cell.to_string().c_str(), nullptr, 10);
                            sr.id = boxId;
                        }
                        break;
                    case 2:
                        // name and properties
                        sr.name = cell.to_string();
                        break;
                    case 3:
                        // qty
                        if (!cell.has_value())
                            continue;
                        sr.qty = std::strtoull(cell.to_string().c_str(), nullptr, 10);
                        break;
                    case 4:
                        // remarks
                        sr.remarks = cell.to_string();
                        break;
                    default:
                        continue;
                }
            }
            items.push_back(sr);
            // update statistic counters
            boxItemCount[sr.id] += sr.qty;
        }
    }
    return 0;
}
