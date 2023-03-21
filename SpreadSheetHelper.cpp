//
// Created by andrei on 20.03.23.
//

#include "SpreadSheetHelper.h"

#include <xlnt/xlnt.hpp>

#include "utilstring.h"

void SheetRow::toCard(
    rcr::Card &retval
) const
{
    retval.set_id(id);
    retval.set_component_id(id);
    retval.set_name(name);
    retval.set_nominal(0);
    for (std::vector <std::string>::const_iterator it = properties.begin(); it != properties.end(); it++) {
        rcr::Property *prop = retval.mutable_properties()->Add();
        prop->set_id(0);
        prop->set_property_type_id(0);
        prop->set_value(*it);
    }
}

SpreadSheetHelper::SpreadSheetHelper(
    const std::string &aFileName,
    uint64_t box
)
    : total(0)
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
            // id
            if (row[0].has_value()) {
                if (row[0].data_type() != xlnt::cell_type::number)
                    continue;
                boxId = std::strtoull(row[0].to_string().c_str(), nullptr, 10);
                sr.id = boxId;
            }
            // name and properties
            if (!row[1].has_value())
               continue;
            sr.name = row[1].to_string();
            sr.name = trim(sr.name);
            // qty
            if ((!row[2].has_value()) || (row[2].data_type() != xlnt::cell_type::number))
                continue;
            sr.qty = std::strtoull(row[2].to_string().c_str(), nullptr, 10);
            if (sr.qty <= 0)
                continue;
            // property: case
            sr.property_dip = row[3].to_string();
            sr.property_dip = trim(sr.property_dip);
            // remarks
            sr.remarks = row[4].to_string();
            sr.remarks = trim(sr.remarks);
            items.push_back(sr);
            // update statistic counters
            total += sr.qty;
            boxItemCount[sr.id] += sr.qty;
        }
    }
    return 0;
}
