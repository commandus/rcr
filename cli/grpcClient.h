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
public:
	std::unique_ptr<rcr::Rcr::Stub> stub;
    RcrClient(
        std::shared_ptr<Channel> channel,
        const std::string &username,
        const std::string &password
	);
	virtual ~RcrClient();
	// method wrappers
    std::string version();
    int32_t addPropertyType(
            const std::string &key,
            const std::string &description
    );

};

#endif /* GRPCCLIENT_H_ */
