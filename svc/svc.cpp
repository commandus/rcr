/* 
 * gRPC service
 * Usage:
 * ./rcrsvc --user rcr --database rcr --password 123456
 */

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>

#include <csignal>
#include <memory.h>
#include <cstdlib>
#include <argtable3/argtable3.h>

#include <grpc++/server.h>
#include <grpcpp/create_channel.h>

#include "svcconfig.h"
#include "daemonize.h"
#include "log.h"
#include "svcImpl.h"
#include "SSLValidator.h"

#include "passphrase.h"

#ifdef ENABLE_HTTP
// embedded HTTP service
#include "rcr-ws.h"
// print version
#include <microhttpd.h>
#endif

// i18n
#include <libintl.h>
#define _(String) gettext (String)

#include "log.h"

#ifdef _MSC_VER
#include <direct.h>
#define PATH_MAX MAX_PATH
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

const char* progname = "rcr-svc";
const char* DEF_DB_SQLITE = "rcr.db";

/**
 * Number of threads to run in the thread pool.  Should (roughly) match
 * the number of cores on your system.
 */

#if defined(CPU_COUNT) && (CPU_COUNT+0) < 2
#undef CPU_COUNT
#endif
#if !defined(CPU_COUNT)
#define CPU_COUNT 2
#endif

#define NUMBER_OF_THREADS CPU_COUNT

#define CODE_WRONG_OPTIONS              1
#define DEF_DATABASESOCKET				""
#define DEF_DATABASECHARSET				"utf8"
#define DEF_DATABASECLIENTFLAGS			0

typedef void (*TDaemonRunner)();

static ServiceConfig config;		//<	program configuration read from command line
static std::unique_ptr<Server> server;
static WSConfig wsConfig;

using grpc::ServerBuilder;

bool stopRequest = false;
bool stopBookingRequest = false;

RcrImpl *service = nullptr;

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
		// server = nullptr;
#ifdef ENABLE_HTTP
        doneWS(wsConfig);
#endif
        if (service) {
            delete service;
            service = nullptr;
        }
    }
}

int reslt;

static void runGrpcSSL()
{
	std::stringstream ss;
	ss << config.address << ":" << config.port;
    if (config.verbosity > 0)
        std::cerr << _("SSL on") << std::endl;

	// Create server metadata  processor
    std::shared_ptr<RcrAuthMetadataProcessor> processor(new RcrAuthMetadataProcessor(true, true,
        new SSLValidator(&config)));

    std::shared_ptr<grpc::ServerCredentials> serverCredentials = grpc::SslServerCredentials(grpc::SslServerCredentialsOptions());

    serverCredentials->SetAuthMetadataProcessor(processor);
	// start
    ServerBuilder builder;
    builder.SetMaxMessageSize(2147483647);
    builder.SetMaxSendMessageSize(2147483647);
    builder.AddListeningPort(ss.str(), serverCredentials);
	builder.RegisterService(service);
	server = builder.BuildAndStart();
	if (server)	{
		if (config.verbosity > 0)
			std::cout << _("Server listening on ") << ss.str() << std::endl;
		server->Wait();
	}
	else
		std::cerr << _("Can not start server") << std::endl;
}

static void runGrpc()
{
    std::stringstream ss;
    ss << config.address << ":" << config.port;
    ServerBuilder builder;
    builder.SetMaxMessageSize(2147483647);
    builder.SetMaxSendMessageSize(2147483647);
    builder.AddListeningPort(ss.str(), grpc::InsecureServerCredentials());
    builder.RegisterService(service);
    server = builder.BuildAndStart();
    if (server)	{
        if (config.verbosity > 0) {
            std::cout << _("Server listening on ") << ss.str() << std::endl;
#ifdef ENABLE_HTTP
            if (config.httpJsonOn)
                std::cout << _("HTTP JSON port ") << config.httpJsonPort << std::endl;
#endif
        }
        server->Wait();
    } else
        std::cerr << _("Can not start server") << std::endl;
}

