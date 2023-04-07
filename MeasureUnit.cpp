//
// Created by andrei on 10.03.23.
//

#include <cstring>

#include "MeasureUnit.h"
#include "string-helper.h"

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
    "а",
    "з",
    "и",
    "р",
    "кв"
};

static int measurePow10[MEASURE_COUNT] {
    0,   // M_A Устройства
    0,   // M_B Микрофоны, громкоговорители
    -12, // M_C Конденсаторы
    0,   // M_D Интегральные схемы
    0,   // M_E Разные элементы
    0,   // M_F Плавкие предохранители
    0,   // M_G Источники питания
    0,   // M_H Индикаторы
    0,   // M_K Реле
    -12, // M_L Дроссели
    0,   // M_M Двигатели
    0,   // M_P Счетчики
    0,   // M_Q Выключатели
    0,   // M_R Резисторы
    0,   // M_S Переключатели
    0,   // M_T Трансформаторы
    0,   // M_U Выпрямители
    0,   // M_V Диоды, тиристоры, транзисторы
    0,   // M_W Антенны
    0,   // M_X Гнезда
    0,   // M_Y Электромагнитный привод
    0    // M_Z Кварцевые фильтры
};

static const std::string symNames[MEASURE_COUNT] {
        "A",   // M_A Устройства
        "B",   // M_B Микрофоны, громкоговорители
        "C",   // M_C Конденсаторы
        "D",   // M_D Интегральные схемы
        "E",   // M_E Разные элементы
        "F",   // M_F Плавкие предохранители
        "G",   // M_G Источники питания
        "H",   // M_H Индикаторы
        "K",   // M_K Реле
        "L",   // M_L Дроссели
        "M",   // M_M Двигатели
        "P",   // M_P Счетчики
        "Q",   // M_Q Выключатели
        "R",   // M_R Резисторы
        "S",   // M_S Переключатели
        "T",   // M_T Трансформаторы
        "U",   // M_U Выпрямители
        "V",   // M_V Диоды, тиристоры, транзисторы
        "W",   // M_W Антенны
        "X",   // M_X Гнезда
        "Y",   // M_Y Электромагнитный привод
        "Z"
};

static const std::string symDescriptions[LOCALES][MEASURE_COUNT] {
        "Devices",   // M_A
        "Mics",   // M_B
        "C",   // M_C
        "IC схемы",   // M_D
        "Etc",   // M_E
        "Fuses",   // M_F
        "Gen",   // M_G
        "Ind",   // M_H
        "Relay",   // M_K
        "L",   // M_L
        "Motors",   // M_M
        "Counters",   // M_P
        "Switches",   // M_Q
        "R",   // M_R
        "S",   // M_S
        "Trans",   // M_T
        "Rect",   // M_U
        "Diodes, Transistors",   // M_V
        "Ant",   // M_W
        "Nests",   // M_X
        "Actuators",   // M_Y
        "Quartz",   // M_Z

        "Устройства",   // M_A
        "Микрофоны, динамики",   // M_B
        "Конденсаторы",   // M_C
        "Интегральные схемы",   // M_D
        "Разные элементы",   // M_E
        "Предохранители",   // M_F
        "Источники питания",   // M_G
        "Индикаторы",   // M_H
        "Реле",   // M_K
        "Дроссели",   // M_L
        "Двигатели",   // M_M
        "Счетчики",   // M_P
        "Выключатели",   // M_Q
        "Резисторы",   // M_R
        "Переключатели",   // M_S
        "Трансформаторы",   // M_T
        "Выпрямители",   // M_U
        "Диоды, транзисторы",   // M_V
        "Антенны",   // M_W
        "Гнезда",   // M_X
        "Электромагнитные приводы",   // M_Y
        "Кварцевые фильтры"   // M_Z
};

static const std::string unitNames[LOCALES][MEASURE_COUNT] {
        "",   // M_A Устройства
        "",   // M_B Микрофоны, громкоговорители
        "F",   // M_C Конденсаторы
        "",   // M_D Интегральные схемы
        "",   // M_E Разные элементы
        "",   // M_F Плавкие предохранители
        "",   // M_G Источники питания
        "",   // M_H Индикаторы
        "",   // M_K Реле
        "G",  // M_L Дроссели
        "",   // M_M Двигатели
        "",   // M_P Счетчики
        "",   // M_Q Выключатели
        "Ohm",// M_R Резисторы
        "",   // M_S Переключатели
        "",   // M_T Трансформаторы
        "",   // M_U Выпрямители
        "",   // M_V Диоды, тиристоры, транзисторы
        "",   // M_W Антенны
        "",   // M_X Гнезда
        "",   // M_Y Электромагнитный привод
        "",   // M_Z Кварцевые фильтры

        "",   // M_A Устройства
        "",   // M_B Микрофоны, громкоговорители
        "Ф",  // M_C Конденсаторы
        "",   // M_D Интегральные схемы
        "",   // M_E Разные элементы
        "",   // M_F Плавкие предохранители
        "",   // M_G Источники питания
        "",   // M_H Индикаторы
        "",   // M_K Реле
        "Гн", // M_L Дроссели
        "",   // M_M Двигатели
        "",   // M_P Счетчики
        "",   // M_Q Выключатели
        "Ом", // M_R Резисторы
        "",   // M_S Переключатели
        "",   // M_T Трансформаторы
        "",   // M_U Выпрямители
        "",   // M_V Диоды, тиристоры, транзисторы
        "",   // M_W Антенны
        "",   // M_X Гнезда
        "",   // M_Y Электромагнитный привод
        ""    // M_Z Кварцевые фильтры
};

