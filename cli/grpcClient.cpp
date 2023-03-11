/*
 * grpcClient.cpp
 */

#include <atomic>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <stack>

#include "grpcClient.h"
#include "RcrCredentials.h"

using grpc::Channel;
using grpc::ChannelCredentials;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::InsecureChannelCredentials;
using grpc::SslCredentials;
using grpc::SslCredentialsOptions;
using grpc::CallCredentials;
using grpc::CompositeChannelCredentials;
using grpc::MetadataCredentialsPlugin;


RcrClient::RcrClient(
    const std::string &interface,
    int port,
    bool sslOn,
    const std::string &username,
    const std::string &password,
    int arepeats
)
{
	// target host name and port
	std::stringstream ss;
	ss << interface << ":" << port;
	std::string target(ss.str());
	repeats = arepeats;

	std::shared_ptr<Channel> channel;
	if (sslOn) {
		SslCredentialsOptions sslOpts;
		// Server use self-signed certificate, so client must trust CA, issued server certificate
		std::shared_ptr<ChannelCredentials> channelCredentials = grpc::SslCredentials(sslOpts);
		std::shared_ptr<CallCredentials> callCredentials = MetadataCredentialsFromPlugin(std::unique_ptr<MetadataCredentialsPlugin>(
			new RcrMetadataCredentialsPlugin(username, password)));
		std::shared_ptr<ChannelCredentials> compositeChannelCredentials = grpc::CompositeChannelCredentials(channelCredentials, callCredentials);
		channel = grpc::CreateChannel(target, compositeChannelCredentials);
	}
	else
	{
		channel = grpc::CreateChannel(target, InsecureChannelCredentials());
	}
	mStub = rcr::Rcr::NewStub(channel);
}

RcrClient::~RcrClient()
{
}

/**
 * Add
 * @param
 * @return 0- success, -1- fatal error, >0- count of warnings(unsuccessful trips)
 */
int32_t RcrClient::addPropertyType(
	const rcr::PropertyType &value
)
{
	uint32_t r = 0;
	for (int i = 0; i < repeats; i++) {
		ClientContext context;
		rcr::Operation response;
		Status status = mStub->chPropertyType(&context, gtfs_trips, &response);
		if (!status.ok())
		{
			std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
			return -1;
		}
		for (int i = 0; i < response.codes_size(); i++)
		{
			if (response.codes(i))
				r++;
		}
		DEBUG_MEMORY_USAGE();
	}
	return r;
}
