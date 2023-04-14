/**
 * rcr-cli
 *
 * Compile:
 * g++ -o rcr-cli -I.. -I../third-party ../cli/rcr-cli.cpp ../cli/grpcClient.cpp ../RcrCredentials.cpp ../AppSettings.cpp ../gen/rcr.grpc.pb.cc ../gen/rcr.pb.cc CMakeFiles/rcr-cli.dir/third-party/argtable3/argtable3.c.o -lgrpc++ -lprotobuf
 */
#include <string>
#include <iostream>
#include <iomanip>

#include "argtable3/argtable3.h"
#include <grpc++/grpc++.h>

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

static const char *GREETING_STRING = "Enter help to see command list\n";

static const char *HELP_STRING =
    "help               Help screen\n"
    "symbol list        Component type and symbol list\n"
    "symbol             Set all component types\n"
    "symbol D           Set component symbol D (integrated circuits)\n"
    "property list      Properties list\n"
    "user list          Registered users\n"
    "import <path>      Preview spreadsheets in the path\n"
    "import <path> R 42 Import resistors (symbol R) from spreadsheets in the path to box 42\n"
    "D*                 Search by name starting with 'D'\n"
    "100 kOhm           Search resistors by nominal\n"
    "10 µF 119          Search capacitor by nominal in boxes 119-..\n"
    "* 119              Search all in boxes 119-..\n"
    "* 119-1 K:DIP      Search all in box 119-1 DIP\n"
    ;

class ClientConfig {
public:
    std::string intface;
    int port;
    bool sslOn;
    int repeats;
    int verbose;

    std::string command;
    std::string request;
    size_t offset;
    size_t size;
    std::string username;
    std::string password;
    std::string box;
    std::string componentSymbol;
};

