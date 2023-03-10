//
// Created by andrei on 10.03.23.
//

#ifndef RCR_MEASUREUNIT_H
#define RCR_MEASUREUNIT_H

#include <string>

typedef enum {
    M_R = 1,
    M_C = 2,
    M_L = 3
} MEASURE;

class MeasureUnit {
public:
    MeasureUnit();
    virtual ~MeasureUnit();
    std::string sym(MEASURE measure);
    std::string unit(MEASURE measure);
    int pow10(MEASURE measure);
    std::string value(MEASURE measure, uint64_t value);
    double val(MEASURE measure, uint64_t value);
};


#endif //RCR_MEASUREUNIT_H
