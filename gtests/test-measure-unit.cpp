#include <map>
#include <string>
#include "gtest/gtest.h"

#include "MeasureUnit.h"
#include "StockOperation.h"
#include "QueryProperties.h"
#include "RCQuery.h"

TEST(RCQuery, Count) {
    size_t position;
    RCQuery q;

    position = 0;
    q.parse(ML_RU, " К555ЛА7 key1:value1 key2:value2", position);
    ASSERT_EQ(q.measure, M_U);
    ASSERT_EQ(q.nominal, 0);
    ASSERT_EQ(q.componentName, "К555ЛА7");
    ASSERT_EQ(q.properties["key2"], "value2");

    ASSERT_EQ(q.boxBlocks, 0);
    ASSERT_EQ(q.boxes, 0);

    ASSERT_EQ(q.code, SO_LIST_NO_BOX);
    ASSERT_EQ(q.count, 0);

    position = 0;
    q.parse(ML_RU, " К555ЛА7 key1:value1 count", position);
    ASSERT_EQ(q.measure, M_U);
    ASSERT_EQ(q.nominal, 0);
    ASSERT_EQ(q.componentName, "К555ЛА7");

    ASSERT_EQ(q.boxBlocks, 0);
    ASSERT_EQ(q.boxes, 0);

    ASSERT_EQ(q.code, SO_COUNT);
    ASSERT_EQ(q.count, 0);


}

TEST(RCQuery, Parse) {
    size_t position;

    position = 0;
    RCQuery q;
    q.parse(ML_RU, "154 ком key:value 221-1 +12", position);
    ASSERT_EQ(q.measure, M_R);
    ASSERT_EQ(q.nominal, 154000);
    ASSERT_EQ(q.properties["key"], "value");

    ASSERT_EQ(q.boxBlocks, 2);
    ASSERT_EQ(q.boxes, 0xDD000100000000);

    ASSERT_EQ(q.code, SO_ADD);
    ASSERT_EQ(q.count, 12);


    position = 0;
    q.parse(ML_RU, " К555ЛА7 key1:value1 key2:value2 221-1 =123 ", position);
    ASSERT_EQ(q.measure, M_U);
    ASSERT_EQ(q.nominal, 0);
    ASSERT_EQ(q.componentName, "К555ЛА7");
    ASSERT_EQ(q.properties["key2"], "value2");

    ASSERT_EQ(q.boxBlocks, 2);
    ASSERT_EQ(q.boxes, 0xDD000100000000);

    ASSERT_EQ(q.code, SO_SET);
    ASSERT_EQ(q.count, 123);

    position = 0;
    q.parse(ML_RU, " К555ЛА7 key1:value1 key2:value2 221-1 ", position);
    ASSERT_EQ(q.measure, M_U);
    ASSERT_EQ(q.nominal, 0);
    ASSERT_EQ(q.componentName, "К555ЛА7");
    ASSERT_EQ(q.properties["key2"], "value2");

    ASSERT_EQ(q.boxBlocks, 2);
    ASSERT_EQ(q.boxes, 0xDD000100000000);

    ASSERT_EQ(q.code, SO_LIST);
    ASSERT_EQ(q.count, 0);

    position = 0;
    q.parse(ML_RU, " К555ЛА7 key1:value1 key2:value2 221-1 count", position);
    ASSERT_EQ(q.measure, M_U);
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
    MEASURE measure;
    std::string ic;

    position = 0;
    MeasureUnit::parse(ML_RU, "100 пФ", position, nominal, measure, ic);
    ASSERT_EQ(nominal, 100);
    ASSERT_EQ(measure, M_C);

    position = 0;
    MeasureUnit::parse(ML_RU, "100 мкФ", position, nominal, measure, ic);
    ASSERT_EQ(nominal, 100000000);
    ASSERT_EQ(measure, M_C);
}