void printSpreadSheets(
    const std::string &path,
    const std::string &boxName,
    int verbosity
) {
    std::vector<std::string> spreadSheets;
    util::filesInPath(path, ".xlsx", 0, &spreadSheets);
    std::cout << spreadSheets.size() << " *.xlsx files found in '" << path << "'" << std::endl;
    for (auto it = spreadSheets.begin(); it != spreadSheets.end(); it++) {
        uint64_t box = BoxName::extractFromFileName(boxName + " " + *it); //  <- add if filename contains boxes
        SpreadSheetHelper spreadSheet(*it);
        if (verbosity) {
            std::cout << *it << ": " << spreadSheet.items.size() << " names, " << spreadSheet.total << " items" << std::endl;
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
    const std::string &boxName
) {
    std::vector<std::string> spreadSheets;
    util::filesInPath(path, ".xlsx", 0, &spreadSheets);
    for (auto it = spreadSheets.begin(); it != spreadSheets.end(); it++) {
        uint64_t box = BoxName::extractFromFileName(boxName + " " + *it); //  <- add if filename contains boxes
        SpreadSheetHelper spreadSheet(*it);
        // component symbol xlsx-add-u -> U xlsx-add-r -> R xlsx-add-c -> C xlsx-add-l -> L
        int r = rpc.saveSpreadsheet(box, symbol, spreadSheet.items);
        if (r) {
            std::cerr << "Error: " << r << std::endl;
            break;
        }
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
	struct arg_str *a_interface = arg_str0("i", "hostname", "<service host name>", "service host name.");
	struct arg_int *a_port = arg_int0("l", "listen", "<port>", "service port. Default 50051");
	// SSL
	struct arg_lit *a_sslon = arg_lit0("s", "sslOn", "SSL on");
	// commands
	// struct arg_file *a_niceclassfn = arg_file0(nullptr, "class", "<file>", "add NICE classes from JSON file");
	struct arg_str *a_command = arg_str0(nullptr, nullptr, "<command>",
            "card|box|login|users|dictionaries|xlsx|xlsx-list|xlsx-add-u");
    struct arg_str *a_request = arg_str0(nullptr, nullptr, "<request>", "command parameter(request)");
    struct arg_str *a_user_name = arg_str0("u", "user", "<user-name>", "User login");
    struct arg_str *a_user_password = arg_str0("p", "password", "<password>", "User password");
    struct arg_str *a_box = arg_str0("b", "box", "<box>", "box prefix e.g. 221-2");
    struct arg_int *a_offset = arg_int0("o", "offset", "<number>", "List offset 0.. Default 0");
    struct arg_int *a_size = arg_int0("s", "size", "<number>", "List size. Default 100");

    struct arg_int *a_repeats = arg_int0("n", "repeat", "<number>", "Default 1");
	struct arg_lit *a_verbose = arg_litn("v", "verbose", 0, 5, "Verbose level");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = { a_interface, a_port, a_sslon,
        a_command, a_request, a_box, a_user_name, a_user_password,
        a_offset, a_size,
		a_repeats, a_verbose,
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
		printf("Usage: %s\n",  progname);
		arg_print_syntax(stdout, argtable, "\n");
		printf("rcr CLI client\n");
		arg_print_glossary(stdout, argtable, "  %-25s %s\n");
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
        value->size = 100;

    if (a_repeats->count)
		value->repeats = *a_repeats->ival;
	else
		value->repeats = 1;

	value->verbose = a_verbose->count;

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 0;
}

int main(int argc, char** argv)
{
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
    // rpc.addPropertyType("ptkey", "pt desc");

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
        std::cout << (rpc.login(&u) ? "success" : "fail") << std::endl;
    }
    if (config.command == "users") {
        rpc.printUser(std::cout, &u);
        std::cout << std::endl;
    }
    if (config.command == "dictionaries") {
        std::cout << rpc.getDictionariesJson() << std::endl;
    }
    if (config.command.find("stream") == 0) {
        std::string cs;
        if (config.command.size() > 7)
            cs = config.command.substr(7);
        if (cs == "query") {
            std::string line;
            std::string symbol = "";
            std::cerr << GREETING_STRING << std::endl;
            while (std::getline(std::cin, line)) {
                if (line.find("help") == 0) {
                    std::cerr << HELP_STRING << std::endl;
                    continue;
                }
                if (line.find("symbol") == 0) {
                    auto verb = line.find("list");
                    if (verb != std::string::npos && verb >= 6) {   // symbollist, symbol-list, symbol list
                        rpc.printSymbols(std::cout);
                        std::cout << std::endl;
                        continue;
                    }
                    symbol = toUpperCase(trim(line.substr(6)));
                }
                if (line.find("user") == 0) {
                    auto verb = line.find("list");
                    if (verb != std::string::npos && verb >= 4) {   // userlist, user-list, user list
                        rpc.printUser(std::cout, &u);
                        std::cout << std::endl;
                        continue;
                    }
                }
                if (line.find("property") == 0) {
                    auto verb = line.find("list");
                    if (verb != std::string::npos && verb >= 8) {   // propertylist, property-list, property list
                        rpc.printProperty(std::cout);
                        std::cout << std::endl;
                        continue;
                    }
                }

                if (line.find("import") == 0) {
                    std::string symbol;
                    std::string path;
                    size_t start = 6;
                    size_t eolp = line.size();
                    size_t finish = eolp;
                    // skip spaces
                    for (auto p = start; p < eolp; p++) {
                        if (!std::isspace(line[p])) {
                            start = p;
                            break;
                        }
                    }
                    // try read path
                    for (auto p = start; p < eolp; p++) {
                        if (std::isspace(line[p])) {
                            finish = p;
                            break;
                        }
                    }
                    path = line.substr(start, finish - start);
                    start = finish;
                    // skip spaces
                    for (auto p = start; p < eolp; p++) {
                        if (!std::isspace(line[p])) {
                            start = p;
                            break;
                        }
                    }
                    // try read symbol
                    for (auto p = start; p < eolp; p++) {
                        if (std::isspace(line[p])) {
                            finish = p;
                            break;
                        }
                    }
                    symbol = line.substr(start, finish - start);
                    start = finish;
                    // skip spaces
                    for (auto p = start; p < eolp; p++) {
                        if (!std::isspace(line[p])) {
                            start = p;
                            break;
                        }
                    }
                    // try read box
                    for (auto p = start; p < eolp; p++) {
                        if (std::isspace(line[p])) {
                            finish = p;
                            break;
                        }
                    }
                    std::string boxName = line.substr(start, finish - start);
                    if (symbol.empty() || boxName.empty())
                        printSpreadSheets(path, "", 1);
                    else
                        importSpreadSheets(rpc, path, symbol, boxName);
                    continue;
                }

                int32_t r = rpc.cardQuery(std::cout, u, line, symbol, config.offset, config.size, false);
                std::cout << std::endl;
                if (r) {
                    exit(r);
                }
            }
        }
    }

    if (config.command.find("xlsx") == 0) {
        printSpreadSheets(config.request, config.box, config.command.find("xlsx-list") == 0 ? 2 : 1);
        if (config.command.find("xlsx-add") == 0) {
            std::string cs;
            if (config.command.size() > 9)
                cs = config.command.substr(9);
            if (cs.empty())
                cs = config.componentSymbol;
            cs = toUpperCase(cs);
            // component symbol xlsx-add-u -> U xlsx-add-r -> R xlsx-add-c -> C xlsx-add-l -> L
            importSpreadSheets(rpc, config.request, config.box, cs);
        }
    }
	return 0;
}

