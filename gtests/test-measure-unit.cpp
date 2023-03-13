#include "gtest/gtest.h"
#include "MeasureUnit.h"

TEST(MeasureUnit, ParseR) {
    size_t position = 0;
    uint64_t nominal;
    MEASURE measure;
    std::string ic;

    MeasureUnit::parse(ML_RU, "100 кОм", position, nominal, measure, ic);
    ASSERT_EQ(nominal, 100000);
    ASSERT_EQ(measure, M_R);
}

TEST(MesureUnit, Resistor) {
    MeasureUnit u;
    ASSERT_EQ(u.value(ML_RU, M_R, 1), "1 Ом");
    ASSERT_EQ(u.value(ML_RU, M_R, 20), "20 Ом");
    ASSERT_EQ(u.value(ML_RU, M_R, 300), "300 Ом");
    ASSERT_EQ(u.value(ML_RU, M_R, 4100), "4 kОм");
    ASSERT_EQ(u.value(ML_RU, M_R, 5500), "5 kОм");
    ASSERT_EQ(u.value(ML_RU, M_R, 6900), "6 kОм");
    ASSERT_EQ(u.value(ML_RU, M_R, 70000), "70 kОм");
    ASSERT_EQ(u.value(ML_RU, M_R, 800000), "800 kОм");
    ASSERT_EQ(u.value(ML_RU, M_R, 9000000), "9 MОм");
    ASSERT_EQ(u.value(ML_RU, M_R, 10000000), "10 MОм");

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