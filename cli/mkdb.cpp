/**
 * ./mkdb connection-string
 */
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include <third-party/argtable3/argtable3.h>
#include "gen/rcr.pb.h"
#include "gen/rcr.pb-odb.hxx"

#include <odb/schema-catalog.hxx>
#include <odb/sqlite/database.hxx>

#include "MeasureUnit.h"
#include "sqlite-helper.h"

// i18n
#include <libintl.h>
#define _(String) gettext (String)

const char* progname = "mkdb";
const char* DEF_CONNECTION = "rcr.db";
const MEASURE_LOCALE DEF_LOCALE = ML_RU;

class ClientConfig {
public:
    bool cleanup;
    int verbosity;                ///< verbose level: 0- error only, 1- warning, 2- info, 3- debug
    std::string connection;
    MEASURE_LOCALE locale;
    ClientConfig()
        : cleanup(false), verbosity(0), locale(ML_RU)
    {

    }
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
	struct arg_str *a_connection = arg_str0("c", "connection", _("<connection-string>"),
        _("Set database connection string"));
    struct arg_str *a_locale = arg_str0("l", "locale", "intl|ru",
        _("Set locale. Default ru"));
    struct arg_lit *a_clean = arg_lit0("0", "clean", _("Clean existing database"));
	struct arg_lit *a_verbose = arg_litn("v", "verbose", 0, 5, _("Verbose level"));
	struct arg_lit *a_help = arg_lit0("h", "help", _("Show this help"));
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = { a_connection, a_locale, a_clean, a_verbose, a_help, a_end };
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
		std::cerr << _("Usage: ") <<  progname << std::endl;
		arg_print_syntax(stdout, argtable, "\n");
        std::cerr << _("make database utility") << std::endl;
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
	value.cleanup = a_clean->count;
    value.verbosity = a_verbose->count;
	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 0;
}

/**
 * Open file to read, skip UTF-8 BOM if exists.
 */
std::ifstream *openUtf8BOM(
    const std::string &fn
)
{
	std::ifstream *ret = new std::ifstream(fn, std::ifstream::in);
	// remove byte order mark (BOM) 0xef 0xbb 0xbf
	unsigned char bom[3];
	ret->read((char*) bom, 3);
	if (!((bom[0] == 0xef) && (bom[1] == 0xbb) && (bom[2] == 0xbf)))
		ret->seekg(0);
	return ret;
}

int main(int argc, char** argv) {
    // I18N
    setlocale(LC_ALL, "");
    // bindtextdomain(progname, "/usr/share/locale");
    // bind_textdomain_codeset(progname, "UTF-8");
    textdomain(progname);

    ClientConfig config;
	int r;
	if (r = parseCmd(argc, argv, config))
		exit(r);

	if (config.verbosity > 1) {
	}
#ifdef ENABLE_SQLITE
    if (config.cleanup)
        sqliteClean(config.connection);
    else {
        sqliteCreateSchemaIfExists(config.connection, config.locale);
        std::cout << _("Admin user SYSDBA created with password \"masterkey\"") << std::endl;
    }
#else
    std::cerr << _("Database is not supported") << std::endl;
#endif

    return 0;
}
