/**
 * ./onewayyicketcli -k cert/client.key -c cert/client.crt -r cert/roots.crt  --adduser "Alice"
 */
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* for tm_gmtoff and tm_zone */
#endif
#include <time.h>

#include <openssl/crypto.h>

#include "argtable3/argtable3.h"
#include <grpc++/grpc++.h>

#include "gen/rcr.grpc.pb.h"

#include "grpcClient.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

const char* progname = "rcr-cli";

#define DEF_PORT		        50051
#define DEF_ADDRESS			    "127.0.0.1"

void argInt2Vector(std::vector<uint64_t> &retval, struct arg_int *args)
{
	int *p = args->ival;
	for (int i = 0; i < args->count; i++) {
		retval.push_back(*p);
		p++;
	}
}

class ClientConfig {
public:
    std::string intface;
    int port;
    bool sslon;
    int repeats;
    int verbose;
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
	struct arg_str *a_interface = arg_str0("i", "hostname", "<service host name>", "service host name. Default onewayticket.commandus.com");
	struct arg_int *a_port = arg_int0("l", "listen", "<port>", "service port. Default 50051");
	// SSL
	struct arg_lit *a_sslon = arg_lit0("s", "sslon", "SSL on");
	// commands
	// struct arg_file *a_niceclassfn = arg_file0(NULL, "class", "<file>", "add NICE classes from JSON file");
	struct arg_str *a_getuser = arg_str0(NULL, "getuser", "<phone>", "get user name.");
	struct arg_str *a_adduser = arg_str0(NULL, "adduser", "<common name>", "register a new user. Use --phone --gcmid");
	struct arg_lit *a_rmuser = arg_lit0(NULL, "rmuser", "remove user");
	struct arg_str *a_setuser = arg_str0(NULL, "setuser", "<common name>", "change user, see --phone, -gcmid.");

	struct arg_int *a_repeats = arg_int0("n", "repeat", "<number>", "Default 1");
	struct arg_lit *a_verbose = arg_litn("v", "verbose", 0, 5, "Verbose level");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = { a_interface, a_port,
		a_sslon,
		// other parameters
		a_getuser, a_adduser, a_rmuser, a_setuser,
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

	value->sslon = a_sslon->count > 0;


	if (a_repeats->count)
	{
		value->repeats = *a_repeats->ival;
	}
	else
		value->repeats = 1;

	value->verbose = a_verbose->count;

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

int main(int argc, char** argv)
{
	struct ClientConfig config;
	int r;
	if (r = parseCmd(argc, argv, &config))
		exit(r);

	return 0;
}