#ifdef ENABLE_HTTP
static void runHttpJson(
    uint16_t port,
    bool asDaemon
)
{
    if (asDaemon)
        SYSLOG(LOG_ALERT, "Start HTTP service")

    wsConfig.descriptor = nullptr;
    wsConfig.flags = 0;
    wsConfig.lasterr = 0;
    wsConfig.onLog = nullptr;
    wsConfig.port = port;
    wsConfig.svc = service;
    if (!startWS(wsConfig)) {
        std::stringstream ss;
        ss << "Can not start web service errno "
            << errno << ": " << strerror(errno)
            << ". libmicrohttpd version " << std::hex << MHD_VERSION;
		if (asDaemon) {
			SYSLOG(LOG_ALERT, ss.str().c_str());
        } else {
            std::cerr << ss.str() << std::endl;
        }
    } else {
		if (asDaemon) {
			SYSLOG(LOG_ALERT, "HTTP service successfully started");
		}
    }
}
#endif

static void run() {
    service = new RcrImpl(&config);
#ifdef ENABLE_HTTP
    if (config.httpJsonOn)
        runHttpJson(config.httpJsonPort, config.daemonize);
#endif
    if (config.sslOn)
        runGrpcSSL();
    else
        runGrpc();
}

void signalHandler(int signal)
{
	switch(signal)
	{
	case SIGINT:
		std::cerr << _("Interrupted..");
		stopNWait();
		done();
		std::cerr << _("exit") << std::endl;
		break;
	default:
		std::cerr << _("Signal ") << signal << std::endl;
	}
}

/**
 * Parse command line into ServiceConfig
 * Return 0- success
 *        1- show help and exit, or command syntax error
 *        2- output file does not exists or can not open to write
 **/
int parseCmd(
	int argc,
	char* argv[],
	ServiceConfig *value
)
{
	struct arg_str *a_interface = arg_str0("i", "ip4", _("<address>"), _("service IPv4 network interface address. Default 0.0.0.0 (all)"));
	struct arg_int *a_port = arg_int0("l", "listen", _("<port>"), _("service port. Default 50051"));
#ifdef ENABLE_SQLITE
    struct arg_str *a_sqliteDbName = arg_str0(nullptr, "db", _("<SQLite# database>"), _("database, default \"rcr.db\""));
    struct arg_str *a_http_json_dirroot = arg_str0("r", "root", _("<path>"), _("web root path. Default './html'"));
#endif
#ifdef ENABLE_PG
	// database connection
	struct arg_str *a_conninfo = arg_str0(NULL, "conninfo", "<string>", _("database connection"));
	struct arg_str *a_user = arg_str0(NULL, "user", "<login>", _("database login"));
	struct arg_str *a_database = arg_str0(NULL, "database", "<scheme>", _("database scheme"));
	struct arg_str *a_password = arg_str0(NULL, "password", "<password>", _("database user password"));
	struct arg_str *a_host = arg_str0(NULL, "host", "<host>", _("database host. Default localhost"));
	struct arg_str *a_dbport = arg_str0(NULL, "port", "<integer>", _("database port. Default 5432"));
	struct arg_file *a_optionsfile = arg_file0(NULL, "options-file", "<file>", _("database options file"));
	struct arg_str *a_dbsocket = arg_str0(NULL, "dbsocket", "<socket>", _("database socket. Default none."));
	struct arg_str *a_dbcharset = arg_str0(NULL, "dbcharset", "<charset>", _("database client charset. Default utf8."));
	struct arg_int *a_dbclientflags = arg_int0(NULL, "dbclientflags", "<number>", _("database client flags. Default 0."));
#endif
    struct arg_lit *a_ssl = arg_lit0("s", "ssl", _("enable SSL"));
	struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", _("start as daemon/service"));
#ifdef ENABLE_HTTP
    struct arg_lit *a_http_json_on = arg_lit0("j", "json", _("run JSON HTTP service"));
    struct arg_int *a_http_json_port = arg_int0("p", "port", "_(<number>)", _("HTTP service port number. Default 8050"));

    if (a_http_json_dirroot->count)
        wsConfig.dirRoot = *a_http_json_dirroot->sval;
    else
        wsConfig.dirRoot = "html";
    wsConfig.threadCount = NUMBER_OF_THREADS;
    wsConfig.connectionLimit = 1024;
#endif
    struct arg_str *a_pluginDirPath = arg_str0("u", "plugin", _("<directory>"), _("plugins directory, default ./plugins"));
    struct arg_str *a_pluginOptions = arg_str0("U", "plugin-options", _("<string>"), _("plugins options, default none"));
    struct arg_str *a_pidfile = arg_str0("p", "pidfile", _("<file>"), _("Check whether a process has created the file pidfile"));
    struct arg_lit *a_verbosity = arg_litn("v", "verbosity", 0, 1, _("-v- verbose"));
	struct arg_lit *a_help = arg_lit0("h", "help", _("Show this help"));
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = { a_interface, a_port,
#ifdef ENABLE_PG
        a_conninfo, a_user, a_database, a_password, a_host, a_dbport, a_optionsfile, a_dbsocket, a_dbcharset, a_dbclientflags,
#endif
#ifdef ENABLE_SQLITE
        a_sqliteDbName,
#endif
#ifdef ENABLE_HTTP
        a_http_json_on,
        a_http_json_port,
        a_http_json_dirroot,
#endif
        a_ssl,
        a_pluginDirPath, a_pluginOptions,
        a_daemonize,
        a_pidfile,
        a_verbosity,
        a_help, a_end
    };

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
		std::cout << _("Usage: ") << progname << std::endl;
		arg_print_syntax(stdout, argtable, "\n");
		std::cout << _("rcr GRPC service") << std::endl;
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
    if (a_pidfile->count)
        value->pidfile = *a_pidfile->sval;
    else
        value->pidfile = "";

    value->sslOn = a_ssl->count > 0;
    value->verbosity = a_verbosity->count;

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
        std::cerr << _("database connection error.\nUsage: ") << progname << std::endl);
        arg_print_syntax(stdout, argtable, "\n");
        return 2;
    }
	PQfinish(conn);
