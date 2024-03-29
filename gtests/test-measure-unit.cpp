#include <map>
#include <string>
#include <vector>
#include "gtest/gtest.h"

#include "MeasureUnit.h"
#include "StockOperation.h"
#include "QueryProperties.h"
#include "RCQuery.h"
#include "BoxName.h"
#include "string-helper.h"

TEST(RCQuery, ParseNameNNominal) {
    size_t position;

    RCQuery q;

    position = 0;
    q.parse(ML_RU, "* 255", position, COMPONENT_Z);
    ASSERT_EQ(q.measure, COMPONENT_Z);
    ASSERT_EQ(q.componentName, "*");
    ASSERT_EQ(q.nominal, 0);
    ASSERT_EQ(q.code, SO_LIST);
    ASSERT_EQ(q.boxes, 0x00ff000000000000);

    position = 0;
    q.parse(ML_RU, "* 100мкф key:value", position, COMPONENT_C);
    ASSERT_EQ(q.measure, COMPONENT_C);
    ASSERT_EQ(q.componentName, "*");
    ASSERT_EQ(q.nominal, 100000000);
    ASSERT_EQ(q.code, SO_LIST_NO_BOX);
    ASSERT_EQ(q.properties["key"], "value");

    position = 0;
    q.parse(ML_RU, "* 100мкф key:value 255", position, COMPONENT_C);
    ASSERT_EQ(q.measure, COMPONENT_C);
    ASSERT_EQ(q.componentName, "*");
    ASSERT_EQ(q.nominal, 100000000);
    ASSERT_EQ(q.code, SO_LIST);
    ASSERT_EQ(q.properties["key"], "value");
    ASSERT_EQ(q.boxes, 0x00ff000000000000);
}

TEST(RCQuery, ParseDigits) {
    size_t position;

    position = 0;
    RCQuery q;
    q.parse(ML_RU, "2S*", position, COMPONENT_D);
    ASSERT_EQ(q.measure, COMPONENT_D);
    ASSERT_EQ(q.componentName, "2S*");
    ASSERT_EQ(q.code, SO_LIST_NO_BOX);
    ASSERT_EQ(q.nominal, 0);
}

TEST(BoxId, Rename) {
    // 1-2 to 3
    uint64_t srcBoxRoot = StockOperation::parseBoxes("1-2");
    uint64_t srcBox1 = StockOperation::parseBoxes("1-2");
    uint64_t srcBox2 = StockOperation::parseBoxes("1-2-3");
    uint64_t srcBox3 = StockOperation::parseBoxes("1-2-3-4");
    uint64_t destBox = StockOperation::parseBoxes("3");
    uint64_t destBox1 = StockOperation::renameBox(srcBox1, srcBoxRoot, destBox);
    uint64_t destBox2 = StockOperation::renameBox(srcBox2, srcBoxRoot, destBox);
    uint64_t destBox3 = StockOperation::renameBox(srcBox3, srcBoxRoot, destBox);
    ASSERT_EQ(StockOperation::boxes2string(destBox1), "3");
    ASSERT_EQ(StockOperation::boxes2string(destBox2), "3-3");
    ASSERT_EQ(StockOperation::boxes2string(destBox3), "3-3-4");
    // to root
    srcBoxRoot = StockOperation::parseBoxes("1-2");
    srcBox1 = StockOperation::parseBoxes("1-2");
    srcBox2 = StockOperation::parseBoxes("1-2-3");
    srcBox3 = StockOperation::parseBoxes("1-2-3-4");
    destBox = StockOperation::parseBoxes("");
    destBox1 = StockOperation::renameBox(srcBox1, srcBoxRoot, destBox);
    destBox2 = StockOperation::renameBox(srcBox2, srcBoxRoot, destBox);
    destBox3 = StockOperation::renameBox(srcBox3, srcBoxRoot, destBox);
    ASSERT_EQ(StockOperation::boxes2string(destBox1), "0");
    ASSERT_EQ(StockOperation::boxes2string(destBox2), "3");
    ASSERT_EQ(StockOperation::boxes2string(destBox3), "3-4");
    // overflow destination
    srcBoxRoot = StockOperation::parseBoxes("1-2");
    srcBox1 = StockOperation::parseBoxes("1-2");
    srcBox2 = StockOperation::parseBoxes("1-2-7");
    srcBox3 = StockOperation::parseBoxes("1-2-7-8");
    destBox = StockOperation::parseBoxes("3-4-5");
    destBox1 = StockOperation::renameBox(srcBox1, srcBoxRoot, destBox);
    destBox2 = StockOperation::renameBox(srcBox2, srcBoxRoot, destBox);
    destBox3 = StockOperation::renameBox(srcBox3, srcBoxRoot, destBox);
    ASSERT_EQ(StockOperation::boxes2string(destBox1), "3-4-5");
    ASSERT_EQ(StockOperation::boxes2string(destBox2), "3-4-5-7");
    ASSERT_EQ(StockOperation::boxes2string(destBox3), "3-4-5-7");
}

