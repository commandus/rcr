/*
 * svcconfig.cpp
 */

#include "svcconfig.h"

#ifdef ENABLE_SQLITE
#endif

#ifdef ENABLE_PG
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

#endif
