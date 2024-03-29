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
#include "string-helper.h"

// i18n
#include <libintl.h>
#define _(String) gettext (String)

#include <xlnt/workbook/workbook.hpp>

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
        std::cerr << _("Error: ") << status.error_code() << " " << status.error_message() << std::endl;
        return status.error_code();
    }
    int32_t c = response.code();
    if (c) {
        std::string d = response.description();
        std::cerr << _("Error: ") << c << " " << d << std::endl;
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
        std::cerr << _("Error: ") << status.error_code() << " " << status.error_message() << std::endl;
        return status.error_code();
    }

    int32_t c = response.rslt().code();

    if (c) {
        std::cerr << _("Error: ") << c << " " << response.rslt().description() << std::endl;
        return c;
    } else {
        google::protobuf::util::JsonPrintOptions formattingOptions;
        formattingOptions.add_whitespace = true;
        formattingOptions.always_print_primitive_fields = true;
        formattingOptions.preserve_proto_field_names = true;

        if (response.cards().cards_size() == 0) {
            // it can be aggregate sum or count
            if (response.rslt().count() || response.rslt().sum())
                ostream << response.rslt().count() << " " << response.rslt().sum() << std::endl;
        }
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
                    ostream << " "
                        << p->property_type()
                        << ":" << p->value();
                }
                for (auto p = card->packages().begin(); p != card->packages().end(); p++) {
                    ostream << " " << StockOperation::boxes2string(p->box()) << " = " << p->qty();
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
    const std::vector<SheetRow> &rows,
    const rcr::User *user
) {
    rcr::OperationResponse response;
    grpc::ClientContext context2;
    std::unique_ptr<grpc::ClientWriter<rcr::CardRequest> > writer(stub->cardPush(&context2, &response));
    for (auto item = rows.begin(); item != rows.end(); item++) {
        rcr::CardRequest card;
        *card.mutable_user() = *user;
        item->toCardRequest("+", componentSymbol, box, card);
        if (!writer->Write(card)) {
            // Broken stream
            break;
        }
    }
    writer->WritesDone();
    grpc::Status status = writer->Finish();
    if (!status.ok()) {
        std::cerr << _("Error: ") << status.error_code() << " " << status.error_message() << std::endl;
        return status.error_code();
    }
    return 0;
}

int RcrClient::loadSpreadsheet(
    std::vector<std::string> &retVal,
    const rcr::User *user,
    const std::string &path,
    const std::string &query,
    const std::string &componentSymbol  ///< "C"- condensers
)
{
    grpc::ClientContext context;
    rcr::ExportExcelResponse response;
    rcr::ExportExcelRequest request;
    *request.mutable_user() = *user;
    request.set_query(query);
    request.set_symbol_name(componentSymbol);
    grpc::Status status = stub->exportExcel(&context, request, &response);
    if (!status.ok()) {
        std::cerr << _("Error: ") << status.error_code() << " " << status.error_message() << std::endl;
        return status.error_code();
    }
    for (int f = 0; f < response.file_size(); f++) {
        xlnt::workbook wb;
        std::stringstream ss(response.file(f).content());
        try {
            wb.load(ss);
            wb.save(path + "/" + response.file(f).name());
            retVal.emplace_back(response.file(f).name());
        } catch (...) {
            std::cerr << _("Error save spreadsheets") << std::endl;
        }
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
        std::cerr << _("Error: ") << status.error_code() << " " << status.error_message() << std::endl;
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
        std::cerr << _("Error: ") << status.error_code() << " " << status.error_message() << std::endl;
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
        std::cerr << _("Error: ") << status.error_code() << " " << status.error_message() << std::endl;
        return "{}";
    }
    return message2Json(response);
}

void RcrClient::printUser(
    std::ostream &strm,
    const rcr::User *user
) {
    grpc::ClientContext context;
    rcr::UserRequest request;
    request.mutable_user()->set_name(user->name());
    request.mutable_user()->set_password(user->password());
    strm << _("Current user: ") << request.user().name() << std::endl;
    
    rcr::UserResponse response;
    stub->lsUser(&context, request, &response);
    for (int i = 0; i < response.user_size(); i++) {
        strm << response.user(i).name() << "\t" << (response.user(i).rights() & 1 ? _("Admin") : "");
        if (!response.user(i).password().empty())
            strm << "\t" << response.user(i).password();
        strm << std::endl;
    }
}

