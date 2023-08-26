//
// Created by andrei on 20.03.23.
//

#include "SpreadSheetHelper.h"

#include <xlnt/xlnt.hpp>
// i18n
#include <libintl.h>
#define _(String) gettext (String)

#include "utilstring.h"
#include "string-helper.h"
#include "StockOperation.h"
#include "QueryProperties.h"

void SheetRow::toCardRequest(
    const std::string &operation,
    const std::string &defaultComponentSymbol,
    uint64_t box,
    rcr::CardRequest &retval
) const
{
    retval.set_operation_symbol(operation);
    if (symbol != 0) {
        retval.set_symbol_name(std::string(1, symbol));
    } else
        retval.set_symbol_name(defaultComponentSymbol);
    retval.set_name(name);
    retval.set_nominal(nominal);
    retval.set_qty(qty);
    retval.set_box(StockOperation::boxAppendBox(box, id));

    if (!property_dip.empty()) {
        rcr::PropertyRequest *prop = retval.mutable_properties()->Add();
        prop->set_property_type_name("K");
        prop->set_value(property_dip);
    }
    if (!property_v != 0) {
        rcr::PropertyRequest *prop = retval.mutable_properties()->Add();
        prop->set_property_type_name("V");
        prop->set_value(std::to_string(property_v));
    }

    for (std::map <std::string, std::string>::const_iterator it = properties.begin(); it != properties.end(); it++) {
        rcr::PropertyRequest *prop = retval.mutable_properties()->Add();
        prop->set_property_type_name(it->first);
        prop->set_value(it->second);
    }
}

SpreadSheetHelper::SpreadSheetHelper()
    : total(0)
{

}

SpreadSheetHelper::SpreadSheetHelper(
    const std::string &aFileName
)
    : total(0)
{
    int r = loadFile(aFileName);
}

SpreadSheetHelper::SpreadSheetHelper(
    const std::string &aFileName,
    const std::string &content
)
    : total(0)
{
    int r = loadString(content);
}

/**
 * A    B        C  D
 * -    name     qty remark
 *      SSH6N70  10	 KOREA 810
 * @param fileName
 * @return
 */
