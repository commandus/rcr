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
    0,    // A Устройства
    0,    // B Микрофоны, громкоговорители
    -12,  // C Конденсаторы
    0,    // D Интегральные схемы
    0,    // E Разные элементы
    0,    // F Плавкие предохранители
    0,    // G Источники питания
    0,    // H Индикаторы
    0,    // I == Крепления
    0,    // J == Провода
    0,   // K Реле
    -12, // L Дроссели
    0,   // M Двигатели
    0,   // N == На выброс
    0,   // O == Проекты
    0,   // P Счетчики
    0,   // Q Выключатели
    0,   // R Резисторы
    0,   // S Переключатели
    0,   // T Трансформаторы
    0,   // U Выпрямители
    0,   // V Диоды, тиристоры, транзисторы
    0,   // W Антенны
    0,   // X Гнезда
    0,   // Y Электромагнитный привод
    0    // Z Кварцевые фильтры
};

static const std::string symNames[MEASURE_COUNT] {
        "A",    // A Устройства
        "B",    // B Микрофоны, громкоговорители
        "C",    // C Конденсаторы
        "D",    // D Интегральные схемы
        "E",    // E Разные элементы
        "F",    // F Плавкие предохранители
        "G",    // G Источники питания
        "H",    // H Индикаторы
        "I",    // I == Крепления
        "J",    // J == Провода
        "K",   // K Реле
        "L",   // M Дроссели
        "M",   // M Двигатели
        "N",   // N == На выброс
        "O",   // O == Проекты
        "P",   // M Счетчики
        "Q",   // M Выключатели
        "R",   // M Резисторы
        "S",   // M Переключатели
        "T",   // M Трансформаторы
        "U",   // M Выпрямители
        "V",   // M Диоды, тиристоры, транзисторы
        "W",   // M Антенны
        "X",   // M Гнезда
        "Y",   // M Электромагнитный привод
        "Z"
};

static const std::string symDescriptions[LOCALES][MEASURE_COUNT] {
        "Devices",          // A
        "Mics",             // B
        "C",                // C
        "IC схемы",         // D
        "Etc",              // E
        "Fuses",            // F
        "Gen",              // G
        "Ind",              // H
        "Bolts",            // I == Крепления
        "Wires",            // J == Провода
        "Relay",           // K
        "L",               // L
        "Motors",          // M
        "Garbage",         // N == На выброс
        "Projects",        // O == Проекты
        "Counters",        // P
        "Switches",        // Q
        "R",               // R
        "S",               // S
        "Trans",           // T
        "Rect",            // U
        "Diodes, Transistors",   // V
        "Ant",             // W
        "Nests",           // X
        "Actuators",       // Y
        "Quartz",          // Z

        "Устройства",           // A
        "Микрофоны, динамики",  // B
        "Конденсаторы",         // C
        "Интегральные схемы",   // D
        "Разные элементы",      // E
        "Предохранители",       // F
        "Источники питания",    // G
        "Индикаторы",           // H
        "Крепления",            // I == Крепления
        "Провода",              // J == Провода
        "Реле",                // K
        "Дроссели",            // L
        "Двигатели",           // M
        "На выброс",           // N == На выброс
        "Проекты",             // O == Проекты
        "Счетчики",            // P
        "Выключатели",         // Q
        "Резисторы",           // R
        "Переключатели",       // S
        "Трансформаторы",      // T
        "Выпрямители",         // U
        "Диоды, транзисторы",  // V
        "Антенны",             // W
        "Гнезда",              // X
        "Электромагнитные приводы", // Y
        "Кварцевые фильтры"    // Z
};