#endif
#ifdef ENABLE_HTTP
    value->httpJsonOn = a_http_json_on->count > 0;
    value->httpJsonPort = 8050;
    if (a_http_json_port->count)
        value->httpJsonPort = *a_http_json_port->ival;
#endif
	value->daemonize = a_daemonize->count > 0;

#ifdef _MSC_VER
	char wd[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH - 1, wd);
#else
	char wd[PATH_MAX];
	value->path = getcwd(wd, PATH_MAX);
#endif
    if (a_pluginDirPath->count)
        value->pluginDirPath = *a_pluginDirPath->sval;
    if (a_pluginOptions->count)
        value->pluginOptions = *a_pluginOptions->sval;
    if (value->pluginDirPath.empty()) {
        value->pluginDirPath = value->path + "/plugins";
    }
	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 0;
}

void setSignalHandler(
    int signal
)
{
#ifndef _MSC_VER
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = &signalHandler;
	sigaction(signal, &action, nullptr);
#endif
}

int main(int argc, char* argv[])
{
    // I18N
    setlocale(LC_ALL, "");
    // bindtextdomain(progname, "/usr/share/locale");
    // bind_textdomain_codeset(progname, "UTF-8");
    textdomain(progname);

    initRandomName();
	// Signal handler
	setSignalHandler(SIGINT);
	reslt = 0;
	if (parseCmd(argc, argv, &config))
		exit(CODE_WRONG_OPTIONS);
	if (config.daemonize) {
        char wd[PATH_MAX];
        std::string currentPath = getcwd(wd, PATH_MAX);
		if (config.verbosity)
			std::cerr << _("Start as daemon, use syslog") << std::endl;
        OPEN_SYSLOG(progname)
        SYSLOG(LOG_ALERT, _("Start as daemon"))
        Daemonize daemonize(progname, currentPath, run, stopNWait, done, 0, config.pidfile);
	}
	else {
        run();
		done();
	}
	return reslt;
}
