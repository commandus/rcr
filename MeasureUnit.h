//
// Created by andrei on 10.03.23.
//

#ifndef RCR_MEASUREUNIT_H
#define RCR_MEASUREUNIT_H

#include <string>
#include <vector>

typedef enum {
    ML_INTL = 0,     ///< international
    ML_RU = 1        ///< Russian
} MEASURE_LOCALE;

#define MEASURE_COUNT 22
#define MAX_POW10 11

typedef enum {
    COMPONENT_A,    // Устройства
    COMPONENT_B,    // Микрофоны, громкоговорители
    COMPONENT_C,    // Конденсаторы
    COMPONENT_D,    // Интегральные схемы
    COMPONENT_E,    // Разные элементы
    COMPONENT_F,    // Плавкие предохранители
    COMPONENT_G,    // Источники питания
    COMPONENT_H,    // Индикаторы
    COMPONENT_K,    // Реле
    COMPONENT_L,    // Дроссели
    COMPONENT_M,    // Двигатели
    COMPONENT_P,    // Счетчики
    COMPONENT_Q,    // Выключатели
    COMPONENT_R,    // Резисторы
    COMPONENT_S,    // Переключатели
    COMPONENT_T,    // Трансформаторы
    COMPONENT_U,    // Выпрямители
    COMPONENT_V,    // Диоды, тиристоры, транзисторы
    COMPONENT_W,    // Антенны
    COMPONENT_X,    // Гнезда
    COMPONENT_Y,    // Электромагнитный привод
    COMPONENT_Z     // Кварцевые фильтры
} COMPONENT;

#define FLAG_COMPONENT(X)   (1 << (int) X)
#define FLAG_ALL_COMPONENTS (1 << (((int) COMPONENT_Z) + 1) - 1)

/**
 * Flags is an component set
 * @param flags
 * @return first found in the set (flags) component id
 */
COMPONENT firstComponentInFlags(uint32_t flags);

class MeasureUnit {
public:
    static std::string sym(
        COMPONENT measure
    );
    static std::string description(
        MEASURE_LOCALE locale,
        COMPONENT measure
    );
    static std::string unit(
        MEASURE_LOCALE locale,
        COMPONENT measure
    );
    static int pow10(
            COMPONENT measure
    );
    static std::string value(
        MEASURE_LOCALE locale,
        COMPONENT measure,
        uint64_t val
    );
    static double val(
        COMPONENT measure,
        uint64_t value
    );
    static int parse(
        MEASURE_LOCALE locale,
        const std::string &value,
        size_t &position,
        uint64_t &nominal,
        COMPONENT &measure,
        std::string &retname,
        COMPONENT param
    );
};

// Helper functions

/**
 * Convert string to measure locale enum
 * @param value "intl" or "ru"
 * @return ML_RU bu default
 */
MEASURE_LOCALE pchar2MEASURE_LOCALE(const char *value);

void listUnitNParticle(
    std::vector<std::string> &retVal,
    MEASURE_LOCALE locale,
    COMPONENT component
);

/**
 * Return COMPONENT_A by default
 * @param symbol "D" for IC
 * @return
 */
COMPONENT getComponentBySymbol(
    const std::string &symbol,
    const COMPONENT defaultComponent = COMPONENT_A
);

#endif //RCR_MEASUREUNIT_H
