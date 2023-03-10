//
// Created by andrei on 10.03.23.
//

#include "MeasureUnit.h"

#define MAX_POW10 11

static const std::string prefixes[MAX_POW10] {
    "",
    "k",
    "M",
    "G",
    "Т",
    "P",
    "E",
    "Z",
    "Y",
    "R",
    "Q"
};

static const std::string prefixesPart[MAX_POW10] {
    "",
    "m",
    "µ",
    "n",
    "p",
    "f",
    "a",
    "z",
    "y",
    "r",
    "q"
};

static int measurePow10[] {
    0,
    -9,
    -9
};

static const std::string symNamesRU[] {
    "R",
    "C",
    "L"
};

static const std::string unitNamesRU[] {
    "Ом",
    "Ф",
    "Гн"
};

MeasureUnit::MeasureUnit()
{

}

MeasureUnit::~MeasureUnit()
{

}

std::string MeasureUnit::sym(MEASURE measure) {
    return symNamesRU[measure];
}

std::string MeasureUnit::unit(MEASURE measure)
{
    return unitNamesRU[measure];
}

int MeasureUnit::pow10(MEASURE measure)
{
    return measurePow10[measure];
}

std::string val1000(uint64_t value, int initialPow10) {
    uint64_t v = value;
    int initialPowIdx = initialPow10 / 3;
    for (auto i = 0; i < MAX_POW10; i++) {
        if (v < 1000) {
            int idx = i + initialPowIdx;
            if (idx >= 0)
                return std::to_string(v) + ' ' + prefixes[idx];
            else
                return std::to_string(v) + ' ' + prefixesPart[-idx];
        }
        v /= 1000;
    }
    return "";
}

std::string MeasureUnit::value(MEASURE measure, uint64_t value)
{
    return val1000(value, measurePow10[measure]) + unit(measure);
}

double MeasureUnit::val(MEASURE measure, uint64_t value)
{
    return 0;
}
