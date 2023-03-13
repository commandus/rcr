/*
 * grpclient.h
 */

#ifndef GRPCCLIENT_H_
#define GRPCCLIENT_H_

#include <iostream>
#include <memory>
#include <string>
#include <grpc++/grpc++.h>

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
        const std::string &username,
        const std::string &password,
        int repeats
	);
	virtual ~RcrClient();
	// method wrappers
    int32_t addPropertyType(
            const rcr::PropertyType &value
    );
};

#endif /* GRPCCLIENT_H_ */
