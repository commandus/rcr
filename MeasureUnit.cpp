//
// Created by andrei on 10.03.23.
//

#include <cstring>

#include "MeasureUnit.h"
#include "string-helper.h"

#define MAX_POW10 11
#define LOCALES 2
static const std::string prefixes[LOCALES][MAX_POW10] {
    "",
    "k",    // kilo - start here
    "M",    // mega
    "G",    // giga
    "Т",    // tera
    "P",
    "E",
    "Z",
    "Y",
    "R",
    "Q",

    "",
    "к",    // kilo - start here
    "М",    // mega
    "Г",    // giga
    "Т",    // tera
    "П",
    "Е",
    "З",
    "И",
    "Р",
    "Кв"

};

static const std::string prefixesPart[LOCALES][MAX_POW10] {
"",
"m",    // milli
"µ",    // micro - start here
"n",    // nano
"p",    // pico
"f",    // femto
"a",
"z",
"y",
"r",
"q",

"",
"м",    // milli
"мк",   // micro - start here
"н",    // nano
"п",    // pico
"ф",    // femto
"ф",
"з",
"и",
"р",
"кв"
};


static int measurePow10[4] {
    0,      // R
    -12,    // C
    -12,    // L
    0       // U
};

static const std::string symNames[LOCALES][4] {
"R",
"C",
"L",
"U",
"R",
"C",
"L",
"U"
};

static const std::string unitNames[LOCALES][4] {
    "Ohm",
    "F",
    "G",
    "",

    "Ом",
    "Ф",
    "Гн",
    ""
};

static const std::string unitNamesUpperCase[LOCALES][4] {
        "OHM",
        "F",
        "G",
        "",

        "ОМ",
        "Ф",
        "ГН",
        ""
};

std::string MeasureUnit::sym(MEASURE_LOCALE locale, MEASURE measure) {
    return symNames[locale][measure];
}

std::string MeasureUnit::unit(MEASURE_LOCALE locale, MEASURE measure)
{
    return unitNames[locale][measure];
}

int MeasureUnit::pow10(MEASURE measure)
{
    return measurePow10[measure];
}

std::string val1000(MEASURE_LOCALE locale, uint64_t value, int initialPow10) {
    uint64_t v = value;
    int initialPowIdx = initialPow10 / 3;
    for (auto i = 0; i < MAX_POW10; i++) {
        if (v < 1000) {
            int idx = i + initialPowIdx;
            if (idx >= 0)
                return std::to_string(v) + ' ' + prefixes[locale][idx];
            else
                return std::to_string(v) + ' ' + prefixesPart[locale][ - idx ];
        }
        v /= 1000;
    }
    return "";
}

std::string MeasureUnit::value(MEASURE_LOCALE locale, MEASURE measure, uint64_t val)
{
    return val1000(locale, val, measurePow10[measure]) + unit(locale, measure);
}

static uint64_t pow10table[13] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000,
    100000000, 1000000000, 10000000000L, 100000000000L, 1000000000000L };

double MeasureUnit::val(MEASURE measure, uint64_t value)
{
    int p = measurePow10[measure];
    if (p < 0)
        return 1.0 * value / pow10table[-p];
    else
        return 1.0 * value * pow10table[p];
}

int MeasureUnit::parse(
    MEASURE_LOCALE locale,
    const std::string &value,
    size_t &position,
    uint64_t &nominal,
    MEASURE &measure,
    std::string &retname
)
{
    size_t start = position;
    size_t eolp = value.length();
    size_t finish = eolp;

    std::string valueUpperCase = toUpperCase(locale, value);

    // skip spaces if exists
    for (auto p = start; p < eolp; p++) {
        if (!std::isspace(value[p])) {
            start = p;
            break;
        }
    }

    // try read nominal
    for (auto p = start; p < eolp; p++) {
        if (!isdigit(value[p])) {
            finish = p;
            break;
        }
    }

    nominal = 0;
    retname = "";

    bool hasNominal = finish > start;

    if (hasNominal) {
        try {
            nominal = std::stoull(value.substr(start, finish - start));
        } catch (std::exception & e) {

        }
        start = finish;
        // skip spaces if exists
        for (auto p = start; p < eolp; p++) {
            if (!std::isspace(value[p])) {
                finish = p;
                break;
            }
        }

        // try find out measure unit and prefixes

        // try to find out prefix
        start = finish;
        int idx = -1;
        for (auto pf = 1; pf < MAX_POW10; pf++) {
            if (value.find(prefixes[locale][pf], start) == start) {
                // found prefix
                finish = start + prefixes[locale][pf].length();
                idx = pf;
                break;
            }
        }

        int pow10Index = 0;
        bool hasPrefix = finish > start;
        if (hasPrefix) {
            pow10Index = idx;
        } else {
            // try to find out prefixPart
            for (auto pf = 2; pf < MAX_POW10; pf++) {
                if (value.find(prefixesPart[locale][pf], start) == start) {
                    // found prefix
                    finish = start + prefixesPart[locale][pf].length();
                    idx = pf;
                    break;
                }
            }
        }
        bool hasPrefixPart = !hasPrefix && (finish > start);
        if (hasPrefixPart) {
            pow10Index = - idx;
        }

        // try to find out unit
        start = finish;
        idx = -1;
        for (auto ui = 0; ui < 3; ui++) {
            if (valueUpperCase.find(unitNamesUpperCase[locale][ui], start) == start) {
                // found unit name
                finish = start + unitNames[locale][ui].length();
                // 100пФ -12                -4 * 3
                // 10кОм 1                  1 * 3
                int dp = - measurePow10[ui] + pow10Index * 3;
                if (dp < 0)
                    nominal /= pow10table[- dp];
                else
                    nominal *= pow10table[dp];
                idx = ui;
                break;
            }
        }
        bool hasMeasureUnit = finish > start;
        if (hasMeasureUnit) {
            measure = (MEASURE) idx;
            position = finish;
            return 0;
        }
    }
    // IC
    measure = M_U;
    // skip spaces if exists
    start = position;
    finish = eolp;
    for (auto p = start; p < eolp; p++) {
        if (!std::isspace(value[p])) {
            start = p;
            break;
        }
    }
    // find out end of IC name (spaces)
    finish = eolp; // by default end of string
    for (auto p = start; p < eolp; p++) {
        if (std::isspace(value[p])) {
            finish = p;
            break;
        }
    }

    if (start < finish)
        retname = value.substr(start, finish - start);
    position = finish;
    return 0;
}

/**
 * Convert string to measure locale enum
 * @param value "intl" or "ru"
 * @return ML_RU by default
 */
MEASURE_LOCALE pchar2MEASURE_LOCALE(
    const char *value
)
{
    if (strcmp("intl", value) == 0)
        return ML_INTL;
    return ML_RU;
}
