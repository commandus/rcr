/*
 * grpcClient.cpp
 */

#include <iostream>

// json
#include <google/protobuf/util/json_util.h>

#include "grpcClient.h"
#include "RcrCredentials.h"
#include "AppSettings.h"
#include "MeasureUnit.h"
#include "StockOperation.h"

static std::string message2Json(
    const google::protobuf::Message& message
)
{
    google::protobuf::util::JsonPrintOptions formattingOptions;
    formattingOptions.add_whitespace = true;
    formattingOptions.always_print_primitive_fields = true;
    formattingOptions.preserve_proto_field_names = true;
    std::string r;
    google::protobuf::util::MessageToJsonString(message, &r, formattingOptions);
    return r;
}

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

bool RcrClient::login(
    const rcr::User *user
)
{
    std::string r;
    grpc::ClientContext ctx;
    rcr::LoginRequest request;
    *request.mutable_user() = *user;
    rcr::LoginResponse response;

    grpc::Status status = stub->login(&ctx, request, &response);
    if (!status.ok())
        return false;
    return response.success();
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
    const rcr::User &user,
    const std::string &query,
    const std::string &measureSymbol,
    size_t offset,
    size_t size,
    bool json
) {
    uint32_t r = 0;
    grpc::ClientContext context;
    rcr::CardQueryResponse response;
    rcr::CardQueryRequest request;
    *request.mutable_user() = user;
    request.set_query(query);
    request.set_measure_symbol(measureSymbol);
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
    } else {
        google::protobuf::util::JsonPrintOptions formattingOptions;
        formattingOptions.add_whitespace = true;
        formattingOptions.always_print_primitive_fields = true;
        formattingOptions.preserve_proto_field_names = true;

        // print cards if exists
        for (auto card = response.cards().cards().begin(); card != response.cards().cards().end(); card++) {
            std::string r;
            if (json) {
                google::protobuf::util::MessageToJsonString(*card, &r, formattingOptions);
                ostream << r << std::endl;
            } else {
                std::string n = card->card().name();
                if (n.empty()) {
                    ostream << MeasureUnit::value(ML_RU, (COMPONENT) (card->card().symbol_id() - 1), card->card().nominal());
                } else {
                    ostream << card->card().name();
                }
                for (auto p = card->properties().begin(); p != card->properties().end(); p++) {
                    ostream
                        << p->property_type()
                        << ": " << p->value();
                }
                for (auto p = card->packages().begin(); p != card->packages().end(); p++) {
                    ostream << " " << StockOperation::boxes2string(p->box()) << ": " << p->qty();
                }
                ostream << std::endl;
            }
        }
    }
    return c;
}

/**
 *
 * @param rows
 * @return
 *     rpc cardLoad(stream Card) returns (OperationResponse) {}
 */
int RcrClient::saveSpreadsheet(
    uint64_t box,
    const std::string componentSymbol,  ///< "D"- IC
    const std::vector<SheetRow> &rows
) {
    rcr::OperationResponse response;
    grpc::ClientContext context2;
    std::unique_ptr<grpc::ClientWriter<rcr::CardRequest> > writer(stub->cardPush(&context2, &response));
    for (auto item = rows.begin(); item != rows.end(); item++) {
        rcr::CardRequest card;
        item->toCardRequest("+", componentSymbol, box, card);
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

void RcrClient::printBox(
    std::ostream &ostream,
    uint64_t minBox,
    size_t offset,
    size_t size
)
{
    grpc::ClientContext context;
    rcr::BoxResponse response;
    rcr::BoxRequest request;
    request.set_start(minBox);
    request.set_depth(4);
    request.mutable_list()->set_offset(offset);
    request.mutable_list()->set_size(size);

    grpc::Status status = stub->getBox(&context, request, &response);
    if (!status.ok()) {
        std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
        return;
    }
    // print cards if exists
    for (auto box = response.box().begin(); box != response.box().end(); box++) {
        std::string r = StockOperation::boxes2string(box->box_id());
        ostream << r << std::endl;
    }
}

std::string RcrClient::getBoxJson(
    uint64_t minBox,
    size_t offset,
    size_t size
) {
    grpc::ClientContext context;
    rcr::BoxResponse response;
    rcr::BoxRequest request;
    request.set_start(minBox);
    request.set_depth(4);
    request.mutable_list()->set_offset(offset);
    request.mutable_list()->set_size(size);

    grpc::Status status = stub->getBox(&context, request, &response);
    if (!status.ok()) {
        std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
        return "{}";
    }
    return message2Json(response);
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
    return message2Json(response);
}

void RcrClient::printUser(
    std::ostream &strm,
    rcr::User *user
) {
    grpc::ClientContext context;
    rcr::UserRequest request;
    request.mutable_user()->set_name(user->name());
    request.mutable_user()->set_password(user->password());
    auto reader = stub->lsUser(&context, request);
    rcr::User u;
    while (reader->Read(&u)) {
        strm << u.name() << "\t" << (u.rights() & 1 ? "Admin" : "");
        if (!u.password().empty())
            strm << "\t" << u.password();
        strm << std::endl;
    }
}

void RcrClient::printSymbols(
        std::ostream &strm
) {
    grpc::ClientContext context;
    rcr::DictionariesResponse response;
    rcr::DictionariesRequest request;
    request.set_flags(0);

    grpc::Status status = stub->getDictionaries(&context, request, &response);
    if (!status.ok()) {
        std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
        return;
    }
    for (auto s(response.symbol().begin()); s != response.symbol().end(); s++) {
        strm << s->sym() << "\t" << s->description() << std::endl;
    }
}

void RcrClient::printProperty(
        std::ostream &strm
) {
    grpc::ClientContext context;
    rcr::DictionariesResponse response;
    rcr::DictionariesRequest request;
    request.set_flags(0);

    grpc::Status status = stub->getDictionaries(&context, request, &response);
    if (!status.ok()) {
        std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
        return;
    }
    for (auto p(response.property_type().begin()); p != response.property_type().end(); p++) {
        strm << p->key() << "\t" << p->description() << std::endl;
    }

}
