/**
 * service options
 **/
#ifndef SVCCONFIG_H
#define SVCCONFIG_H

#include <string>

#define DEF_PORT        50051
#define DEF_ADDRESS     "0.0.0.0"
#define DEF_DB_HOST		"localhost"
#define DEF_DB_PORT		"5432"

class ServiceConfig
{
public:
	// start up options
	bool daemonize;				    ///< start as daemon
	int verbosity;	                ///< verbose level: 0- error only, 1- warning, 2- info, 3- debug
	const char *address;			///< HTTP/2 service interface address
	int port;						///< HTTP/2 service interface port
#ifdef ENABLE_SQLITE
    std::string sqliteDbName;
#endif
#ifdef ENABLE_PG
    // PostgreSQL connection
    const char *dbconn;				///< PostgreSQL connection string
    const char *dbhost;				///< PostgreSQL host
    const char *dbport;				///< PostgreSQL port
    const char *dboptionsfile;		///< reserved for MySQL
    const char *dbname;				///< database/schema name
    const char *dbuser;				///< PostgreSQL user name
    const char *dbpassword;			///< PostgreSQL user password

    const char *dbsocket;			///< reserved
    const char *dbcharset;			///< reserved
    long dbclientflags;				///< reserved
#endif
    /// SSL
    bool sslOn;
    /// HTTP JSON embedded service
    bool httpJsonOn;
    uint16_t httpJsonPort;
	std::string path;
    std::string pluginDirPath;
    std::string pluginOptions;
};

#ifdef ENABLE_PG

#include <postgresql/libpq-fe.h>
#include <odb/database.hxx>

/**
 * Establish configured database connection
 */
PGconn *dbconnect(
	struct ServiceConfig *config		///< database configuration
);
#endif

#endif
