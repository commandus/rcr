/*
 * RcrCredentials.cpp
 */

#include <iostream>
#include <sstream>
#include <grpcpp/security/server_credentials.h>
#include "RcrCredentials.h"

using grpc::SslServerCredentialsOptions;

const char *META_AUTH_TICKET = "x-auth-ticket";
const std::string &META_AUTH_TICKET_STR(META_AUTH_TICKET);
const grpc::string_ref META_AUTH_TICKET_STR_REF(META_AUTH_TICKET_STR);
const char *META_AUTH_USER = "x-auth-user";
const char DELIM_TICKET = ' ';
const char *METADATA_PATH = ":path";
// this is special case when no SSL certificate is not required
const std::string NOSSL_GETCERTIFICATE_METADATA_PATH = "/rcr.Rcr/";

const char *RcrMetadataCredentialsPlugin::getMetaTicketName()
{
	return META_AUTH_TICKET;
}

const std::string& RcrMetadataCredentialsPlugin::getMetaTicketNameStr()
{
	return META_AUTH_TICKET_STR;
}

const grpc::string_ref RcrMetadataCredentialsPlugin::getMetaTicketNameRef()
{
	return META_AUTH_TICKET_STR_REF;
}

const char *RcrMetadataCredentialsPlugin::getMetaUserName()
{
	return META_AUTH_USER;
}

const grpc::string ERR_INV_TICKET("Invalid ticket");
const grpc::string ERR_INV_CERTIFICATE("Invalid certificate");

// Client creates token
RcrMetadataCredentialsPlugin::RcrMetadataCredentialsPlugin(const std::string& username, const std::string &password)
{
	// ticket is combination of user name and password
	std::stringstream ssticket;
	ssticket << username << "|" << password;
	mClientTicket = ssticket.str();
}

// Client insert credentials ticket here
grpc::Status RcrMetadataCredentialsPlugin::GetMetadata(grpc::string_ref service_url, grpc::string_ref method_name,
		const grpc::AuthContext& channel_auth_context,
		std::multimap<grpc::string, grpc::string>* metadata)
{
	metadata->insert(std::make_pair(META_AUTH_TICKET, mClientTicket));
	return Status::OK;
}

void RcrAuthMetadataProcessor::debugPrintAuthContext(std::ostream &strm, AuthContext* context)
{
	for (grpc::AuthPropertyIterator it(context->begin()); it != context->end(); ++it)
	{
		strm << (*it).first << ":" << (*it).second;
	}
}

/**
  * server side
  * Check user by SSL certificate, or token.
  * x509_common_name
  * There are no password check.
  */
Status RcrAuthMetadataProcessor::Process(
	const InputMetadata& auth_metadata, AuthContext* context,
	OutputMetadata* consumed_auth_metadata, OutputMetadata* response_metadata
)
{
	if (!mRcrValidator)
		return Status::OK;
	int f;
	int64_t uid, oid;
	if (mIsRequestUserCertificate) {
		// check certificate CN if server required
		std::vector<grpc::string_ref> id = context->GetPeerIdentity();
		// client may provide 0, 1 or more identities.
		bool ok = false;
		for (std::vector<grpc::string_ref>::iterator it = id.begin(); it != id.end(); it++)
		{
			if (mRcrValidator->onCheckCertificateCN(*it, &f, &oid, &uid)) {
				ok = true;
				break;
			}
		}
		if (!ok) {
			// This is special case	:path: /nfcreceipt.NFCReceipt/getNice
			auto metapath = auth_metadata.find(METADATA_PATH);
			if (metapath != auth_metadata.end())
			{
				std::string p(metapath->second.data());
				if (p.find(NOSSL_GETCERTIFICATE_METADATA_PATH) == 0)
					return Status::OK;
			}
			return Status(grpc::StatusCode::UNAUTHENTICATED, ERR_INV_CERTIFICATE);
		}
	}
	else
	{
		// try get from metadata
		auto metaAuth = auth_metadata.find(RcrMetadataCredentialsPlugin::getMetaTicketName());
		if (metaAuth == auth_metadata.end())
			return Status(grpc::StatusCode::UNAUTHENTICATED, ERR_INV_TICKET);

		if (!mRcrValidator->onCheckTicket(metaAuth->second, &f, &uid, &oid))
			return Status(grpc::StatusCode::UNAUTHENTICATED, ERR_INV_TICKET);
	}

	std::stringstream ss;
	ss << oid << DELIM_TICKET << uid << DELIM_TICKET << f;
	std::string s(ss.str());
	context->AddProperty(RcrMetadataCredentialsPlugin::getMetaUserName(), s);
	// LOG(INFO) << "Process: " << s;
	return Status::OK;
}
