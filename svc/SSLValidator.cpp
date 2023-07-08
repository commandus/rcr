/*
 * SSLValidator.cpp
 */

#include <iostream>

#include "SSLValidator.h"

SSLValidator::SSLValidator(ServiceConfig *config)
{
	mConfig = config;
}

SSLValidator::~SSLValidator()
{
}

/**
  * credtype	0- token
  * 		1- SSL certificate CN
  */
bool SSLValidator::checkCredentials(const grpc::string_ref &value, int *retflags, int64_t *retorgid, int64_t *retuserid)
{
	std::string v(value.data());
	int64_t certid = strtoll(v.c_str(), nullptr, 0);
	// load from database
	bool ok = true;
	if (ok) {
		*retflags = 1;
		*retorgid = 0;
		*retuserid = 42;
	} else {
		*retflags = 0;
		*retorgid = 0;
		*retuserid = 0;
	}
	return ok;
}

/**
  * not implemented
  */
bool SSLValidator::onCheckPassword(const grpc::string_ref &user, const grpc::string_ref &password, int *retflags, int64_t *retorgid, int64_t *retuserid)
{
	return false;
}

bool SSLValidator::onCheckTicket(const grpc::string_ref &ticket, int *retflags, int64_t *retorgid, int64_t *retuserid)
{
	return checkCredentials(ticket, retflags, retorgid, retuserid);
}

bool SSLValidator::onCheckCertificateCN(const grpc::string_ref &sslcn, int *retflags, int64_t *retorgid, int64_t *retuserid)
{
	return checkCredentials(sslcn, retflags, retorgid, retuserid);
}