TEST(String, split) {
    std::vector<std::string> r = split(" 1  2    3", ' ');
    ASSERT_EQ(r.size(), 3);
    ASSERT_EQ(r[0], "1");
    ASSERT_EQ(r[1], "2");
    ASSERT_EQ(r[2], "3");
}

TEST(Measure, List) {
    std::vector<std::string> r;
    listUnitNParticle(r, ML_RU, COMPONENT_R);
    ASSERT_EQ(r.size(), MAX_POW10);
    ASSERT_EQ(r[0], "Ом");
    ASSERT_EQ(r[1], "кОм");

    r.clear();
    listUnitNParticle(r, ML_RU, COMPONENT_C);
    ASSERT_EQ(r.size(), MAX_POW10);
    ASSERT_EQ(r[0], "Ф");
    ASSERT_EQ(r[1], "мФ");
    ASSERT_EQ(r[2], "мкФ");
}

TEST(Box, Append) {
    uint64_t b = 0;
    b = StockOperation::boxAppendBox(b, 0xdd);
    ASSERT_EQ(b, 0x00dd000000000000);
    b = StockOperation::boxAppendBox(b, 0xcc);
    ASSERT_EQ(b, 0x00dd00cc00000000);
    b = StockOperation::boxAppendBox(b, 0xbb);
    ASSERT_EQ(b, 0x00dd00cc00bb0000);
    b = StockOperation::boxAppendBox(b, 0xaa);
    ASSERT_EQ(b, 0x00dd00cc00bb00aa);
}

TEST(BoxName, FileName) {
    uint64_t b = BoxName::extractFromFileName("221 Мксх_Золотовский_1");
    ASSERT_EQ(b, 0x00dd000100000000);
    std::string bn = StockOperation::boxes2string(b);
    ASSERT_EQ(bn, "221-1");
    uint64_t a = StockOperation::boxAppendBox(0x00dd000100000000, 0xdd);
    bn = StockOperation::boxes2string(a);
    ASSERT_EQ(bn, "221-1-221");

    a = StockOperation::boxAppendBox(a, 0xdd);
    bn = StockOperation::boxes2string(a);
    ASSERT_EQ(bn, "221-1-221-221");

}

