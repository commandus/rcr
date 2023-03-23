/*
 * grpcClient.cpp
 */

#include <iostream>

// json
#include <google/protobuf/util/json_util.h>

#include "grpcClient.h"
#include "RcrCredentials.h"
#include "AppSettings.h"

RcrClient::RcrClient(
    std::shared_ptr<grpc::Channel> channel,
    const std::string &username,
    const std::string &password
)
{
    stub = rcr::Rcr::NewStub(channel);
}

RcrClient::~RcrClient()
{
}

uint64_t RcrClient::version()
{
    std::string r;
    grpc::ClientContext ctx;
    rcr::VersionRequest request;
    request.set_value(1);
    rcr::VersionResponse response;

    grpc::Status status = stub->version(&ctx, request, &response);
    if (!status.ok())
        return 0;
    return response.value();
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
    grpc::ClientContext context;
    rcr::OperationResponse response;
    rcr::ChPropertyTypeRequest request;
    request.set_operationsymbol("+");
    request.mutable_value()->set_key(key);
    request.mutable_value()->set_description(description);

    grpc::Status status = stub->chPropertyType(&context, request, &response);
    if (!status.ok()) {
        std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
        return status.error_code();
    }
    int32_t c = response.code();
    if (c) {
        std::string d = response.description();
        std::cerr << "Error: " << c << " " << d << std::endl;
        return c;
    }
	return r;
}

int32_t RcrClient::cardQuery(
    std::ostream &ostream,
    const std::string &query,
    size_t offset,
    size_t size
) {
    uint32_t r = 0;
    grpc::ClientContext context;
    rcr::CardQueryResponse response;
    rcr::CardQueryRequest request;
    request.set_query(query);
    request.mutable_list()->set_offset(offset);
    request.mutable_list()->set_size(size);

    grpc::Status status = stub->cardQuery(&context, request, &response);
    if (!status.ok()) {
        std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
        return status.error_code();
    }

    int32_t c = response.rslt().code();

    if (c) {
        std::cerr << "Error: " << c << " " << response.rslt().description() << std::endl;
        return c;
    }

    // print cards if exists
    for (auto c = 0; c < response.cards().count(); c++) {

    }

    return 0;
}

/**
 *
 * @param rows
 * @return
 *     rpc cardLoad(stream Card) returns (OperationResponse) {}
 */
int RcrClient::saveSpreadsheet(
    uint64_t box,
    const std::string componentSymbol,  ///< "U"- IC
    const std::vector<SheetRow> &rows
) {
    rcr::OperationResponse response;
    rcr::CardRequest card;
    card.set_symbol_name("+"); // add

    grpc::ClientContext context2;
    std::unique_ptr<grpc::ClientWriter<rcr::CardRequest> > writer(stub->cardPush(&context2, &response));
    for (auto item = rows.begin(); item != rows.end(); item++) {
        item->toCardRequest("+", box, card);
        if (!writer->Write(card)) {
            // Broken stream
            break;
        }
    }
    writer->WritesDone();
    grpc::Status status = writer->Finish();
    if (!status.ok()) {
        std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
        return status.error_code();
    }
    return 0;
}

std::string RcrClient::getDictionariesJson() {
    grpc::ClientContext context;
    rcr::DictionariesResponse response;
    rcr::DictionariesRequest request;
    request.set_flags(0);

    grpc::Status status = stub->getDictionaries(&context, request, &response);
    if (!status.ok()) {
        std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
        return "{}";
    }
    std::string r;
    google::protobuf::util::JsonPrintOptions formattingOptions;
    formattingOptions.add_whitespace = true;
    formattingOptions.always_print_primitive_fields = true;
    formattingOptions.preserve_proto_field_names = true;
    google::protobuf::util::MessageToJsonString(response, &r, formattingOptions);
    return r;
}
