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

typedef enum {
    M_A,    // Устройства
    M_B,    // Микрофоны, громкоговорители
    M_C,    // Конденсаторы
    M_D,    // Интегральные схемы
    M_E,    // Разные элементы
    M_F,    // Плавкие предохранители
    M_G,    // Источники питания
    M_H,    // Индикаторы
    M_K,    // Реле
    M_L,    // Дроссели
    M_M,    // Двигатели
    M_P,    // Счетчики
    M_Q,    // Выключатели
    M_R,    // Резисторы
    M_S,    // Переключатели
    M_T,    // Трансформаторы
    M_U,    // Выпрямители
    M_V,    // Диоды, тиристоры, транзисторы
    M_W,    // Антенны
    M_X,    // Гнезда
    M_Y,    // Электромагнитный привод
    M_Z     // Кварцевые фильтры
} MEASURE;

class MeasureUnit {
public:
    static std::string sym(
        MEASURE_LOCALE locale,
        MEASURE measure
    );
    static std::string description(
        MEASURE_LOCALE locale,
        MEASURE measure
    );
    static std::string unit(
        MEASURE_LOCALE locale,
        MEASURE measure
    );
    static int pow10(
        MEASURE measure
    );
    static std::string value(
        MEASURE_LOCALE locale,
        MEASURE measure,
        uint64_t val
    );
    static double val(
        MEASURE measure,
        uint64_t value
    );
    static int
    parse(
        MEASURE_LOCALE locale,
        const std::string &value,
        size_t &position,
        uint64_t &nominal,
        MEASURE &measure,
        std::string &retname,
        MEASURE param
    );
};

// Helper functions

/**
 * Convert string to measure locale enum
 * @param value "intl" or "ru"
 * @return ML_RU bu default
 */
MEASURE_LOCALE pchar2MEASURE_LOCALE(const char *value);

#endif //RCR_MEASUREUNIT_H
