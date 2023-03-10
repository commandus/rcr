//
// Created by andrei on 10.03.23.
//

#ifndef RCR_MEASUREUNIT_H
#define RCR_MEASUREUNIT_H

#include <string>

typedef enum {
    M_R = 0,
    M_C = 1,
    M_L = 2
} MEASURE;

class MeasureUnit {
public:
    MeasureUnit();
    virtual ~MeasureUnit();
    static std::string sym(MEASURE measure);
    static std::string unit(MEASURE measure);
    static int pow10(MEASURE measure);
    static std::string value(MEASURE measure, uint64_t value);
    static double val(MEASURE measure, uint64_t value);
};

#endif //RCR_MEASUREUNIT_H
