/**
 * service options
 *
 **/
#ifndef SVCCONFIG_H
#define SVCCONFIG_H

#include <stdio.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif
#include <sys/types.h>

#ifdef OS_UBUNTU
#include <postgresql/libpq-fe.h>
#else
#include <libpq-fe.h>
#endif

#include <odb/database.hxx>
#include <odb/pgsql/database.hxx>

#define DEF_PORT        50051
#define DEF_ADDRESS     "0.0.0.0"
#define DEF_DB_HOST		"localhost"
#define DEF_DB_PORT		"5432"

struct ServiceConfig
{
	// start up options
	bool	daemonize;				///< start as daemon
	int		verbosity;				///< verbose level: 0- error only, 1- warning, 2- info, 3- debug
	const char *address;			///< HTTP/2 service interface address
	int port;						///< HTTP/2 service interface port

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

    /// SSL
    bool sslon;

	std::string path;
};

/**
 * Establish configured database connection
 */
PGconn *dbconnect(
	struct ServiceConfig *config		///< database configuration
);

/**
 * Establish configured ODB ORM database connection
 */
odb::database *odbconnect(
	struct ServiceConfig *config		///< database configuration
);

#endif
