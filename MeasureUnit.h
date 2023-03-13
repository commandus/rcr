//
// Created by andrei on 10.03.23.
//

#ifndef RCR_MEASUREUNIT_H
#define RCR_MEASUREUNIT_H

#include <string>

typedef enum {
    ML_INTL = 0,     ///< international
    ML_RU = 1        ///< Russian
} MEASURE_LOCALE;

typedef enum {
    M_R = 0,    ///< resistor
    M_C = 1,    ///< condenser
    M_L = 2,    ///< inductive
    M_U = 3     ///< integrated circuit
} MEASURE;

class MeasureUnit {
public:
    MeasureUnit() = default;
    virtual ~MeasureUnit() = default;
    static std::string sym(MEASURE_LOCALE locale, MEASURE measure);
    static std::string unit(MEASURE_LOCALE locale, MEASURE measure);
    static int pow10(MEASURE measure);
    static std::string value(MEASURE_LOCALE locale, MEASURE measure, uint64_t val);
    static double val(MEASURE measure, uint64_t value);
    static int parse(MEASURE_LOCALE locale, const std::string &value,
                     size_t &position, uint64_t &nominal, MEASURE &measure, std::string &retname);
};

// Helper functions

/**
 * Convert string to measure locale enum
 * @param value "intl" or "ru"
 * @return ML_RU bu default
 */
MEASURE_LOCALE pchar2MEASURE_LOCALE(const char *value);

#endif //RCR_MEASUREUNIT_H
