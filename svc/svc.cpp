/* 
 * gRPC service
 * Usage:
 * ./rcrsvc --user rcr --database rcr --password 123456
 */

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <thread>

#include <ctime>
#include <stdio.h>
#include <cerrno>
#include <csignal>
#include <memory.h>
#include <fcntl.h>
#include <stdlib.h>
#include <argtable3/argtable3.h>

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>

#include "platform.h"
#include "svcconfig.h"
#include "daemonize.h"
#include "svcImpl.h"
#include "SSLValidator.h"

#include "passphrase.h"

const char* progname = "rcr-svc";
const char* DEF_DB_SQLITE = "rcr.db";

#define CODE_WRONG_OPTIONS              1
#define DEF_DATABASESOCKET				""
#define DEF_DATABASECHARSET				"utf8"
#define DEF_DATABASECLIENTFLAGS			0

typedef void (*TDaemonRunner)();

static struct ServiceConfig config;		//<	program configuration read from command line
static std::unique_ptr<Server> server;

using grpc::ServerBuilder;

bool stopRequest = false;
bool stopBookingRequest = false;

void stopNWait()
{
	stopRequest = true;
	stopBookingRequest = true;
	server->Shutdown();
}

void done()
{
	if (server)	{
		// TODO wait smth
		// delete server.release();
		// server = NULL;
	}
}

int reslt;

void runSSL()
{
	std::stringstream ss;
	ss << config.address << ":" << config.port;
	RcrImpl service(&config);

	ServerBuilder builder;
	builder.SetMaxMessageSize(2147483647);
	builder.SetMaxSendMessageSize(2147483647);

	// Create server metadata  processor
	SSLValidator *validator = new SSLValidator(&config);
	std::shared_ptr<RcrAuthMetadataProcessor> processor(new RcrAuthMetadataProcessor(true, true, validator));
	std::shared_ptr<ServerCredentials> serverCredentials;

	if (config.verbosity > 0) {
			std::cerr << "SSL on" << std::endl;
	}
	grpc::SslServerCredentialsOptions sslOpts;
	sslOpts.force_client_auth = false;
	// sslOpts.pem_root_certs required to invalidate client certificates.
	serverCredentials = SslServerCredentials(sslOpts);
	serverCredentials->SetAuthMetadataProcessor(processor);
	// start
	builder.AddListeningPort(ss.str(), serverCredentials);
	builder.RegisterService(&service);
	server = builder.BuildAndStart();
	if (server)	{
		if (config.verbosity > 0)
			std::cout << "Server listening on " << ss.str() << "." << std::endl;
		server->Wait();
	}
	else
		std::cerr << "Can not start server." << std::endl;
}

void run()
{
	std::stringstream ss;
	ss << config.address << ":" << config.port;
	RcrImpl service(&config);

	ServerBuilder builder;
	builder.SetMaxMessageSize(2147483647);
	builder.SetMaxSendMessageSize(2147483647);

	if (config.verbosity > 0)
	{
			std::cerr << "SSL off" << std::endl;
	}
	// start
	builder.AddListeningPort(ss.str(), grpc::InsecureServerCredentials());
	builder.RegisterService(&service);
	server = builder.BuildAndStart();
	if (server)
	{
		if (config.verbosity > 0)
			std::cout << "Server listening on " << ss.str() << "." << std::endl;
		server->Wait();
	}
	else
		std::cerr << "Can not start server." << std::endl;
}


void signalHandler(int signal)
{
	switch(signal)
	{
	case SIGINT:
		std::cerr << "Interrupted..";
		stopNWait();
		done();
		std::cerr << "exit." << std::endl;
		break;
	default:
		std::cerr << "Signal " << signal << std::endl;
	}
}

/**
 * Parse command line into ServiceConfig
 * Return 0- success
 *        1- show help and exit, or command syntax error
 *        2- output file does not exists or can not open to write
 **/
