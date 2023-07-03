/**
 * rcr-cli
 *
 * Compile:
 * g++ -o rcr-cli -I.. -I../third-party ../cli/rcr-cli.cpp ../cli/grpcClient.cpp ../RcrCredentials.cpp ../AppSettings.cpp ../gen/rcr.grpc.pb.cc ../gen/rcr.pb.cc CMakeFiles/rcr-cli.dir/third-party/argtable3/argtable3.c.o -lgrpc++ -lprotobuf
 */
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "argtable3/argtable3.h"
#include <grpc++/grpc++.h>

// i18n
#include <libintl.h>
#define _(String) gettext (String)

#include "grpcClient.h"

#include "AppSettings.h"
#include "RcrCredentials.h"
#include "BoxName.h"
#include "StockOperation.h"
#include "utilfile.h"
#include "string-helper.h"
#include "utilstring.h"

const char* progname = "rcr-cli";
const char* DEF_COMMAND = "stream-query";

#define DEF_PORT		        50051
#define DEF_ADDRESS			    "127.0.0.1"

#define HELP_STRING _("help               Help screen\n\
quit               Exit\n\
symbol             Component type and symbol list\n\
symbol *           Set all component types\n\
symbol D           Set component symbol D (integrated circuits)\n\
box                Box list\n\
box + 21-1 spare   Add box 21-1 named \"spare\"\n\
box - 21-1         Remove box\n\
box = 21-1 newname Change box 21-1\n\
box / 21-1 22      Rename box 21-1 to 22\n\
box / 1-1 2 new    Rename box 1-1 to 2 and assign a name\n\
property           Properties list\n\
property = A accu  Set property accu with shortkey A\n\
property + B bat   Add property\n\
property - A       Remove property\n\
user               Registered users\n\
import <path>      Preview spreadsheets in the path\n\
import <path> R 42 Import resistors (symbol R) from spreadsheets in the path to box 42\n\
.. no-num       Do not read box number from Excel file name\n\
        D*                 Search by name starting with 'D'\n\
100 kOhm           Search resistors by nominal\n\
1 mF 119           Search capacitor by nominal in boxes 119-..\n\
* 119              Search all in boxes 119-..\n\
* 119-1 K:DIP      Search all in box 119-1 DIP\n")

class ClientConfig {
public:
    std::string intface;
    int port;
    bool sslOn;
    int repeats;
    int verbose;

    MEASURE_LOCALE locale;
    std::string command;
    std::string request;
    size_t offset;
    size_t size;
    std::string username;
    std::string password;
    std::string box;
    std::string componentSymbol;
    bool numberInFileName;
};

size_t findFiles(
    std::vector<std::string> &retVal,
    const std::string &path
) {
    return util::filesInPath(path, ".xlsx", 0, &retVal);
}

void printSpreadSheets(
    const std::string &path,
    const std::string &boxName,
    int verbosity,
    bool numberInFilename
) {
    std::vector<std::string> spreadSheets;
    findFiles(spreadSheets, path);
    std::cout << spreadSheets.size() << _(" *.xlsx files found in '") << path << "'" << std::endl;
    for (auto it = spreadSheets.begin(); it != spreadSheets.end(); it++) {
        uint64_t box;
        if (numberInFilename)
            box = BoxName::extractFromFileName(boxName + " " + *it); //  <- add if filename contains boxes
        else
            box = BoxName::extractFromFileName(boxName);
        SpreadSheetHelper spreadSheet(*it);
        if (verbosity) {
            std::cout << *it << ": " << spreadSheet.items.size() << _(" names, ") << spreadSheet.total << _(" items") << std::endl;
            for (auto bx = spreadSheet.boxItemCount.begin(); bx != spreadSheet.boxItemCount.end(); bx++) {
                if (bx->second) {
                    std::cout << StockOperation::boxes2string(StockOperation::boxAppendBox(box, bx->first)) << ": "
                          << bx->second << "\t";
                }
            }
            std::cout << std::endl;
            if (verbosity > 1) {
                // print data itself
                for (auto item = spreadSheet.items.begin(); item != spreadSheet.items.end(); item++) {
                    std::cout
                        << StockOperation::boxes2string(StockOperation::boxAppendBox(box, item->id))
                        << "\t" << item->name << "\t" << item->qty << "\t" << item->remarks << "\t"
                        << std::endl;
                }
            }
        }
    }
}

