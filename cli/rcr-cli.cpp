/**
 * rcr-cli
 *
 * Compile:
 * g++ -o rcr-cli -I.. -I../third-party ../cli/rcr-cli.cpp ../cli/grpcClient.cpp ../RcrCredentials.cpp ../AppSettings.cpp ../gen/rcr.grpc.pb.cc ../gen/rcr.pb.cc CMakeFiles/rcr-cli.dir/third-party/argtable3/argtable3.c.o -lgrpc++ -lprotobuf
 */
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "argtable3/argtable3.h"
#include <grpc++/grpc++.h>

#include "grpcClient.h"

#include "AppSettings.h"
#include "RcrCredentials.h"

const char* progname = "rcr-cli";
const char* DEF_COMMAND = "card";

#define DEF_PORT		        50051
#define DEF_ADDRESS			    "127.0.0.1"

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
};

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
	struct arg_str *a_command = arg_str0(nullptr, nullptr, "<command>", "card|version");
    struct arg_str *a_request = arg_str0(nullptr, nullptr, "<request>", "command request");
    struct arg_int *a_offset = arg_int0("o", "offset", "<number>", "List offset 0.. Default 0");
    struct arg_int *a_size = arg_int0("s", "size", "<number>", "List size. Default 100");

    struct arg_int *a_repeats = arg_int0("n", "repeat", "<number>", "Default 1");
	struct arg_lit *a_verbose = arg_litn("v", "verbose", 0, 5, "Verbose level");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = { a_interface, a_port, a_sslon,
        a_command, a_request,
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

    if (a_offset->count)
        value->offset = *a_offset->ival;
    else
        value->offset = 0;

    if (a_size->count)
        value->size = *a_offset->ival;
    else
        value->size = 100;

    size_t offset;
    size_t size;

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

    if (config.command == "card") {
        rpc.cardQuery(std::cout, config.request, config.offset, config.size);
        std::cout << std::endl;
    }

    if (config.command == "version")
        std::cout << "version " << std::hex << "0x" << rpc.version() << std::endl;
	return 0;
}
