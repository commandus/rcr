/*
 * svcconfig.cpp
 */

#include "svcconfig.h"

#ifdef ENABLE_SQLITE
#endif

#ifdef ENABLE_PG
#include <postgresql/libpq-fe.h>
#include <odb/database.hxx>
#include <odb/pgsql/database.hxx>

/**
 * Establish configured database connection
 */
static PGconn *dbconnect(
    struct ServiceConfig *config
)
{
	if ((config->dbconn) && (strlen(config->dbconn)))
		return PQconnectdb(config->dbconn);
	else
		return PQsetdbLogin(config->dbhost, config->dbport, config->dboptionsfile,
			NULL, config->dbname, config->dbuser, config->dbpassword);
}


static odb::database *odbconnect(
    struct ServiceConfig *config
)
{
	return new odb::pgsql::database(
        std::string(config->dbuser),
        std::string(config->dbpassword),
        std::string(config->dbname),
        std::string(config->dbhost),
        config->dbport);
}

#endif