TEST(MeasureUnit, ParseR) {
    size_t position;
    uint64_t nominal;
    MEASURE measure;
    std::string ic;

    position = 0;
    MeasureUnit::parse(ML_RU, "100 кОм", position, nominal, measure, ic);
    ASSERT_EQ(nominal, 100000);
    ASSERT_EQ(measure, M_R);

    position = 0;
    MeasureUnit::parse(ML_RU, "1000 ком", position, nominal, measure, ic);
    ASSERT_EQ(nominal, 1000000);
    ASSERT_EQ(measure, M_R);

    position = 0;
    MeasureUnit::parse(ML_RU, "100 ом", position, nominal, measure, ic);
    ASSERT_EQ(nominal, 100);
    ASSERT_EQ(measure, M_R);

    position = 0;
    MeasureUnit::parse(ML_RU, "1 ом", position, nominal, measure, ic);
    ASSERT_EQ(nominal, 1);
    ASSERT_EQ(measure, M_R);

    position = 0;
    MeasureUnit::parse(ML_RU, "0 ом", position, nominal, measure, ic);
    ASSERT_EQ(nominal, 0);
    ASSERT_EQ(measure, M_R);

    position = 0;
    MeasureUnit::parse(ML_RU, " ом", position, nominal, measure, ic);
    ASSERT_EQ(ic, "ом");
    ASSERT_EQ(nominal, 0);
    ASSERT_EQ(measure, M_U);

    position = 0;
    MeasureUnit::parse(ML_RU, "К155ЛА5", position, nominal, measure, ic);
    ASSERT_EQ(ic, "К155ЛА5");
    ASSERT_EQ(nominal, 0);
    ASSERT_EQ(measure, M_U);
}

TEST(MesureUnit, Resistor) {
    MeasureUnit u;
    ASSERT_EQ(u.value(ML_RU, M_R, 1), "1 Ом");
    ASSERT_EQ(u.value(ML_RU, M_R, 20), "20 Ом");
    ASSERT_EQ(u.value(ML_RU, M_R, 300), "300 Ом");
    ASSERT_EQ(u.value(ML_RU, M_R, 4100), "4 кОм");
    ASSERT_EQ(u.value(ML_RU, M_R, 5500), "5 кОм");
    ASSERT_EQ(u.value(ML_RU, M_R, 6900), "6 кОм");
    ASSERT_EQ(u.value(ML_RU, M_R, 70000), "70 кОм");
    ASSERT_EQ(u.value(ML_RU, M_R, 800000), "800 кОм");
    ASSERT_EQ(u.value(ML_RU, M_R, 9000000), "9 МОм");
    ASSERT_EQ(u.value(ML_RU, M_R, 10000000), "10 МОм");

    ASSERT_EQ(u.value(ML_RU, M_C, 1), "1 пФ");
    ASSERT_EQ(u.value(ML_RU, M_C, 20), "20 пФ");
    ASSERT_EQ(u.value(ML_RU, M_C, 300), "300 пФ");
    ASSERT_EQ(u.value(ML_RU, M_C, 4000), "4 нФ");
    ASSERT_EQ(u.value(ML_RU, M_C, 50000), "50 нФ");
    ASSERT_EQ(u.value(ML_RU, M_C, 600000), "600 нФ");
    ASSERT_EQ(u.value(ML_RU, M_C, 7000000), "7 мкФ");
    ASSERT_EQ(u.value(ML_RU, M_C, 80000000), "80 мкФ");
    ASSERT_EQ(u.value(ML_RU, M_C, 900000000), "900 мкФ");
    ASSERT_EQ(u.value(ML_RU, M_C, 1234567890), "1 мФ");
    ASSERT_EQ(u.value(ML_RU, M_C, 23456789012), "23 мФ");
    ASSERT_EQ(u.value(ML_RU, M_C, 345678901234), "345 мФ");
    ASSERT_EQ(u.value(ML_RU, M_C, 5678901234567), "5 Ф");

    ASSERT_DOUBLE_EQ(u.val(M_C, 5678901234567), 5.678901234567);
    ASSERT_DOUBLE_EQ(u.val(M_R, 5678901234567), 5678901234567);
}
