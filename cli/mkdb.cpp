/**
 * ./mkdb connection-string
 */
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <fstream>

#include <third-party/argtable3/argtable3.h>
#include "gen/rcr.pb.h"
#include "gen/rcr.pb-odb.hxx"

#include <odb/query.hxx>
#include <odb/schema-catalog.hxx>
#include <odb/sqlite/database.hxx>

#include "MeasureUnit.h"


const char* progname = "mkdb";
const char* DEF_CONNECTION = "rcr.db";
const MEASURE_LOCALE DEF_LOCALE = ML_RU;

class ClientConfig {
public:
    int verbosity;                ///< verbose level: 0- error only, 1- warning, 2- info, 3- debug
    std::string connection;
    MEASURE_LOCALE locale;
};

/**
 * Parse command line into struct ClientConfig
 * Return 0- success
 *        1- show help and exit, or command syntax error
 *        2- output file does not exists or can not open to write
 **/
int parseCmd(
	int argc,
	char* argv[],
	ClientConfig &value
)
{
	struct arg_str *a_connection = arg_str0("c", "connection", "<connection-string>",
            "Set database connection string");
    struct arg_str *a_locale = arg_str0("l", "locale", "intl|ru",
                                            "Set locale. Default ru");
    struct arg_lit *a_verbose = arg_litn("v", "verbose", 0, 5, "Verbose level");
	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = { a_connection, a_locale, a_verbose, a_help, a_end };
	int nerrors;

	// verify the argtable[] entries were allocated successfully
	if (arg_nullcheck(argtable) != 0) {
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return 1;
	}
	// Parse the command line as defined by argtable[]
	nerrors = arg_parse(argc, argv, argtable);

	// special case: '--help' takes precedence over error reporting
	if ((a_help->count) || nerrors)	{
		if (nerrors)
			arg_print_errors(stderr, a_end, progname);
		std::cerr << "Usage: " <<  progname << std::endl;
		arg_print_syntax(stdout, argtable, "\n");
        std::cerr << "make database utility" << std::endl;
		arg_print_glossary(stdout, argtable, "  %-25s %s\n");
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return 1;
	}
	if (a_connection->count)
		value.connection = *a_connection->sval;
	else
		value.connection = DEF_CONNECTION;
    if (a_locale->count)
        value.locale = pchar2MEASURE_LOCALE(*a_locale->sval);
    else
        value.locale = DEF_LOCALE;
	value.verbosity = a_verbose->count;
	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 0;
}

/**
 * Open file to read, skip UTF-8 BOM if exists.
 */
std::ifstream *openUtf8BOM(const std::string &fn)
{
	std::ifstream *ret = new std::ifstream(fn, std::ifstream::in);
	// remove byte order mark (BOM) 0xef 0xbb 0xbf
	unsigned char bom[3];
	ret->read((char*) bom, 3);
	if (!((bom[0] == 0xef) && (bom[1] == 0xbb) && (bom[2] == 0xbf)))
		ret->seekg(0);
	return ret;
}

static bool sqliteFillOutDatabase(MEASURE_LOCALE locale, odb::sqlite::database &db)
{
    bool r;
    try {
        MeasureUnit unit;
        rcr::Symbol symbol;
        symbol.set_sym(unit.sym(locale, M_R));
        symbol.set_unit(unit.unit(locale, M_R));
        symbol.set_pow10(unit.pow10(M_R));
        db.persist(symbol);
        symbol.set_sym(unit.sym(locale, M_C));
        symbol.set_unit(unit.unit(locale, M_C));
        symbol.set_pow10(unit.pow10(M_C));
        db.persist(symbol);
        symbol.set_sym(unit.sym(locale, M_L));
        symbol.set_unit(unit.unit(locale, M_L));
        symbol.set_pow10(unit.pow10(M_L));
        db.persist(symbol);

        symbol.set_sym(unit.sym(locale, M_U));
        symbol.set_unit(unit.unit(locale, M_U));
        symbol.set_pow10(unit.pow10(M_U));
        db.persist(symbol);

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

        rcr::PropertyType propertyType;
        propertyType.set_key("K");
        propertyType.set_description("корпус");
        db.persist(propertyType);

        r = true;
    } catch (odb::exception &ex) {
        std::cerr << "Error filling schema : " << ex.what() << std::endl;
        r = false;
    } catch (...) {
        std::cerr << "Unknown error filling schema" << std::endl;
    }

    return r;
}

static bool sqliteCreateSchemaIfExists(ClientConfig &config)
{
    auto db = new odb::sqlite::database(config.connection, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE);
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
            std::cerr << "Error create schema " << ex.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown error create schema" << std::endl;
        }
    }

    created = sqliteFillOutDatabase(config.locale, *db);

    t.commit();
    delete db;
    return created;
}

int main(int argc, char** argv)
{
	ClientConfig config;
	int r;
	if (r = parseCmd(argc, argv, config))
		exit(r);

	if (config.verbosity > 1) {
	}
#ifdef ENABLE_SQLITE
    sqliteCreateSchemaIfExists(config);
#else
#endif
	return 0;
}
