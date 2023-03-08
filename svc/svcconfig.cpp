/*
 * svcconfig.cpp
 */

#include <string.h>
#include "svcconfig.h"

/**
 * Establish configured database connection
 */
PGconn *dbconnect(struct ServiceConfig *config)
{
	if ((config->dbconn) && (strlen(config->dbconn)))
		return PQconnectdb(config->dbconn);
	else
		return PQsetdbLogin(config->dbhost, config->dbport, config->dboptionsfile,
			NULL, config->dbname, config->dbuser, config->dbpassword);
}


odb::database *odbconnect(struct ServiceConfig *config)
{
	return new odb::pgsql::database(
			std::string(config->dbuser),
			std::string(config->dbpassword),
			std::string(config->dbname),
			std::string(config->dbhost),
			config->dbport);
}
