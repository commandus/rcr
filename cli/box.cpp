/**
 * box
 */
#include <iostream>
#include <string>

#include "argtable3/argtable3.h"

#include "StockOperation.h"

// i18n
#include <libintl.h>
#define _(String) gettext (String)

const char* progname = "box";

class BoxConfig {
public:
    std::string value;
    bool noEol;
    bool reverse;
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
    BoxConfig *value
)
{
    struct arg_str *a_value = arg_str1(nullptr, nullptr, _("<box path>"), _("print box number"));
    struct arg_lit *a_no_eol = arg_lit0("n", nullptr, _("No output end of line"));
    struct arg_lit *a_reverse = arg_lit0("r", nullptr, _("print box path for number"));
    struct arg_lit *a_help = arg_lit0("h", "help", _("Show this help"));
    struct arg_end *a_end = arg_end(20);

    void* argtable[] = { a_value, a_reverse, a_no_eol, a_help, a_end };

    int nerrors;

    // verify the argtable[] entries were allocated successfully
    if (arg_nullcheck(argtable) != 0) {
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        return 1;
    }
    // Parse the command line as defined by argtable[]
    nerrors = arg_parse(argc, argv, argtable);

    // special case: '--help' takes precedence over error reporting
    if ((a_help->count) || nerrors) {
        if (nerrors)
            arg_print_errors(stderr, a_end, progname);
        std::cerr << _("Usage: ") << progname << std::endl;
        arg_print_syntax(stderr, argtable, "\n");
        std::cerr << _("Print box path from box identifier and vice versa") << std::endl;
        arg_print_glossary(stderr, argtable, "  %-25s %s\n");
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        return 1;
    }

    value->value = *a_value->sval;
    value->noEol = a_no_eol->count > 0;
    value->reverse = a_reverse->count > 0;

    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return 0;
}

int main(int argc, char** argv) {
    // I18N
    setlocale(LC_ALL, "");
    // bindtextdomain(progname, "/usr/share/locale");
    // bind_textdomain_codeset(progname, "UTF-8");
    textdomain(progname);

    BoxConfig config;
    int r;
    if (r = parseCmd(argc, argv, &config))
        exit(r);

    if (config.reverse) {
        uint64_t boxId = strtoull(config.value.c_str(), nullptr, 10);
        std::cout << StockOperation::boxes2string(boxId);
    } else {
        uint64_t boxId;
        int l = StockOperation::parseBoxes(boxId, config.value, 0, config.value.size());
        std::cout << boxId;
    }
    if (!config.noEol)
        std::cout << std::endl;

    return 0;
}
