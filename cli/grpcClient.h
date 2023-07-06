/*
 * grpclient.h
 */

#ifndef GRPCCLIENT_H_
#define GRPCCLIENT_H_

#include <string>
#include <grpc++/grpc++.h>

#include "gen/rcr.grpc.pb.h"
#include "SpreadSheetHelper.h"
#include "MeasureUnit.h"

class RcrClient
{
public:
	std::unique_ptr<rcr::Rcr::Stub> stub;
    RcrClient(
        std::shared_ptr<grpc::Channel> channel,
        const std::string &username,
        const std::string &password
	);
	virtual ~RcrClient();

	// method wrappers
    bool login(
        const rcr::User *user
    );
    int32_t addPropertyType(
        const std::string &key,
        const std::string &description
    );

    int32_t cardQuery(
        std::ostream &ostream,
        const rcr::User &user,
        const std::string &query,
        const std::string &measureSymbol,
        size_t offset,
        size_t size,
        bool json
    );

    int saveSpreadsheet(
        uint64_t box,
        const std::string componentSymbol,  ///< "U"- IC
        const std::vector<SheetRow> &rows,
        const rcr::User *user
    );

    std::string getDictionariesJson();

    std::string getBoxJson(
        uint64_t minBox,
        size_t offset,
        size_t size
    );

    void printBox(
        std::ostream &strm,
        uint64_t boxId,
        size_t offset,
        size_t size
    );

    void printUser(
        std::ostream &strm,
        const rcr::User *user
    );

    void printSymbols(
        std::ostream &strm,
        MEASURE_LOCALE locale
    );

    void printProperty(
        std::ostream &strm
    );

    void printBoxes(
        std::ostream &ostream,
        size_t offset,
        size_t size,
        const rcr::User *user
    );

    void changeProperty(
        const std::string &clause,
        const rcr::User *user
    );

    void chBox(
        const char operation,
        uint64_t sourceBox,
        uint64_t destBox,
        const std::string &name,
        const rcr::User *user
    );
};

#endif /* GRPCCLIENT_H_ */