int SpreadSheetHelper::loadFile(
    const std::string &fileName
) {
    xlnt::workbook book;
    book.load(fileName);
    int boxId = 1;
    for (size_t i = 0; i < book.sheet_count(); i++) {
        xlnt::worksheet wsheet = book.sheet_by_index(i);
        std::string uSheetName =  toUpperCase(wsheet.title());
        char symb = getSymbolFromSheetName(wsheet);
        for (auto row : wsheet.rows()) {
            SheetRow sr;
            sr.symbol = symb;
            sr.id = boxId;
            // id
            if (row[0].has_value()) {
                if (row[0].data_type() != xlnt::cell_type::number)
                    continue;
                boxId = std::strtol(row[0].to_string().c_str(), nullptr, 10);
                sr.id = boxId;
            }
            // name and properties or nominal and voltage
            if (!row[1].has_value())
               continue;
            switch (sr.symbol) {
                case 'C':
                    if (!parseC(sr, row[1].to_string()))
                        continue;
                    break;
                case 'L':
                    if (!parseL(sr, row[1].to_string()))
                        continue;
                    break;
                case 'R':
                    if (!parseR(sr, row[1].to_string()))
                        continue;
                    break;
                default:
                    sr.name = row[1].to_string();
                    sr.name = trim(sr.name);
                    sr.nominal = 0;
            }
            // qty
            if ((!row[2].has_value()) || (row[2].data_type() != xlnt::cell_type::number))
                continue;
            sr.qty = std::strtol(row[2].to_string().c_str(), nullptr, 10);
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

/**
 * @param content
 * @return
 */
int SpreadSheetHelper::loadString(
    const std::string &content
) {
    xlnt::workbook book;
    std::vector<uint8_t> cont(content.begin(), content.end());
    book.load(cont);
    int boxId = 1;
    for (size_t i = 0; i < book.sheet_count(); i++) {
        xlnt::worksheet wsheet = book.sheet_by_index(i);
        char symb = getSymbolFromSheetName(wsheet);
        for (auto row : wsheet.rows()) {
            bool isNewFormat = false;
            SheetRow sr;
            sr.symbol = symb;
            // use previous box if not specified
            sr.id = boxId;
            if (row[0].has_value()) {
                std::string v = row[0].to_string();
                if (v.find('-') != std::string::npos) {
                    // new format contain full box path
                    boxId = StockOperation::parseBoxes(v);
                    isNewFormat = true;
                } else {
                    // old format has last box number
                    if (row[0].data_type() != xlnt::cell_type::number)
                        continue;
                    boxId = std::strtol(v.c_str(), nullptr, 10);
                }
                sr.id = boxId;
            }
            // name and properties
            if (!row[1].has_value())
                continue;
            switch (sr.symbol) {
                case 'C':
                    if (!parseC(sr, row[1].to_string()))
                        continue;
                    break;
                case 'R':
                    if (!parseR(sr, row[1].to_string()))
                        continue;
                    break;
                default:
                    sr.name = row[1].to_string();
                    sr.name = trim(sr.name);
                    sr.nominal = 0;
            }
            // qty
            if ((!row[2].has_value()) || (row[2].data_type() != xlnt::cell_type::number))
                continue;
            sr.qty = std::strtol(row[2].to_string().c_str(), nullptr, 10);
            if (sr.qty <= 0)
                continue;

            if (isNewFormat) {
                size_t ofs = 0;
                QueryProperties::parse(row[4].to_string(), ofs, sr.properties);
                sr.property_dip = "";
                sr.remarks = "";
            } else {
                // property: case
                sr.property_dip = row[3].to_string();
                sr.property_dip = trim(sr.property_dip);
                // remarks
                sr.remarks = row[4].to_string();
                sr.remarks = trim(sr.remarks);
            }

            items.push_back(sr);
            // update statistic counters
            total += sr.qty;
            boxItemCount[sr.id] += sr.qty;
        }
    }
    return 0;
}

int SpreadSheetHelper::loadCards(
    xlnt::workbook &book,
    const rcr::CardResponse &cards
)
{
    xlnt::worksheet wsheets[26];
    int lastRow[26];

    // create component sheets
    for (int i = 0; i < 26; i++) {
        wsheets[i] = book.create_sheet();
        char t = 'A' + i;
        std::string dsc = std::string(&t, 1) + " - " + MeasureUnit::description(ML_RU, (COMPONENT) i);
        wsheets[i].title(dsc);

        lastRow[i] = 2;
        wsheets[i].cell("A1").value(_("Box"));
        wsheets[i].cell("B1").value(_("Name"));
        wsheets[i].cell("C1").value(_("Quantity"));
        wsheets[i].cell("D1").value(_("Label"));
        wsheets[i].cell("E1").value(_("Card #"));
        wsheets[i].cell("F1").value(_("Package #"));
    }
    // delete first sheet
    book.remove_sheet(book.active_sheet());

    for (int cardNo = 0; cardNo < cards.cards_size(); cardNo++) {
        rcr::CardNPropetiesPackages c = cards.cards(cardNo);
        auto componentIdx = c.card().symbol_id();
        if (componentIdx > 0)
            componentIdx--;
        xlnt::worksheet &w = wsheets[componentIdx];
        auto lr = lastRow[componentIdx];
        for (int p = 0; p < c.packages_size(); p++) {
            auto pack = c.packages(p);
            w.cell(xlnt::cell_reference(1, lr)).value(StockOperation::boxes2string(pack.box())); // A box
            w.cell(xlnt::cell_reference(2, lr)).value(c.card().name());    // B
            w.cell(xlnt::cell_reference(3, lr)).value(std::to_string(pack.qty())); // C
            std::stringstream ss;
            for (int pr = 0; pr < c.properties_size(); pr++) {
                ss << c.properties(pr).property_type() << ": " << c.properties(pr).value() << " ";
            }
            w.cell(xlnt::cell_reference(4, lr)).value(ss.str()); // D
            w.cell(xlnt::cell_reference(5, lr)).value(std::to_string(c.card().id()));   // E
            w.cell(xlnt::cell_reference(6, lr)).value(std::to_string(pack.id())); // F
        }
        lastRow[componentIdx] = lr + 1;
    }

    // freeze top row and left column
    for (int i = 0; i < 26; i++) {
        wsheets[i].freeze_panes("B2");
    }

    // find out first filled worksheet
    int firstSheet = 0;
    for (int i = 0; i < 26; i++) {
        if (lastRow[i] > 2) {
            firstSheet = i;
            break;
        }
    }
    // set it active
#ifndef _WINDOWS
    book.active_sheet(firstSheet);
#endif
    return 0;
}

std::string SpreadSheetHelper::toString(
    xlnt::workbook &book
)
{
    std::stringstream strm;
    book.save(strm);
    return strm.str();
}

char SpreadSheetHelper::getSymbolFromSheetName(
    const xlnt::worksheet &wsheet
) {
    std::string uSheetName = toUpperCase(wsheet.title());
    char r = 0;
    if (uSheetName.length() == 0)
        return r;
    char c = uSheetName[0];
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    if (uSheetName.find("КОНД") == 0)
        r = 'C';
    else
        if (uSheetName.find("РЕЗИ"))
            r = 'R';
    return r;
}

/**
 * 35v  330mkf -> 330мкФ V:35
 * 400v10mf -> 10мкФ V:400
 * 50v3,3mf -> 3.3мкФ V:50
 * Sheet name конденсаторы

 * @param retVal
 * @param value
 * @return
 */
bool SpreadSheetHelper::parseC(
    SheetRow &retVal,
    const std::string &value
) {
    std::string vc;
    vc = toUpperCase(value);
    vc = trim(vc);
    size_t v = vc.find('V');
    if (v == std::string::npos) {
        v = 0;
        retVal.property_v = 0;
    } else {
        std::string volts = vc.substr(0, v);
        volts = trim(volts);
        retVal.property_v = std::strtol(volts.c_str(), nullptr, 10);
    }
    if (v >= vc.size())
        return false;
    v++;
    vc = vc.substr(v);
    vc = trim(vc);
    size_t ef = vc.size();
    for (size_t p = 0; p < vc.size(); p++) {
        if (!isdigit(vc[p])) {
            ef = p;
            break;
        }
    }

    std::string farads = vc.substr(0, ef);
    farads = trim(farads);
    retVal.nominal = std::strtoull(farads.c_str(), nullptr, 10);
    //
    retVal.nominal *= 1000000; // MF, MKF -> pF
    return true;
}

/**
 * 35v  330mkG -> 330мкГн V:35
 * 400v10mg -> 10мкГн V:400
 * 50v3,3mg -> 3.3мкГн V:50
 * Sheet name индуктивности

 * @param retVal
 * @param value
 * @return
 */
bool SpreadSheetHelper::parseL(
    SheetRow &retVal,
    const std::string &value
) {
    std::string vc;
    vc = toUpperCase(value);
    vc = trim(vc);
    size_t v = vc.find('V');
    if (v == std::string::npos) {
        v = 0;
        retVal.property_v = 0;
    } else {
        std::string volts = vc.substr(0, v);
        volts = trim(volts);
        retVal.property_v = std::strtol(volts.c_str(), nullptr, 10);
    }
    if (v >= vc.size())
        return false;
    v++;
    vc = vc.substr(v);
    vc = trim(vc);
    size_t ef = vc.size();
    for (size_t p = 0; p < vc.size(); p++) {
        if (!isdigit(vc[p])) {
            ef = p;
            break;
        }
    }
    std::string gnr = vc.substr(0, ef);
    gnr = trim(gnr);
    retVal.nominal = std::strtoull(gnr.c_str(), nullptr, 10);
    retVal.nominal *= 1000000; // MF, MKF -> pG
    return true;
}

bool SpreadSheetHelper::parseR(
    SheetRow &retVal,
    const std::string &value
) {
    std::string vc;
    vc = toUpperCase(value);
    vc = trim(vc);
    return false;
}
