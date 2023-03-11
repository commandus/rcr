#include "gtest/gtest.h"
#include "MeasureUnit.h"

TEST(MesureUnit, Resistor) {
    MeasureUnit u;
    ASSERT_EQ(u.value(M_R, 1), "1 Ом");
    ASSERT_EQ(u.value(M_R, 20), "20 Ом");
    ASSERT_EQ(u.value(M_R, 300), "300 Ом");
    ASSERT_EQ(u.value(M_R, 4100), "4 kОм");
    ASSERT_EQ(u.value(M_R, 5500), "5 kОм");
    ASSERT_EQ(u.value(M_R, 6900), "6 kОм");
    ASSERT_EQ(u.value(M_R, 70000), "70 kОм");
    ASSERT_EQ(u.value(M_R, 800000), "800 kОм");
    ASSERT_EQ(u.value(M_R, 9000000), "9 MОм");
    ASSERT_EQ(u.value(M_R, 10000000), "10 MОм");

    ASSERT_EQ(u.value(M_C, 1), "1 pФ");
    ASSERT_EQ(u.value(M_C, 20), "20 pФ");
    ASSERT_EQ(u.value(M_C, 300), "300 pФ");
    ASSERT_EQ(u.value(M_C, 4000), "4 nФ");
    ASSERT_EQ(u.value(M_C, 50000), "50 nФ");
    ASSERT_EQ(u.value(M_C, 600000), "600 nФ");
    ASSERT_EQ(u.value(M_C, 7000000), "7 µФ");
    ASSERT_EQ(u.value(M_C, 80000000), "80 µФ");
    ASSERT_EQ(u.value(M_C, 900000000), "900 µФ");
    ASSERT_EQ(u.value(M_C, 1234567890), "1 mФ");
    ASSERT_EQ(u.value(M_C, 23456789012), "23 mФ");
    ASSERT_EQ(u.value(M_C, 345678901234), "345 mФ");
    ASSERT_EQ(u.value(M_C, 5678901234567), "5 Ф");

    ASSERT_DOUBLE_EQ(u.val(M_C, 5678901234567), 5.678901234567);
    ASSERT_DOUBLE_EQ(u.val(M_R, 5678901234567), 5678901234567);


}