void importSpreadSheets(
    RcrClient &rpc,
    const std::string &path,
    const std::string &symbol,
    const std::string &boxName,
    bool numberInFilename
) {
    std::vector<std::string> spreadSheets;
    findFiles(spreadSheets, path);
    for (auto it = spreadSheets.begin(); it != spreadSheets.end(); it++) {
        uint64_t box;
        if (numberInFilename)
            box = BoxName::extractFromFileName(boxName + " " + *it); //  <- add if filename contains boxes
        else
            box = BoxName::extractFromFileName(boxName);
        SpreadSheetHelper spreadSheet(*it);
        // component symbol xlsx-add-u -> U xlsx-add-r -> R xlsx-add-c -> C xlsx-add-l -> L
        int r = rpc.saveSpreadsheet(box, symbol, spreadSheet.items);
        if (r)
            break;
    }
}

/**
 * Parse command line into struct ClientConfig
 * Return 0- success
 *        1- show help and exit, or command syntax error
 *        2- output file does not exists or can not open to write
 **/
int parseCmd
(
	int argc,
	char* argv[],
	ClientConfig *value
)
{
	struct arg_str *a_interface = arg_str0("i", "hostname", _("<service host name>"), _("service host name or IP address."));
	struct arg_int *a_port = arg_int0("l", "listen", "<port>", _("service port. Default 50051"));
	// SSL
	struct arg_lit *a_sslon = arg_lit0("s", "sslOn", _("SSL on"));
	// commands
	// struct arg_file *a_niceclassfn = arg_file0(nullptr, "class", "<file>", "add NICE classes from JSON file");
	struct arg_str *a_command = arg_str0(nullptr, nullptr, _("<command>"),
            "card|box|login|boxes|users|dictionaries|xlsx|xlsx-list|xlsx-add-u");
    struct arg_str *a_request = arg_str0(nullptr, nullptr, _("<params>"), _("command parameter(params)"));
    struct arg_str *a_user_name = arg_str0("u", "user", _("<user-name>"), _("User login"));
    struct arg_str *a_user_password = arg_str0("p", "password", _("<password>"), _("User password"));
    struct arg_str *a_box = arg_str0("b", "box", _("<box>"), _("box prefix e.g. 221-2"));
    struct arg_int *a_offset = arg_int0("o", "offset", _("<number>"), _("List offset 0.. Default 0"));
    struct arg_int *a_size = arg_int0("s", "size", _("<number>"), _("List size. Default 10000"));

    struct arg_lit *a_no_number_in_filename = arg_lit0(nullptr, "no_box_number_in_filename", _("do not read box from Excel file name"));
    struct arg_str *a_locale = arg_str0("l", "locale", _("<locale>"), _("Locale: intl, ru"));

	struct arg_lit *a_verbose = arg_litn("v", "verbose", 0, 5, _("Verbose level"));

	struct arg_lit *a_help = arg_lit0("h", "help", _("Show this help"));
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = { a_interface, a_port, a_sslon,
        a_command, a_request, a_box, a_user_name, a_user_password,
        a_offset, a_size,
		a_locale, a_verbose,
		a_help, a_end };

	int nerrors;

	// verify the argtable[] entries were allocated successfully
	if (arg_nullcheck(argtable) != 0) {
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return 1;
	}
	// Parse the command line as defined by argtable[]
	nerrors = arg_parse(argc, argv, argtable);

	// special case: '--help' takes precedence over error reporting
	if ((a_help->count) || nerrors)
	{
		if (nerrors)
			arg_print_errors(stderr, a_end, progname);
		std::cerr << _("Usage: ") << progname << std::endl;
		arg_print_syntax(stderr, argtable, "\n");
		std::cerr << _("rcr CLI client") << std::endl;
		arg_print_glossary(stderr, argtable, "  %-25s %s\n");
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return 1;
	}

    value->componentSymbol = "D";   // IC

	if (a_interface->count)
		value->intface = *a_interface->sval;
	else
		value->intface = DEF_ADDRESS;

	if (a_port->count)
		value->port = *a_port->ival;
	else
		value->port = DEF_PORT;

	// SSL
	value->sslOn = a_sslon->count > 0;

    if (a_command->count)
        value->command = *a_command->sval;
    else
        value->command = DEF_COMMAND;
    if (a_request->count)
        value->request = *a_request->sval;
    else
        value->request = "";
    if (a_box->count)
        value->box = *a_box->sval;
    else
        value->box = "";

    if (a_user_name->count)
        value->username = *a_user_name->sval;
    else
        value->username = "";
    if (a_user_password->count)
        value->password = *a_user_password->sval;
    else
        value->password = "";

    if (a_offset->count)
        value->offset = *a_offset->ival;
    else
        value->offset = 0;

    if (a_size->count)
        value->size = *a_offset->ival;
    else
        value->size = 10000;

    if (a_no_number_in_filename->count)
        value->numberInFileName = false;
    else
        value->numberInFileName = true;

	value->verbose = a_verbose->count;

    if (a_locale->count)
        value->locale = pchar2MEASURE_LOCALE(*a_locale->sval);
    else
        value->locale = ML_INTL;

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 0;
}

