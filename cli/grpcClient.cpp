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
#include "AppSettings.h"


RcrClient::RcrClient(
    std::shared_ptr<Channel> channel,
    const std::string &username,
    const std::string &password
)
{
    stub = rcr::Rcr::NewStub(channel);
}

RcrClient::~RcrClient()
{
}

std::string RcrClient::version()
{
    std::string r;
    ClientContext ctx;
    rcr::VersionRequest request;
    request.set_value(1);
    rcr::VersionResponse *response = nullptr;

    Status status = stub->version(&ctx, request, response);
    if (!status.ok()) {
        std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
        return "Error";
    }
    return response->name();
}

/**
 * Add
 * @param
 * @return 0- success, -1- fatal error, >0- count of warnings(unsuccessful trips)
 */
int32_t RcrClient::addPropertyType(
	const std::string &key,
    const std::string &description
)
{
    uint32_t r = 0;
    ClientContext context;
    rcr::OperationResponse *response = nullptr;
    rcr::ChPropertyTypeRequest request;
    request.set_operationsymbol("+");
    request.mutable_value()->set_key(key);
    request.mutable_value()->set_description(description);

    Status status = stub->chPropertyType(&context, request, response);
    if (!status.ok()) {
        std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
        return -1;
    }
    int32_t c = response->code();
    if (c) {
        std::string d = response->description();
        std::cerr << "Error: " << c << " " << d << std::endl;
        return c;
    }
	return r;
}