void RcrClient::printSymbols(
    std::ostream &strm,
    MEASURE_LOCALE locale
) {
    grpc::ClientContext context;
    rcr::DictionariesResponse response;
    rcr::DictionariesRequest request;
    request.set_locale_id(locale);
    request.set_flags(0);

    grpc::Status status = stub->getDictionaries(&context, request, &response);
    if (!status.ok()) {
        std::cerr << _("Error: ") << status.error_code() << " " << status.error_message() << std::endl;
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
        std::cerr << _("Error: ") << status.error_code() << " " << status.error_message() << std::endl;
        return;
    }
    for (auto p(response.property_type().begin()); p != response.property_type().end(); p++) {
        strm << p->key() << "\t" << p->description() << std::endl;
    }
}

void RcrClient::printBoxes(
    std::ostream &strm,
    size_t offset,
    size_t size,
    const rcr::User *user
) {
    grpc::ClientContext context;
    rcr::BoxResponse response;
    rcr::BoxRequest request;
    request.mutable_list()->set_offset(offset);
    request.mutable_list()->set_size(size);
    *request.mutable_user() = *user;

    grpc::Status status = stub->getBox(&context, request, &response);
    if (!status.ok()) {
        std::cerr << _("Error: ") << status.error_code() << " " << status.error_message() << std::endl;
        return;
    }
    for (auto box(response.box().begin()); box != response.box().end(); box++) {
        std::string sbid = StockOperation::boxes2string(box->box_id());
        strm << sbid;
        if (!box->name().empty() && (sbid != box->name()))
            strm << "\t" << box->name();
        strm << std::endl;
    }
}

void RcrClient::changeProperty(
    const std::string &clause,
    const rcr::User *user
) {
    grpc::ClientContext context;
    rcr::OperationResponse response;
    rcr::ChPropertyTypeRequest request;
    std::vector <std::string> clauses = split(clause, ' ');
    if (clauses.size() < 3) {
        if (!(clauses[0] == "-" && clauses.size() == 2)) {
            std::cerr << _("Error: ") << -1 << " " << _("Invalid arguments") << std::endl;
            return;
        }
    }
    request.set_operationsymbol(clauses[0]);
    request.mutable_value()->set_key(clauses[1]);
    if (clauses.size() >= 3)
        request.mutable_value()->set_description(clauses[2]);
    *request.mutable_user() = *user;

    grpc::Status status = stub->chPropertyType(&context, request, &response);
    if (!status.ok()) {
        std::cerr << _("Error: ") << status.error_code() << " " << status.error_message() << std::endl;
        return;
    }
    if (response.code()) {
        std::cerr << _("Error: ") << response.code() << " " << response.description() << std::endl;
        return;
    }
}

void RcrClient::chBox(
    const char operation,
    uint64_t sourceBox,
    uint64_t destBox,
    const std::string &name,
    const rcr::User *user
) {
    grpc::ClientContext context;
    rcr::OperationResponse response;
    rcr::ChBoxRequest request;
    request.mutable_value()->set_box_id(sourceBox);
    switch(operation) {
        case '+':
            request.set_operationsymbol("+");
            request.mutable_value()->set_name(name);
            break;
        case '-':
            request.set_operationsymbol("-");
            break;
        case '/':
            request.mutable_value()->set_id(destBox);
            request.set_operationsymbol("/");
            request.mutable_value()->set_name(name);
            break;
        case '=':
            request.set_operationsymbol("=");
            request.mutable_value()->set_name(name);
            break;
        default:
            break;
    }

    *request.mutable_user() = *user;

    grpc::Status status = stub->chBox(&context, request, &response);
    if (!status.ok()) {
        std::cerr << _("Error: ") << status.error_code() << " " << status.error_message() << std::endl;
        return;
    }
    if (response.code()) {
        std::cerr << _("Error: ") << response.code() << " " << response.description() << std::endl;
        return;
    }
}
