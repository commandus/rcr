//
// Created by andrei on 20.03.23.
//

#include "SpreadSheetHelper.h"

#include <xlnt/xlnt.hpp>

#include "utilstring.h"
#include "string-helper.h"
#include "StockOperation.h"

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

    for (std::vector <std::string>::const_iterator it = properties.begin(); it != properties.end(); it++) {
        rcr::PropertyRequest *prop = retval.mutable_properties()->Add();
        prop->set_property_type_name("K");
        prop->set_value(*it);
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
        wsheets[i].title(std::string('A' + i, 1));
        lastRow[i] = 2;
        wsheets[i].cell("A1").value("Коробка");
        wsheets[i].cell("B1").value("Наименование");
        wsheets[i].cell("C1").value("Количество");
        wsheets[i].cell("D1").value("Марка");
        wsheets[i].cell("E1").value(" Описание");
    }

    for (int i = 0; i < cards.cards_size(); i++) {
        rcr::CardNPropetiesPackages c = cards.cards(i);
        xlnt::worksheet &w = wsheets[c.card().symbol_id()];
        for (int p = 0; p < c.packages_size(); p++) {
            auto pack = c.packages(p);
            BoxArray a;
            StockOperation::box2Array(a, pack.box());
            w.cell("A" + std::to_string(lastRow[i])).value(a.a[0]);
            w.cell("B" + std::to_string(lastRow[i])).value(c.card().name());
            w.cell("C" + std::to_string(lastRow[i])).value(std::to_string(pack.qty()));
            std::stringstream ss;
            for (int pr = 0; pr < c.properties_size(); pr++) {
                ss << c.properties(pr).property_type() << ": " << c.properties(pr).value() << " ";
            }
            w.cell("D" + std::to_string(lastRow[i])).value(ss.str());
            w.cell("E" + std::to_string(lastRow[i])).value("");
        }
        lastRow[i] = lastRow[i] + 1;
    }
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

bool SpreadSheetHelper::parseR(
    SheetRow &retVal,
    const std::string &value
) {
    std::string vc;
    vc = toUpperCase(value);
    vc = trim(vc);
    return false;
}