static const std::string unitNames[LOCALES][MEASURE_COUNT] {
        "",    // A Устройства
        "",    // B Микрофоны, громкоговорители
        "F",   // C Конденсаторы
        "",    // D Интегральные схемы
        "",    // E Разные элементы
        "A",   // F Плавкие предохранители
        "V",   // G Источники питания
        "",    // H Индикаторы
        "",    // I == Крепления
        "",    // J == Провода
        "",   // K Реле
        "G",  // L Дроссели
        "W",  // M Двигатели
        "",   // N == На выброс
        "",   // O == Проекты
        "",   // P Счетчики
        "A",  // Q Выключатели
        "Ohm",// R Резисторы
        "A",  // S Переключатели
        "W",  // T Трансформаторы
        "",   // U Выпрямители
        "",   // V Диоды, тиристоры, транзисторы
        "",   // W Антенны
        "",   // X Гнезда
        "",   // Y Электромагнитный привод
        "",   // Z Кварцевые фильтры

        "",     // A Устройства
        "",     // B Микрофоны, громкоговорители
        "Ф",    // C Конденсаторы
        "",     // D Интегральные схемы
        "",     // E Разные элементы
        "А",    // F Плавкие предохранители
        "В",    // G Источники питания
        "",     // H Индикаторы
        "",     // I == Крепления
        "",     // J == Провода
        "",     // K Реле
        "Гн",  // L Дроссели
        "Вт",  // M Двигатели
        "",    // N == На выброс
        "",    // O == Проекты
        "",    // P Счетчики
        "А",   // Q Выключатели
        "Ом",  // R Резисторы
        "А",   // S Переключатели
        "Вт",  // T Трансформаторы
        "",    // U Выпрямители
        "",    // V Диоды, тиристоры, транзисторы
        "",    // W Антенны
        "",    // X Гнезда
        "",    // Y Электромагнитный привод
        ""     // Z Кварцевые фильтры
};

static const std::string unitNamesUpperCase[LOCALES][MEASURE_COUNT] {
        "",    // A Устройства
        "",    // B Микрофоны, громкоговорители
        "F",   // C Конденсаторы
        "",    // D Интегральные схемы
        "",    // E Разные элементы
        "",    // F Плавкие предохранители
        "",    // G Источники питания
        "",    // H Индикаторы
        "",    // I == Крепления
        "",    // J == Провода
        "",   // K Реле
        "G",  // L Дроссели
        "",   // M Двигатели
        "",   // N == На выброс
        "",   // O == Проекты
        "",   // P Счетчики
        "",   // Q Выключатели
        "OHM",// R Резисторы
        "",   // S Переключатели
        "",   // T Трансформаторы
        "",   // U Выпрямители
        "",   // V Диоды, тиристоры, транзисторы
        "",   // W Антенны
        "",   // X Гнезда
        "",   // Y Электромагнитный привод
        "",   // Z Кварцевые фильтры

        "",    // A Устройства
        "",    // B Микрофоны, громкоговорители
        "Ф",   // C Конденсаторы
        "",    // D Интегральные схемы
        "",    // E Разные элементы
        "",    // F Плавкие предохранители
        "",    // G Источники питания
        "",    // H Индикаторы
        "",    // I == Крепления
        "",    // J == Провода
        "",   // K Реле
        "ГН", // L Дроссели
        "",   // M Двигатели
        "",   // N == На выброс
        "",   // O == Проекты
        "",   // P Счетчики
        "",   // Q Выключатели
        "ОМ", // R Резисторы
        "",   // S Переключатели
        "",   // T Трансформаторы
        "",   // U Выпрямители
        "",   // V Диоды, тиристоры, транзисторы
        "",   // W Антенны
        "",   // X Гнезда
        "",   // Y Электромагнитный привод
        ""    // Z Кварцевые фильтры
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
    // if component name starting with digit, nominal is not 0, in this case reset mominal
    nominal = 0;
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

bool MeasureUnit::hasNominal(COMPONENT value) {
    return value == COMPONENT_C ||    // Конденсаторы
        value == COMPONENT_F ||       // Плавкие предохранители
        value == COMPONENT_G ||       // Источники питания
        value == COMPONENT_L ||       // Дроссели
        value == COMPONENT_M ||       // Двигатели
        value == COMPONENT_Q ||       // Выключатели
        value == COMPONENT_R ||       // Резисторы
        value == COMPONENT_S ||       // Переключатели
        value == COMPONENT_T;         // Трансформаторы
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
