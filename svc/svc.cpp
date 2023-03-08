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
#include <argtable3.h>

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>

#include "platform.h"
#include "svcconfig.h"
#include "daemonize.h"
#include "utilstring.h"
#include "svcImpl.h"
#include "SSLValidator.h"

#include "passphrase.h"

const char* progname = "rcrsvc";

#define CODE_WRONG_OPTIONS              1
#define DEF_DATABASESOCKET				""
#define DEF_DATABASECHARSET				"utf8"
#define DEF_DATABASECLIENTFLAGS			0
#define DEF_ROUTE_TAG_ID				1
#define DEF_BOOKING_EXPIRATION_MINUTES	30
#define DEF_SALES_FINISH_MINUTES		180
#define DEF_SALES_START_MINUTES			10080

#define DEF_MQTT_ADDRESS				"tcp://127.0.0.1"
#define DEF_MQTT_PORT					1883
#define DEF_MQTT_PORT_S					"1883"
#define DEF_MQTT_QOS					1
#define DEF_MQTT_QOS_S					"1"
#define DEF_MQTT_CLIENT_ID				"mqtt-receiver"
#define DEF_MQTT_KEEP_ALIVE_INTERVAL	20
#define DEF_MQTT_KEEP_ALIVE_INTERVAL_S	"20"

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
	LOG(INFO) << "stopped";
	server->Shutdown();
}

void done()
{
	LOG(INFO) << "done";
	if (server)
	{
		// TODO wait smth
		// delete server.release();
		// server = NULL;
	}
	if (config.snmp_agent > 0)
		snmpDone(progname);
	if (config.booking_expiration_mins > 0)
		bookingExpireDone();
}

int reslt;

void runSSL()
{
	LOG(INFO) << "running";
	config.pub = new NanoPub("inproc://onewayticketsvc");
	LOG(INFO) << "pub started: " << config.pub->address();

	std::stringstream ss;
	ss << config.address << ":" << config.port;
	OneWayTicketImpl service(&config);

	ServerBuilder builder;
	builder.SetMaxMessageSize(2147483647);
	builder.SetMaxSendMessageSize(2147483647);
	std::string key = file2string(config.fnpemkey);
	std::string certificate = file2string(config.fnpemcertificate);
	std::string cakey = file2string(config.fnpemcakey);
	std::string cacertificate = file2string(config.fnpemcacertificate);

	// Create server metadata  processor
	SSLValidator *validator = new SSLValidator(&config);
	std::shared_ptr<TicketAuthMetadataProcessor> processor(new TicketAuthMetadataProcessor(true, true, validator));
	std::shared_ptr<ServerCredentials> serverCredentials;

	if (config.verbosity > 0)
	{
			std::cerr << "SSL on" << std::endl;
			if ((key.length() == 0) || (certificate.length() == 0))
					std::cerr << "Server private key file " << config.fnpemkey << " and/or certificate file " << config.fnpemcertificate << " does not exist or empty." << std::endl;
	}
	grpc::SslServerCredentialsOptions::PemKeyCertPair pkcp = {key, certificate};
	grpc::SslServerCredentialsOptions sslOpts;
	sslOpts.pem_key_cert_pairs.push_back(pkcp);
	sslOpts.force_client_auth = true;
	// sslOpts.pem_root_certs required to invalidate client certificates.
	sslOpts.pem_root_certs = cacertificate;
	serverCredentials = SslServerCredentials(sslOpts);
	serverCredentials->SetAuthMetadataProcessor(processor);
	// start
	builder.AddListeningPort(ss.str(), serverCredentials);
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

	delete config.pub;
	LOG(INFO) << "pub stopped";
	config.pub = NULL;
}

