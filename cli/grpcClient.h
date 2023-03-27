/*
 * grpclient.h
 */

#ifndef GRPCCLIENT_H_
#define GRPCCLIENT_H_

#include <string>
#include <grpc++/grpc++.h>

#include "gen/rcr.grpc.pb.h"
#include "SpreadSheetHelper.h"

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
    uint64_t version();
    int32_t addPropertyType(
        const std::string &key,
        const std::string &description
    );

    int32_t cardQuery(
        std::ostream &ostream,
        const std::string &query,
        const std::string &measureSymbol,
        size_t offset,
        size_t size,
        bool json
    );

    int saveSpreadsheet(
        uint64_t box,
        const std::string componentSymbol,  ///< "U"- IC
        const std::vector<SheetRow> &rows
    );

    std::string getDictionariesJson();

    std::string getBoxJson(
        uint64_t minBox,
        size_t offset,
        size_t size
    );

    void printBox(std::ostream &strm, uint64_t boxId, size_t offset, size_t size);
};

#endif /* GRPCCLIENT_H_ */
