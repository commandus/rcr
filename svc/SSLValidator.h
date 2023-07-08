/*
 * SSLValidator.h
 */

#ifndef SSLVALIDATOR_H_
#define SSLVALIDATOR_H_

#include <string>
#include <memory>

#include <grpc++/impl/codegen/string_ref.h>

#include "svcconfig.h"
#include "RcrCredentials.h"

/**
 * Rcr validator on the SSL transport
 */
class SSLValidator : public RcrValidator
{
private:
	ServiceConfig *mConfig;			///< service configuration
protected:
	/// check user credentials
	bool checkCredentials(
		const grpc::string_ref &value,
		int *retflags, 						///< flags. 1- validated user, 0- invalid user
		int64_t *retorgid, 					///< group id. Reserved. Must be 0.
		int64_t *retuserid					///< User id. 0if not validated
	);
public:
	SSLValidator(
		ServiceConfig *config
	);
	virtual ~SSLValidator();
	virtual bool onCheckPassword(
		const grpc::string_ref &user,
		const grpc::string_ref &password,
		int *retflags,
		int64_t *retorgid,
		int64_t *retuserid
	) override;
	virtual bool onCheckTicket(
		const grpc::string_ref &ticket,
		int *retflags,
		int64_t *retorgid,
		int64_t *retuserid
	) override;
	virtual bool onCheckCertificateCN(
		const grpc::string_ref &sslcn,
		int *retflags,
		int64_t *retorgid,
		int64_t *retuserid
	) override;
};

#endif /* SSLVALIDATOR_H_ */