TEST(RCQuery, Count) {
    size_t position;
    RCQuery q;

    position = 0;
    q.parse(ML_RU, " К555ЛА7 key1:value1 key2:value2", position, COMPONENT_S);
    ASSERT_EQ(q.measure, COMPONENT_S);
    ASSERT_EQ(q.nominal, 0);
    ASSERT_EQ(q.componentName, "К555ЛА7");
    ASSERT_EQ(q.properties["key2"], "value2");

    ASSERT_EQ(q.boxBlocks, 0);
    ASSERT_EQ(q.boxes, 0);

    ASSERT_EQ(q.code, SO_LIST_NO_BOX);
    ASSERT_EQ(q.count, 0);

    position = 0;
    q.parse(ML_RU, " К555ЛА7 key1:value1 count", position, COMPONENT_D);
    ASSERT_EQ(q.measure, COMPONENT_D);
    ASSERT_EQ(q.nominal, 0);
    ASSERT_EQ(q.componentName, "К555ЛА7");

    ASSERT_EQ(q.boxBlocks, 0);
    ASSERT_EQ(q.boxes, 0);

    ASSERT_EQ(q.code, SO_COUNT);
    ASSERT_EQ(q.count, 0);


    position = 0;
    q.parse(ML_RU, " К555ЛА7 key1:value1 key2:value2 221-1 /1234   221-2   ", position, COMPONENT_V);
    ASSERT_EQ(q.measure, COMPONENT_V);
    ASSERT_EQ(q.nominal, 0);
    ASSERT_EQ(q.componentName, "К555ЛА7");
    ASSERT_EQ(q.properties["key1"], "value1");
    ASSERT_EQ(q.properties["key2"], "value2");

    ASSERT_EQ(q.boxBlocks, 2);
    ASSERT_EQ(q.boxes, 0x00dd000100000000);
    ASSERT_EQ(q.destinationBox, 0x00dd000200000000);

    ASSERT_EQ(q.code, SO_MOV);
    ASSERT_EQ(q.count, 1234);

    position = 0;
    q.parse(ML_RU, " К555ЛА7 key1:value1 key2:value2 221-1 /1234", position, COMPONENT_V);
    ASSERT_EQ(q.measure, COMPONENT_V);
    ASSERT_EQ(q.nominal, 0);
    ASSERT_EQ(q.componentName, "К555ЛА7");
    ASSERT_EQ(q.properties["key1"], "value1");
    ASSERT_EQ(q.properties["key2"], "value2");

    ASSERT_EQ(q.boxBlocks, 2);
    ASSERT_EQ(q.boxes, 0x00dd000100000000);
    ASSERT_EQ(q.destinationBox, 0);

    ASSERT_EQ(q.code, SO_LIST); // no
    ASSERT_EQ(q.count, 1234);
}

TEST(RCQuery, Parse) {
    size_t position;

    position = 0;
    RCQuery q;
    q.parse(ML_RU, "154 ком key:value 221-1 +12", position, COMPONENT_S);
    ASSERT_EQ(q.measure, COMPONENT_R);
    ASSERT_EQ(q.nominal, 154000);
    ASSERT_EQ(q.properties["key"], "value");

    ASSERT_EQ(q.boxBlocks, 2);
    ASSERT_EQ(q.boxes, 0xDD000100000000);

    ASSERT_EQ(q.code, SO_ADD);
    ASSERT_EQ(q.count, 12);


    position = 0;
    q.parse(ML_RU, " К555ЛА7 key1:value1 key2:value2 221-1 =123 ", position, COMPONENT_U);
    ASSERT_EQ(q.measure, COMPONENT_U);
    ASSERT_EQ(q.nominal, 0);
    ASSERT_EQ(q.componentName, "К555ЛА7");
    ASSERT_EQ(q.properties["key2"], "value2");

    ASSERT_EQ(q.boxBlocks, 2);
    ASSERT_EQ(q.boxes, 0xDD000100000000);

    ASSERT_EQ(q.code, SO_SET);
    ASSERT_EQ(q.count, 123);

    position = 0;
    q.parse(ML_RU, " К555ЛА7 key1:value1 key2:value2 221-1 ", position, COMPONENT_U);
    ASSERT_EQ(q.measure, COMPONENT_U);
    ASSERT_EQ(q.nominal, 0);
    ASSERT_EQ(q.componentName, "К555ЛА7");
    ASSERT_EQ(q.properties["key2"], "value2");

    ASSERT_EQ(q.boxBlocks, 2);
    ASSERT_EQ(q.boxes, 0xDD000100000000);

    ASSERT_EQ(q.code, SO_LIST);
    ASSERT_EQ(q.count, 0);

    position = 0;
    q.parse(ML_RU, " К555ЛА7 key1:value1 key2:value2 221-1 count", position, COMPONENT_U);
    ASSERT_EQ(q.measure, COMPONENT_U);
    ASSERT_EQ(q.nominal, 0);
    ASSERT_EQ(q.componentName, "К555ЛА7");
    ASSERT_EQ(q.properties["key2"], "value2");

    ASSERT_EQ(q.boxBlocks, 2);
    ASSERT_EQ(q.boxes, 0xDD000100000000);

    ASSERT_EQ(q.code, SO_COUNT);
    ASSERT_EQ(q.count, 0);
}

