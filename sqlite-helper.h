#ifndef RCR_SQLITE_HELPER_H
#define RCR_SQLITE_HELPER_H

#include <string>
#include <vector>
#include <sqlite3.h>

#include "MeasureUnit.h"

int sqliteClean(
    const std::string &db
);

bool sqliteCreateSchemaIfExists(
    const std::string &db,
    MEASURE_LOCALE locale
);

#endif