static const std::string unitNamesUpperCase[LOCALES][MEASURE_COUNT] {
        "",   // M_A Устройства
        "",   // M_B Микрофоны, громкоговорители
        "F",   // M_C Конденсаторы
        "",   // M_D Интегральные схемы
        "",   // M_E Разные элементы
        "",   // M_F Плавкие предохранители
        "",   // M_G Источники питания
        "",   // M_H Индикаторы
        "",   // M_K Реле
        "G",  // M_L Дроссели
        "",   // M_M Двигатели
        "",   // M_P Счетчики
        "",   // M_Q Выключатели
        "OHM",// M_R Резисторы
        "",   // M_S Переключатели
        "",   // M_T Трансформаторы
        "",   // M_U Выпрямители
        "",   // M_V Диоды, тиристоры, транзисторы
        "",   // M_W Антенны
        "",   // M_X Гнезда
        "",   // M_Y Электромагнитный привод
        "",   // M_Z Кварцевые фильтры

        "",   // M_A Устройства
        "",   // M_B Микрофоны, громкоговорители
        "Ф",  // M_C Конденсаторы
        "",   // M_D Интегральные схемы
        "",   // M_E Разные элементы
        "",   // M_F Плавкие предохранители
        "",   // M_G Источники питания
        "",   // M_H Индикаторы
        "",   // M_K Реле
        "ГН", // M_L Дроссели
        "",   // M_M Двигатели
        "",   // M_P Счетчики
        "",   // M_Q Выключатели
        "ОМ", // M_R Резисторы
        "",   // M_S Переключатели
        "",   // M_T Трансформаторы
        "",   // M_U Выпрямители
        "",   // M_V Диоды, тиристоры, транзисторы
        "",   // M_W Антенны
        "",   // M_X Гнезда
        "",   // M_Y Электромагнитный привод
        ""    // M_Z Кварцевые фильтры
};

std::string MeasureUnit::sym(COMPONENT measure) {
    return symNames[measure];
}

std::string MeasureUnit::description(MEASURE_LOCALE locale, COMPONENT measure) {
    return symDescriptions[locale][measure];
}

std::string MeasureUnit::unit(MEASURE_LOCALE locale, COMPONENT measure)
{
    return unitNames[locale][measure];
}

int MeasureUnit::pow10(COMPONENT measure)
{
    return measurePow10[measure];
}

/**
 * Print values with 1000 precision with particles
 * For instance, 1000 Ohm = 1kOhm
 * @param locale
 * @param value
 * @param initialPow10
 * @return
 */
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

std::string MeasureUnit::value(MEASURE_LOCALE locale, COMPONENT measure, uint64_t val)
{
    return val1000(locale, val, measurePow10[measure]) + unit(locale, measure);
}

static uint64_t pow10table[13] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000,
    100000000, 1000000000, 10000000000L, 100000000000L, 1000000000000L };

double MeasureUnit::val(COMPONENT measure, uint64_t value)
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
        COMPONENT &measure,
        std::string &retname,
        COMPONENT defaultMeasure
)
{
    size_t start = position;
    size_t eolp = value.length();
    size_t finish = eolp;

    std::string valueUpperCase = toUpperCase(value);

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
        for (auto ui = 0; ui < MEASURE_COUNT; ui++) {
            const std::string &s = unitNamesUpperCase[locale][ui];
            if (s.empty())
                continue;
            if (valueUpperCase.find(s, start) == start) {
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
            measure = (COMPONENT) idx;
            position = finish;
            return 0;
        }
    }
    // by default IC
    measure = defaultMeasure;
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

COMPONENT firstComponentInFlags(
    uint32_t flags
)
{
    for (int c = COMPONENT_A; c <= COMPONENT_Z; c++) {
        if (flags & (1 << c))
            return (COMPONENT) c;
    }
    return COMPONENT_A;
}

void listUnitNParticle(
    std::vector<std::string> &retVal,
    MEASURE_LOCALE locale,
    COMPONENT component
)
{
    std::string un = unitNames[locale][component];
    if (un.empty())
        return;
    int mp = measurePow10[component];
    for (int i = 0; i < MAX_POW10; i++) {
        if (mp < 0)
            retVal.push_back(prefixesPart[locale][i] + un);
        else
            retVal.push_back(prefixes[locale][i] + un);
    }
}

/**
 * Return COMPONENT_A by default
 * @param symbol "D" for IC
 * @return
 */
COMPONENT getComponentBySymbol(
    const std::string &symbol,
    const COMPONENT defaultComponent
) {
    for (int m = 0; m < MEASURE_COUNT; m++) {
        if (symNames[m] == symbol) {
            return (COMPONENT) m;
        }
    }
    return defaultComponent;
}