TEST(QueryProperties, Parse) {
    size_t position;
    std::map<std::string, std::string> kv;
    position = 0;
    QueryProperties::parse("key:value", position, kv);
    ASSERT_EQ(kv["key"], "value");
    position = 0;
    QueryProperties::parse("  key2:  value2  ", position, kv);
    ASSERT_EQ(kv["key2"], "value2");
    position = 0;
    QueryProperties::parse("  key3  :value3  ", position, kv);
    ASSERT_EQ(kv["key3"], "value3");

    position = 0;
    QueryProperties::parse("  key4  : value4 key5  : value5 key6  : value6", position, kv);
    ASSERT_EQ(kv["key4"], "value4");
    ASSERT_EQ(kv["key5"], "value5");
    ASSERT_EQ(kv["key6"], "value6");
}

TEST(StockOperation, List) {
    StockOperation so1("   1 +12345  ");
    ASSERT_EQ(so1.code, SO_ADD);
    ASSERT_EQ(so1.boxBlocks, 1);
    ASSERT_EQ(so1.count, 12345);
    StockOperation so2("1-2 +2345");
    ASSERT_EQ(so2.boxBlocks, 2);
    ASSERT_EQ(so2.code, SO_ADD);
    ASSERT_EQ(so2.count, 2345);
    StockOperation so3("1-2-3 -42");
    ASSERT_EQ(so3.code, SO_SUB);
    ASSERT_EQ(so3.boxBlocks, 3);
    ASSERT_EQ(so3.count, 42);
    StockOperation so4("1-2-3-4 =1000");
    ASSERT_EQ(so4.code, SO_SET);
    ASSERT_EQ(so4.boxBlocks, 4);
    ASSERT_EQ(so4.count, 1000);
    StockOperation so5("1-2-3-4-5 =12");
    ASSERT_EQ(so5.code, SO_SET);
    ASSERT_EQ(so5.boxBlocks, 4);
    ASSERT_EQ(so5.count, 12);
    StockOperation so6(" 255:255-255-255-1+5:1 = 123");
    ASSERT_EQ(so6.code, SO_SET);
    ASSERT_EQ(so6.boxes, 0x00ff00ff00ff00ff);
    ASSERT_EQ(so6.boxBlocks, 4);
    ASSERT_EQ(so6.count, 123);

}

TEST(MeasureUnit, ParseC) {
    size_t position;
    uint64_t nominal;
    COMPONENT measure;
    std::string ic;

    position = 0;
    MeasureUnit::parse(ML_RU, "100 пФ", position, nominal, measure, ic, COMPONENT_S);
    ASSERT_EQ(nominal, 100);
    ASSERT_EQ(measure, COMPONENT_C);

    position = 0;
    MeasureUnit::parse(ML_RU, "100 мкФ", position, nominal, measure, ic, COMPONENT_S);
    ASSERT_EQ(nominal, 100000000);
    ASSERT_EQ(measure, COMPONENT_C);
}

TEST(MeasureUnit, ParseR) {
    size_t position;
    uint64_t nominal;
    COMPONENT measure;
    std::string ic;

    position = 0;
    MeasureUnit::parse(ML_RU, "100 кОм", position, nominal, measure, ic, COMPONENT_S);
    ASSERT_EQ(nominal, 100000);
    ASSERT_EQ(measure, COMPONENT_R);

    position = 0;
    MeasureUnit::parse(ML_RU, "1000 ком", position, nominal, measure, ic, COMPONENT_S);
    ASSERT_EQ(nominal, 1000000);
    ASSERT_EQ(measure, COMPONENT_R);

    position = 0;
    MeasureUnit::parse(ML_RU, "100 ом", position, nominal, measure, ic, COMPONENT_S);
    ASSERT_EQ(nominal, 100);
    ASSERT_EQ(measure, COMPONENT_R);

    position = 0;
    MeasureUnit::parse(ML_RU, "1 ом", position, nominal, measure, ic, COMPONENT_S);
    ASSERT_EQ(nominal, 1);
    ASSERT_EQ(measure, COMPONENT_R);

    position = 0;
    MeasureUnit::parse(ML_RU, "0 ом", position, nominal, measure, ic, COMPONENT_S);
    ASSERT_EQ(nominal, 0);
    ASSERT_EQ(measure, COMPONENT_R);

    position = 0;
    MeasureUnit::parse(ML_RU, " ом", position, nominal, measure, ic, COMPONENT_S);
    ASSERT_EQ(ic, "ом");
    ASSERT_EQ(nominal, 0);
    ASSERT_EQ(measure, COMPONENT_S);

    position = 0;
    MeasureUnit::parse(ML_RU, "К155ЛА5", position, nominal, measure, ic, COMPONENT_D);
    ASSERT_EQ(ic, "К155ЛА5");
    ASSERT_EQ(nominal, 0);
    ASSERT_EQ(measure, COMPONENT_D);
}