int parseCmd
(
	int argc,
	char* argv[],
	struct ServiceConfig *value
)
{
	struct arg_str *a_interface = arg_str0("i", "ip4", "<address>", "service IPv4 network interface address. Default 0.0.0.0 (all)");
	struct arg_int *a_port = arg_int0("l", "listen", "<port>", "service port. Default 50051");
#ifdef ENABLE_SQLITE
    struct arg_str *a_sqliteDbName = arg_str0(nullptr, "db", "<SQLite# database>", "database, default \"rcr.db\"");
#endif
#ifdef ENABLE_PG
	// database connection
	struct arg_str *a_conninfo = arg_str0(NULL, "conninfo", "<string>", "database connection");
	struct arg_str *a_user = arg_str0(NULL, "user", "<login>", "database login");
	struct arg_str *a_database = arg_str0(NULL, "database", "<scheme>", "database scheme");
	struct arg_str *a_password = arg_str0(NULL, "password", "<password>", "database user password");
	struct arg_str *a_host = arg_str0(NULL, "host", "<host>", "database host. Default localhost");
	struct arg_str *a_dbport = arg_str0(NULL, "port", "<integer>", "database port. Default 5432");
	struct arg_file *a_optionsfile = arg_file0(NULL, "options-file", "<file>", "database options file");
	struct arg_str *a_dbsocket = arg_str0(NULL, "dbsocket", "<socket>", "database socket. Default none.");
	struct arg_str *a_dbcharset = arg_str0(NULL, "dbcharset", "<charset>", "database client charset. Default utf8.");
	struct arg_int *a_dbclientflags = arg_int0(NULL, "dbclientflags", "<number>", "database client flags. Default 0.");
#endif

	struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", "start as daemon/service");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = { a_interface, a_port,
#ifdef ENABLE_PG
			a_conninfo, a_user, a_database, a_password, a_host, a_dbport, a_optionsfile, a_dbsocket, a_dbcharset, a_dbclientflags,
#endif
#ifdef ENABLE_SQLITE
            a_sqliteDbName,
#endif
        a_help, a_end };

	int nerrors;

	// verify the argtable[] entries were allocated successfully
	if (arg_nullcheck(argtable) != 0)
	{
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
		printf("One way ticket GRPC service\n");
		arg_print_glossary(stdout, argtable, "  %-25s %s\n");
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return 1;
	}

	if (a_interface->count)
		value->address = *a_interface->sval;
	else
		value->address = DEF_ADDRESS;
	if (a_port->count)
		value->port = *a_port->ival;
	else
		value->port = DEF_PORT;


	value->verbosity = 0;

#ifdef ENABLE_SQLITE
    // database
	value->sqliteDbName = *a_sqliteDbName->sval;
	if (a_sqliteDbName->count)
		value->sqliteDbName = *a_sqliteDbName->sval;
	else
		value->sqliteDbName = DEF_DB_SQLITE;
#endif
#ifdef ENABLE_PG
	// database
	value->dbconn = *a_conninfo->sval;
	if (a_host->count)
		value->dbhost = *a_host->sval;
	else
		value->dbhost = DEF_DB_HOST;

	if (a_dbport->count)
		value->dbport = *a_dbport->sval;
	else
		value->dbport = DEF_DB_PORT;

	value->dboptionsfile = *a_optionsfile->filename;
	value->dbname = *a_database->sval;
	value->dbuser = *a_user->sval;
	value->dbpassword = *a_password->sval;
	if (a_dbsocket->count)
		value->dbsocket = *a_dbsocket->sval;
	else
		value->dbsocket = DEF_DATABASESOCKET;

	if (a_dbcharset->count)
		value->dbcharset = *a_dbcharset->sval;
	else
		value->dbcharset = DEF_DATABASECHARSET;

	if (a_dbclientflags->count)
		value->dbclientflags = *a_dbclientflags->ival;
	else
		value->dbclientflags = DEF_DATABASECLIENTFLAGS;

	// check database connection
	PGconn *conn = dbconnect(value);
	if (PQstatus(conn) != CONNECTION_OK) {
        std::cerr << "database connection error.\nUsage: " << progname << std::endl);
        arg_print_syntax(stdout, argtable, "\n");
        return 2;
    }
	PQfinish(conn);
#endif

	value->daemonize = a_daemonize->count > 0;

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));

	char wd[PATH_MAX];
	value->path = getcwd(wd, PATH_MAX);	
	return 0;
}

void setSignalHandler(
    int signal
)
{
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = &signalHandler;
	sigaction(signal, &action, NULL);
}

int main(int argc, char* argv[])
{
	initRandomName();
	// Signal handler
	setSignalHandler(SIGINT);
	reslt = 0;
	if (parseCmd(argc, argv, &config))
		exit(CODE_WRONG_OPTIONS);
	if (config.daemonize) {
		if (config.verbosity)
			std::cerr << "Start as daemon, use syslog" << std::endl;
		Daemonize daemonize(progname, config.path, config.sslon ? runSSL : run, stopNWait, done);
	}
	else {
		config.sslon ? runSSL() : run();
		done();
	}
	exit(reslt);
}