static std::string nextWord(
    const std::string &line,
    size_t &start
) {
    size_t eolp = line.size();
    size_t finish = eolp;
    // skip spaces
    for (auto p = start; p < eolp; p++) {
        if (!std::isspace(line[p])) {
            start = p;
            break;
        }
    }
    // try read word
    for (auto p = start; p < eolp; p++) {
        if (std::isspace(line[p])) {
            finish = p;
            break;
        }
    }
    std::string r = line.substr(start, finish - start);
    start = finish;
    return r;
}

static std::string remainText(
    const std::string &line,
    size_t &start
) {
    size_t eolp = line.size();
    size_t finish = eolp;
    // skip spaces
    for (auto p = start; p < eolp; p++) {
        if (!std::isspace(line[p])) {
            start = p;
            break;
        }
    }
    // try read to the end
    std::string r = line.substr(start, eolp - start);
    start = finish;
    return r;
}

int main(int argc, char** argv)
{
    // I18N
    setlocale(LC_ALL, "");
    // bindtextdomain(progname, "/usr/share/locale");
    // bind_textdomain_codeset(progname, "UTF-8");
    textdomain(progname);

	ClientConfig config;
	int r;
	if (r = parseCmd(argc, argv, &config))
		exit(r);

    // target host name and port
    std::stringstream ss;
    ss << config.intface << ":" << config.port;
    std::string target(ss.str());

    std::shared_ptr<grpc::Channel> channel;
    if (config.sslOn) {
        grpc::ChannelArguments args;
        args.SetSslTargetNameOverride("avtovokzal-yakutsk.ru");

        grpc::SslCredentialsOptions sslOpts;
        sslOpts.pem_root_certs = AppSettings::certificate_ca();
        std::shared_ptr<grpc::ChannelCredentials> channelCredentials = grpc::SslCredentials(sslOpts);

        std::shared_ptr<CallCredentials> callCredentials = MetadataCredentialsFromPlugin(std::unique_ptr<MetadataCredentialsPlugin>(
                new RcrMetadataCredentialsPlugin(config.username, config.password)));
        std::shared_ptr<grpc::ChannelCredentials> compositeChannelCredentials = grpc::CompositeChannelCredentials(channelCredentials, callCredentials);
        // channel = grpc::CreateChannel(target, compositeChannelCredentials);
        channel = grpc::CreateCustomChannel(target, compositeChannelCredentials, args);
    } else {
        channel = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    }

    RcrClient rpc(channel, config.username, config.password);

    rcr::User u;
    u.set_name(config.username);
    u.set_password(config.password);

    if (config.command.find("card") == 0) {
        // component symbol card-d -> D card-r -> R card-c -> C card-l -> L
        std::string cs;
        if (config.command.size() > 5)
            cs = config.command.substr(5);
        if (cs.empty())
            cs = config.componentSymbol;
        cs = toUpperCase(cs);
        int32_t r = rpc.cardQuery(std::cout, u, config.request, cs, config.offset, config.size, true);
        if (r) {
            exit(r);
        }
    }

    if (config.command.find("box") == 0) {
        // component symbol card-219 -> 219 card-219-1 -> 219-1 card-219-1-2 -> 219-1-2
        std::string cs;
        if (config.command.size() > 4)
            cs = config.command.substr(4);
        if (cs.empty())
            cs = config.componentSymbol;
        uint64_t minBox = 0;
        StockOperation::parseBoxes(minBox, cs, 0, cs.size());
        // std::cout << rpc.getBoxJson(minBox, config.offset, config.size) << std::endl;;
        rpc.printBox(std::cout, minBox, config.offset, config.size);
        std::cout << std::endl;;
        if (r) {
            exit(r);
        }
    }

    if (config.command == "login") {
        std::cout << (rpc.login(&u) ? _("success") : _("fail")) << std::endl;
    }
    if (config.command == "users") {
        rpc.printUser(std::cout, &u);
        std::cout << std::endl;
    }
    if (config.command == "boxes") {
        rpc.printBoxes(std::cout, config.offset, config.size, config.username, config.password);
        std::cout << std::endl;
    }
    if (config.command == "dictionaries") {
        std::cout << rpc.getDictionariesJson() << std::endl;
    }

    if (config.command.find("xlsx") == 0) {
        printSpreadSheets(config.request, config.box, config.command.find("xlsx-list") == 0 ? 2 : 1,
                          config.numberInFileName);
        if (config.command.find("xlsx-add") == 0) {
            std::string cs;
            if (config.command.size() > 9)
                cs = config.command.substr(9);
            if (cs.empty())
                cs = config.componentSymbol;
            cs = toUpperCase(cs);
            // component symbol xlsx-add-u -> U xlsx-add-r -> R xlsx-add-c -> C xlsx-add-l -> L
            importSpreadSheets(rpc, config.request, cs, config.box, config.numberInFileName);
        }
    }

    if (config.command.find("stream") == 0) {
        std::string cs;
        if (config.command.size() > 7)
            cs = config.command.substr(7);
        if (cs == "query") {
            std::string line;
            std::string symbol;
            std::cerr << _("Enter help to see command list") << std::endl;

            while (std::getline(std::cin, line)) {
                // nothing to do
                if (line.empty())
                    continue;
                size_t start = 0;
                std::string cliCmd = toUpperCase(nextWord(line, start));
                if (cliCmd == "HELP") { // "help"
                    std::cerr << HELP_STRING << std::endl;
                    continue;
                }
                if (cliCmd == "QUIT")  // "quit"
                    break;
                if (cliCmd == "SYMBOL") {
                    std::string symbolParam = toUpperCase(nextWord(line, start));
                    if (symbolParam.empty()) {
                        rpc.printSymbols(std::cout, config.locale);
                        std::cout << std::endl;
                        continue;
                    } else {
                        if (symbolParam == "*")
                            symbol = "";
                        else
                            symbol = trim(symbolParam);
                    }
                }
                if (cliCmd == "USER") {
                    std::string userParam = toUpperCase(nextWord(line, start));
                    if (userParam.empty()) {
                        rpc.printUser(std::cout, &u);
                        std::cout << std::endl;
                        continue;
                    }
                }
                if (cliCmd == "PROPERTY") {
                    std::string propertyManipStr = nextWord(line, start);
                    char propertyManip = 'l';
                    if (!propertyManipStr.empty())
                        propertyManip = propertyManipStr[0];
                    switch (propertyManip) {
                        case '+':
                        case '-':
                        case '=':
                            rpc.changeProperty(line.substr(8), config.username, config.password);
                            break;
                        default:
                            rpc.printProperty(std::cout);
                            std::cout << std::endl;
                            break;
                    }
                    continue;
                }

                if (cliCmd == "BOX") {
                    std::string boxCmdStr = nextWord(line, start);
                    char boxCmd = 'l';
                    if (!boxCmdStr.empty())
                        boxCmd = boxCmdStr[0];
                    uint64_t srcBox;
                    uint64_t destBox;
                    std::string p;
                    std::string name;
                    switch(boxCmd) {
                        case '+':
                            p = nextWord(line, start);
                            StockOperation::parseBoxes(srcBox, p, 0, p.size());
                            name = remainText(line, start);
                            break;
                        case '-':
                            p = nextWord(line, start);
                            StockOperation::parseBoxes(srcBox, p, 0, p.size());
                            break;
                        case '=':
                            p = nextWord(line, start);
                            StockOperation::parseBoxes(srcBox, p, 0, p.size());
                            name = remainText(line, start);
                            break;
                        case '/':
                            p = nextWord(line, start);
                            StockOperation::parseBoxes(srcBox, p, 0, p.size());
                            p = nextWord(line, start);
                            StockOperation::parseBoxes(destBox, p, 0, p.size());
                            name = remainText(line, start);
                            break;
                        default:    // box listing
                            rpc.printBoxes(std::cout, config.offset, config.size, config.username, config.password);
                            std::cout << std::endl;
                            continue;
                    }
                    rpc.chBox(boxCmd, srcBox, destBox, name, config.username, config.password);
                    continue;
                }
                if (cliCmd == "IMPORT") {
                    std::string path = nextWord(line, start);
                    std::string importSymbol = nextWord(line, start);
                    std::string boxName = nextWord(line, start);
                    std::string sNumberInFilename = nextWord(line, start);
                    bool numberInFilename;
                    if (!sNumberInFilename.empty()) {
                        switch(sNumberInFilename[0]) {
                            case 'F':
                            case 'f':
                            case 'N':
                            case 'n':
                                numberInFilename = false;
                                break;
                        }
                    }
                    if (importSymbol.empty() || boxName.empty())
                        printSpreadSheets(path, boxName, 1, numberInFilename);
                    else
                        importSpreadSheets(rpc, path, importSymbol, boxName, numberInFilename);
                    continue;
                }
                int32_t r = rpc.cardQuery(std::cout, u, line, symbol, config.offset, config.size, false);
                std::cout << std::endl;
                if (r)
                    exit(r);
            }
        }
    }
    return 0;
}