TEST(MesureUnit, Resistor) {
    MeasureUnit u;
    ASSERT_EQ(u.value(ML_RU, COMPONENT_R, 1), "1 Ом");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_R, 20), "20 Ом");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_R, 300), "300 Ом");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_R, 4100), "4 кОм");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_R, 5500), "5 кОм");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_R, 6900), "6 кОм");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_R, 70000), "70 кОм");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_R, 800000), "800 кОм");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_R, 9000000), "9 МОм");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_R, 10000000), "10 МОм");

    ASSERT_EQ(u.value(ML_RU, COMPONENT_C, 1), "1 пФ");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_C, 20), "20 пФ");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_C, 300), "300 пФ");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_C, 4000), "4 нФ");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_C, 50000), "50 нФ");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_C, 600000), "600 нФ");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_C, 7000000), "7 мкФ");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_C, 80000000), "80 мкФ");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_C, 900000000), "900 мкФ");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_C, 1234567890), "1 мФ");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_C, 23456789012), "23 мФ");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_C, 345678901234), "345 мФ");
    ASSERT_EQ(u.value(ML_RU, COMPONENT_C, 5678901234567), "5 Ф");

    ASSERT_DOUBLE_EQ(u.val(COMPONENT_C, 5678901234567), 5.678901234567);
    ASSERT_DOUBLE_EQ(u.val(COMPONENT_R, 5678901234567), 5678901234567);
}

TEST(RCQuery, ParseAsterisk) {
    size_t position;

    position = 0;
    RCQuery q;
    q.parse(ML_RU, "K* key:value 221-1 +12", position, COMPONENT_D);
    ASSERT_EQ(q.measure, COMPONENT_D);
    ASSERT_EQ(q.nominal, 0);

    position = 0;
    q.parse(ML_RU, "* key:value 221-1 +12", position, COMPONENT_D);
    ASSERT_EQ(q.measure, COMPONENT_D);
    ASSERT_EQ(q.nominal, 0);
}

extern "C" void *pluginInit(
    const std::string &path,
    void *options
);

extern "C" void pluginDone(
    void *env
);

extern "C" bool pluginLogin(
    void *env,
    const std::string &login,
    const std::string &password
);

TEST(LDAP, login) {
    const std::string u = "ldap://ad.ysn.ru";
    void *e = pluginInit("", (void *) &u);
    bool y = pluginLogin(e, "andrey.ivanov@ikfia.ysn.ru", "");
    ASSERT_EQ(y, false);
    y = pluginLogin(e, "andrey.ivanov@ikfia.ysn.ru", "vjhlthktyl");
    ASSERT_EQ(y, true);
    y = pluginLogin(e, "", "");
    ASSERT_EQ(y, false);

    for (int i = 0; i < 3; i++) {
        y = pluginLogin(e, "andrey.ivanov@ikfia.ysn.ru", "***");
        ASSERT_EQ(y, false);
        y = pluginLogin(e, "", "");
        ASSERT_EQ(y, false);
    }
    y = pluginLogin(e, "andrey.ivanov@ikfia.ysn.ru", "vjhlthktyl");
    ASSERT_EQ(y, true);

    pluginDone(e);
}
