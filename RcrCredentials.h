/*
 * RcrCredentials.h
 *
 *  Created on: 18.02.2016
 *      Author: andrei
 */

#ifndef OTPCREDENTIALSAUTHENTIFICATOR_H_
#define OTPCREDENTIALSAUTHENTIFICATOR_H_

// #define GRPC_OVERRIDE

#include <string>
#include <memory>

#include <grpc++/security/auth_metadata_processor.h>
#include <grpc++/security/credentials.h>
#include <grpc++/security/server_credentials.h>

using grpc::ServerCredentials;
using grpc::CallCredentials;
using grpc::Status;
using grpc::AuthContext;
using grpc::AuthMetadataProcessor;
using grpc::MetadataCredentialsPlugin;

#define ORG_ROLE_MASK_SVC_ADMIN		1
#define ORG_ROLE_MASK_ORG_ADMIN		2
#define ORG_ROLE_MASK_ORG_USER		4
#define ORG_ROLE_MASK_USER		8

/**
 * Interface
 */
class RcrValidator
{
public:
	// typedef bool (RcrValidator:: *CheckCertificateCNCallback)(const grpc::string_ref &value);
	// typedef bool (RcrValidator:: *CheckTicketCallback)(const grpc::string_ref &ticket);

	/// abstract method checks certificate
	virtual bool onCheckCertificateCN(
		const grpc::string_ref &value,
		int *retflags,					///< 1- identified user, 0- unidentified.
		int64_t *retorgid, 				///< return group id. Reserved. Must be 0.
		int64_t *retuserid				///< return user id. 0- User is not valid.
	) = 0;
	/// abstract method checks security ticket set by client (not used now)
	virtual bool onCheckTicket(
		const grpc::string_ref &ticket,
		int *retflags, 					///< 1- identified user, 0- unidentified.
		int64_t *retorgid, 				///< return group id. Reserved. Must be 0.
		int64_t *retuserid				///< return user id. 0- User is not valid.
	) = 0;
	/// abstract method checks user/password pair set by client. Not used now.
	virtual bool onCheckPassword(
		const grpc::string_ref &user,
		const grpc::string_ref &password,
		int *retflags, 					///< 1- identified user, 0- unidentified.
		int64_t *retorgid, 				///< return group id. Reserved. Must be 0.
		int64_t *retuserid				///< return user id. 0- User is not valid.
	) = 0;
};

/**
  *	server side ticket authenticator
  */
class RcrAuthMetadataProcessor : public AuthMetadataProcessor
{
private:
	bool mIsRequestUserCertificate;
	RcrValidator *mRcrValidator;
	void debugPrintAuthContext(std::ostream &strm, AuthContext* context);
public:
	RcrAuthMetadataProcessor(
			bool isblocking,
			bool isRequestUserCertificate,
			RcrValidator *RcrValidator)
	: mIsRequestUserCertificate(isRequestUserCertificate),
	  mRcrValidator(RcrValidator)
	{

	};

	// Interface implementation
	bool IsBlocking() const GRPC_OVERRIDE { return true; }
	bool isRequestUserCertificate() const { return mIsRequestUserCertificate; }
	Status Process(const InputMetadata& auth_metadata, AuthContext* context,
			OutputMetadata* consumed_auth_metadata,
			OutputMetadata* response_metadata) GRPC_OVERRIDE;
};

/*
 *	client side plugin provides metadata
 */
class RcrMetadataCredentialsPlugin : public MetadataCredentialsPlugin
{
private:
	grpc::string mClientTicket;
public:
	/// Return meta name used in authentication as char*
	static const char* getMetaTicketName();
	/// Return meta name used in authentication as std::string
	static const std::string& getMetaTicketNameStr();
	/// Return meta name used in authentication as string_ref
	static const grpc::string_ref getMetaTicketNameRef();
	/// Return meta user name used in authentication as char*
	static const char* getMetaUserName();
	/// client authentication. Client creates token
	RcrMetadataCredentialsPlugin(
		const std::string& username,
		const std::string &password
	);
	/// Client insert credentials ticket here
	grpc::Status GetMetadata(
		grpc::string_ref service_url,
		grpc::string_ref method_name,
		const grpc::AuthContext& channel_auth_context,
		std::multimap<grpc::string, grpc::string>* metadata		///< Insert ticket here
	) override;
};

#endif
