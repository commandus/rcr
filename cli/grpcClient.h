/*
 * grpclient.h
 */

#ifndef GRPCCLIENT_H_
#define GRPCCLIENT_H_

#include <iostream>
#include <memory>
#include <string>
#include <grpc++/grpc++.h>

#include "gen/rcr.pb-odb.hxx"
#include "gen/rcr.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;


class RcrClient
{
private:
	std::unique_ptr<rcr::Rcr::Stub> mStub;
	int repeats;
public:
    RcrClient(
		const std::string &interface,
		int port, 
		bool sslOn,
		int repeats
	);
	virtual ~RcrClient();
	// method wrappers
};

#endif /* GRPCCLIENT_H_ */