void run()
{
	LOG(INFO) << "running";

	if (config.snmp_agent > 0)
		snmpInit(progname, config.snmp_agent > 1, config.verbosity, &stopRequest, init_mibs);

	if (config.booking_expiration_mins > 0)
		bookingExpireInit(&config, &stopBookingRequest);

	config.pub = new NanoPub("inproc://onewayticketsvc");
	LOG(INFO) << "pub started: " << config.pub->address();

	std::stringstream ss;
	ss << config.address << ":" << config.port;
	OneWayTicketImpl service(&config);

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

	delete config.pub;
	LOG(INFO) << "pub stopped";
	config.pub = NULL;
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
	// default route_tag_id
	struct arg_int *a_route_tag_id = arg_int0(NULL, "route_tag_id", "<number>", "Default route tag id. Default 1");

	// GCM API key
	struct arg_str *a_apikey = arg_str0("G", "apikey", "<API key>", "GCM API key");
	// GLOG
	struct arg_lit *a_severity = arg_litn("v", "verbose", 0, 2, "-v- warning, -vv info, default error");

	// MQTT
	struct arg_str *a_broker_address = arg_str0("a", "broker", "<host name/address>", "MQTT broker address. Default " DEF_MQTT_ADDRESS);
	struct arg_int *a_broker_port = arg_int0("p", "port", "<number>", "MQTT broker TCP port. Default " DEF_MQTT_PORT_S);
	struct arg_int *a_qos = arg_int0("q", "qos", "<0..2>", "0- at most once, 1- at least once, 2- exactly once. Default " DEF_MQTT_QOS_S);
	struct arg_str *a_client_id = arg_str0("c", "client", "<id>", "MQTT client identifier string. Default " DEF_MQTT_CLIENT_ID);
	struct arg_int *a_keep_alive_interval = arg_int0("k", "keepalive", "<seconds>", "Keep alive interval. Default " DEF_MQTT_KEEP_ALIVE_INTERVAL_S);

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

	// SSL
	struct arg_file *a_fnpemkey = arg_file0("k", "key", "<file name>", "Server private key PEM file. Default cert/server.crt.");
	struct arg_file *a_fnpemcertificate = arg_file0("c", "certificate", "<file>", "Server certificate PEM file. Default cert/server.crt");
	struct arg_file *a_fnpemcakey = arg_file0("K", "cakey", "<file name>", "CA private key PEM file. Default cert/ca.key.");
	struct arg_file *a_fnpemcacertificate = arg_file0("C", "cacertificate", "<file>", "CA certificates PEM file. Default cert/ca.crt.");

	struct arg_lit *a_daemonize = arg_lit0("d", "daemonize", "start as daemon/service");
	// booking ticket
	struct arg_int *a_booking_expiration_mins = arg_int0("x", "expiration", "<mins>", "Booking expiration in minutes. Default 30. 0- booking never expire.");
	// trip sale expiration
	struct arg_int *a_sales_finish_mins = arg_int0(NULL, "sales_finish", "<mins>", "Sale close [180] minutes before start. 0- never stop sale automatically.");
	// trip sale start
	struct arg_int *a_sales_start_mins = arg_int0(NULL, "sales_start", "<mins>", "Sale start [10080] minutes before start. 0- never start sale automatically.");

	// SNMP
	struct arg_int *a_snmp_agent = arg_int0(NULL, "snmp", "0,1,2", "0- none(default); 1- udp/161 listener; 2- SNMP agent on (Net-SNMP daemon must running on).");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = { a_interface, a_port, a_route_tag_id, a_apikey, a_severity,
			a_broker_address, a_broker_port, a_qos, a_client_id, a_keep_alive_interval,
			a_conninfo, a_user, a_database, a_password, a_host, a_dbport, a_optionsfile, a_dbsocket, a_dbcharset, a_dbclientflags,
			a_fnpemkey, a_fnpemcertificate, a_fnpemcakey, a_fnpemcacertificate,
			a_booking_expiration_mins, a_sales_finish_mins, a_sales_start_mins, a_snmp_agent,
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

	if (a_apikey->count > 0)
		value->apikey = *a_apikey->sval;
	else
		value->apikey = "";

	value->verbosity = a_severity->count;

	// MQTT
	if (a_broker_address->count)
		value->broker_address = *a_broker_address->sval;
	else
		value->broker_address = DEF_MQTT_ADDRESS;

	if (a_broker_port->count)
		value->broker_port = *a_broker_port->ival;
	else
		value->broker_port = DEF_MQTT_PORT;

	if (a_qos->count)
		value->qos = *a_qos->ival;
	else
		value->qos = DEF_MQTT_QOS;

	if (a_client_id->count)
		value->client_id = *a_client_id->sval;
	else
		value->client_id = DEF_MQTT_CLIENT_ID;

	if (a_keep_alive_interval->count)
		value->keep_alive_interval = *a_keep_alive_interval->ival;
	else
		value->keep_alive_interval = DEF_MQTT_KEEP_ALIVE_INTERVAL;

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
	if (PQstatus(conn) != CONNECTION_OK)
	{
			printf("database connection error\nUsage: %s\n",  progname);
			arg_print_syntax(stdout, argtable, "\n");
			return 2;
	}
	PQfinish(conn);

	value->daemonize = a_daemonize->count > 0;

	if (a_route_tag_id->count)
			value->route_tag_id = *a_route_tag_id->ival;
	if (value->route_tag_id <= 0)
		value->route_tag_id = DEF_ROUTE_TAG_ID;
	
	value->topics.clear();
	std::stringstream prefix_ss;
	prefix_ss << "/onewayticket/" << value->route_tag_id << "/";
	std::string prefix = prefix_ss.str();
	value->topics.push_back(prefix + "trip/#");
	value->topics.push_back(prefix + "ticket/#");

	if (a_booking_expiration_mins->count)
		value->booking_expiration_mins = *a_booking_expiration_mins->ival;
	if (value->booking_expiration_mins <= 0)
		value->booking_expiration_mins = DEF_BOOKING_EXPIRATION_MINUTES;

	if (a_sales_finish_mins->count)
		value->sales_finish_mins = *a_sales_finish_mins->ival;
	if (value->sales_finish_mins <= 0)
		value->sales_finish_mins = DEF_SALES_FINISH_MINUTES;

	if (a_sales_start_mins->count)
		value->sales_start_mins = *a_sales_start_mins->ival;
	if (value->sales_start_mins <= 0)
		value->sales_start_mins = DEF_SALES_START_MINUTES;

	if (a_snmp_agent->count > 0)
	{
		value->snmp_agent = *a_snmp_agent->ival;
	}
	else
	{
		value->snmp_agent = 0;
	}

	if (a_fnpemkey->count)
	{
		value->sslon = true;
		value->fnpemkey = *a_fnpemkey->filename;
	}
	else
	{
		value->sslon = false;
		value->fnpemkey = DEF_SVCFNPEMKEY;
	}

	if (a_fnpemcertificate->count)
		value->fnpemcertificate = *a_fnpemcertificate->filename;
	else
		value->fnpemcertificate= DEF_SVCFNPEMCERTIFICATE;

	if (a_fnpemcakey->count)
		value->fnpemcakey = *a_fnpemcakey->filename;
	else
		value->fnpemcakey = DEF_SVCFNPEMCAKEY;

	if (a_fnpemcacertificate->count)
		value->fnpemcacertificate = *a_fnpemcacertificate->filename;
	else
		value->fnpemcacertificate= DEF_SVCFNPEMCACERTIFICATE;

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));

	char wd[PATH_MAX];
	value->path = getcwd(wd, PATH_MAX);	
	return 0;
}

void setSignalHandler(int signal)
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

	INIT_LOGGING("onewayticketsvc")

	LOG(INFO) << "Starting...";

	if (config.daemonize)
	{
		if (config.verbosity)
			std::cerr << "Start as daemon, use syslog" << std::endl;
		Daemonize daemonize(progname, config.path, config.sslon ? runSSL : run, stopNWait, done);
	}
	else
	{
		config.sslon ? runSSL() : run();
		done();
	}

	exit(reslt);
}
