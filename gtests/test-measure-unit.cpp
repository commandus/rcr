#include "gtest/gtest.h"
#include "MeasureUnit.h"

TEST(MesureUnit, Resistor) {
    MeasureUnit u;
    ASSERT_EQ(u.value(M_R, 1000), "1 kОм");
    ASSERT_EQ(u.value(M_R, 2000), "2 kОм");
    ASSERT_EQ(u.value(M_R, 3000), "3 kОм");
}

int main() {
    MeasureUnit u;

    std::cout << u.value(M_R, 1) << std::endl;
    std::cout << u.value(M_R, 20) << std::endl;
    std::cout << u.value(M_R, 300) << std::endl;
    std::cout << u.value(M_R, 4000) << std::endl;
    std::cout << u.value(M_R, 5000) << std::endl;
    std::cout << u.value(M_R, 6000) << std::endl;

    std::cout << u.value(M_R, 70000) << std::endl;
    std::cout << u.value(M_R, 800000) << std::endl;
    std::cout << u.value(M_R, 9000000) << std::endl;

    std::cout << u.value(M_R, 10000000) << std::endl;
    std::cout << u.value(M_R, 110000000) << std::endl;
    std::cout << u.value(M_R, 1200000000) << std::endl;


    std::cout << u.value(M_C, 1) << std::endl;
    std::cout << u.value(M_C, 20) << std::endl;
    std::cout << u.value(M_C, 300) << std::endl;
    std::cout << u.value(M_C, 4000) << std::endl;
    std::cout << u.value(M_C, 50000) << std::endl;
    std::cout << u.value(M_C, 600000) << std::endl;
    std::cout << u.value(M_C, 7000000) << std::endl;
    std::cout << u.value(M_C, 80000000) << std::endl;
    std::cout << u.value(M_C, 900000000) << std::endl;
    std::cout << u.value(M_C, 1234567890) << std::endl;
    std::cout << u.value(M_C, 23456789012) << std::endl;
    std::cout << u.value(M_C, 345678901234) << std::endl;
    std::cout << u.value(M_C, 5678901234567) << std::endl;





}
