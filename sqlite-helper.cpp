//
// Created by andrei on 14.03.23.
//
#include "sqlite-helper.h"
#include <odb/schema-catalog.hxx>
#include <odb/sqlite/database.hxx>

#include "gen/rcr.pb.h"
#include "gen/rcr.pb-odb.hxx"

// i18n
#include <libintl.h>
#define _(String) gettext (String)

#define CLEAN_STMT_SIZE 10

static const char* CLEAN_STMT[CLEAN_STMT_SIZE] {
    "DELETE FROM 'Card'",
    "DELETE FROM 'Property'",
    "DELETE FROM 'Package'",
    "DELETE FROM 'Box'",
    "DELETE FROM 'Journal'",
    "UPDATE sqlite_sequence SET seq = 1 WHERE name = 'Card'",
    "UPDATE sqlite_sequence SET seq = 1 WHERE name = 'Property'",
    "UPDATE sqlite_sequence SET seq = 1 WHERE name = 'Package'",
    "UPDATE sqlite_sequence SET seq = 1 WHERE name = 'Box'",
    "UPDATE sqlite_sequence SET seq = 1 WHERE name = 'Journal'"
};

static int sqlite3Callback(
    void *env,
    int columns,
    char **value,
    char **column
)
{
    if (!env)
        return 0;
    std::vector<std::vector<std::string>> *retval = (std::vector<std::vector<std::string>> *) env;
    std::vector<std::string> line;
    for (int i = 0; i < columns; i++) {
        line.push_back(value[i] ? value[i] : "");
    }
    retval->push_back(line);
    return 0;
}

static int execSqliteStmt(
        sqlite3 *dbSqlite3,
        const char *statement
)
{
    char *zErrMsg = nullptr;
    int r = sqlite3_exec(dbSqlite3, statement, sqlite3Callback, nullptr, &zErrMsg);
    if (r != SQLITE_OK) {
        if (zErrMsg) {
            std::cerr << zErrMsg << std::endl;
        }
    }
    return r;
}

int sqliteClean(
    const std::string &db
)
{
    sqlite3 *dbSqlite3;
    int r = sqlite3_open(db.c_str(), &dbSqlite3);
    if (r)
        return r;
    for (int i = 0; i < CLEAN_STMT_SIZE; i++) {
        std::cout << CLEAN_STMT[i] << std::endl;
        r = execSqliteStmt(dbSqlite3, CLEAN_STMT[i]);
        if (r) {
            sqlite3_close(dbSqlite3);
            return r;
        }
    }
    r = sqlite3_close(dbSqlite3);
    return r;
}

static bool sqliteFillOutDatabase(
        MEASURE_LOCALE locale,
        odb::sqlite::database &db
)
{
    bool r;
    try {
        for (COMPONENT v = COMPONENT_A; v <= COMPONENT_Z;) {
            rcr::Symbol symbol;
            symbol.set_sym(MeasureUnit::sym(v));
            symbol.set_description(MeasureUnit::description(locale, v));
            symbol.set_unit(MeasureUnit::unit(locale, v));
            symbol.set_pow10(MeasureUnit::pow10(v));
            db.persist(symbol);
            v = (COMPONENT) ((int) v + 1);
        }
        rcr::Operation operation;
        operation.set_symbol("+");
        operation.set_description("");
        db.persist(operation);
        operation.set_symbol("-");
        operation.set_description("");
        db.persist(operation);
        operation.set_symbol("=");
        operation.set_description("");
        db.persist(operation);
        operation.set_symbol("/");
        operation.set_description("");
        db.persist(operation);

        rcr::SymbolProperty symbolProperty;
        // ABCDEFGHIJKLMNOPQRSTUVWXYZ
        // 12345678901234567890123456
        //          1         2
        symbolProperty.set_symbol_id(3);        // C
        symbolProperty.set_property_type_id(1); // K
        db.persist(symbolProperty);

        symbolProperty.set_symbol_id(3);        // C
        symbolProperty.set_property_type_id(2); // V
        db.persist(symbolProperty);

        symbolProperty.set_symbol_id(4);        // D
        symbolProperty.set_property_type_id(1); // K
        db.persist(symbolProperty);

        symbolProperty.set_symbol_id(6);        // F
        symbolProperty.set_property_type_id(1); // K
        db.persist(symbolProperty);

        symbolProperty.set_symbol_id(6);        // F
        symbolProperty.set_property_type_id(3); // A
        db.persist(symbolProperty);

        symbolProperty.set_symbol_id(18);        // R
        symbolProperty.set_property_type_id(4); // P
        db.persist(symbolProperty);

        symbolProperty.set_symbol_id(18);        // R
        symbolProperty.set_property_type_id(5);  // %
        db.persist(symbolProperty);

        symbolProperty.set_symbol_id(22);        // V
        symbolProperty.set_property_type_id(1);  // K
        db.persist(symbolProperty);

        rcr::PropertyType propertyType;

        propertyType.set_key("K");
        propertyType.set_description(_("housing")); // 1
        db.persist(propertyType);

        propertyType.set_key("V");
        propertyType.set_description(_("voltage")); // 2
        db.persist(propertyType);

        propertyType.set_key("A");
        propertyType.set_description(_("current")); // 3
        db.persist(propertyType);

        propertyType.set_key("P");
        propertyType.set_description(_("power"));   // 4
        db.persist(propertyType);

        propertyType.set_key("%");
        propertyType.set_description(_("precision"));   // 5
        db.persist(propertyType);

        propertyType.set_key("D");
        propertyType.set_description(_("description")); // 6
        db.persist(propertyType);

        // user
        rcr::User user;
        user.set_name("SYSDBA");
        user.set_password("masterkey");
        user.set_rights(1);
        db.persist(user);

        // group
        rcr::Group group;
        group.set_name("Group 1");
        db.persist(group);

        // groupuser
        rcr::GroupUser groupUser;
        groupUser.set_group_id(1);
        groupUser.set_user_id(1);
        db.persist(groupUser);

        r = true;
    } catch (odb::exception &ex) {
        std::cerr << _("Error filling schema :") << ex.what() << std::endl;
        r = false;
    } catch (...) {
        std::cerr << _("Unknown error filling schema") << std::endl;
    }

    return r;
}

bool sqliteCreateSchemaIfExists(
    const std::string &dbFileName,
    MEASURE_LOCALE locale
)
{
    auto db = new odb::sqlite::database(dbFileName, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE);
    odb::transaction t(db->begin());
    bool exists;
    bool created;
    try {
        db->query<rcr::Card>(odb::query<rcr::Card>::id == 0);
        exists = true;
    } catch (...) {
        exists = false;
    }
    if (!exists) {
        try {
            odb::schema_catalog::create_schema(*db);
        } catch (odb::exception &ex) {
            std::cerr << _("Error create schema: ") << ex.what() << std::endl;
        } catch (...) {
            std::cerr << _("Unknown error create schema") << std::endl;
        }
    }

    created = sqliteFillOutDatabase(locale, *db);

    t.commit();
    delete db;
    return created;
}
