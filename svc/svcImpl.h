/*
 * SvcImpl.h
 */

#ifndef SVCIMPL_H_
#define SVCIMPL_H_

#include <memory>
#include <vector>

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>

#include "gen/rcr.grpc.pb.h"
#include "gen/rcr.grpc.pb.h"

#include "MeasureUnit.h"
#include "svcconfig.h"

using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;

/**
 * Class used in authorization
 */
class UserIds {
public:
	UserIds() : id(0), oid(0), roleflags(0) {};
	uint64_t	id;		///< User identifier. 0- not identified
	uint64_t	oid;	///< Group identifier. Reserved. Must be set to 0.
	int roleflags;		///< User role. 0- not identified. Must be set to 1.
};

/**
 * gRPC OneWayTicket implementation with ODB (postgresql) backend
 */
class RcrImpl : public rcr::Rcr::Service {
private:
	struct ServiceConfig *mConfig;
    int loadDictionaries(
        rcr::DictionariesResponse *pResponse,
        MEASURE_LOCALE locale
    );
    grpc::Status importExcel(
        grpc::ServerContext* context,
        const rcr::ImportExcelRequest* request,
        rcr::OperationResponse* response
    );

    /**
     *
     * @param t
     * @param db
     * @param user
     * @return -1 if user not found or password is incorrect
     */
    int checkUserRights(
        odb::transaction &t,
        odb::database *db,
        const rcr::User &user
    );

    bool checkCredentialsNSetToken(
        odb::transaction &t,
        odb::database *db,
        rcr::User *retVal
    );

    uint64_t generateNewToken();

protected:
	/// return to client status: no permission
	static const grpc::Status& STATUS_NO_GRANTS;
	/// return to client status: method not implemented yet
	static const grpc::Status& STATUS_NOT_IMPLEMENTED;
    template <class T> std::unique_ptr<T> load(uint64_t id);
    size_t importExcelFile(
        odb::transaction &t,
        odb::database *db,
        const std::string &symbol,
        const rcr::ExcelFile &file,
        uint64_t prefixBox,
        bool numberInFilename,
        rcr::DictionariesResponse &dictionaries
    );
public:
	/// ODB database
	odb::database *mDb;
	explicit RcrImpl(struct ServiceConfig *config);
	virtual ~RcrImpl();

	/// Return service configuration
	struct ServiceConfig *getConfig();

    // ------------------ front office ------------------

    grpc::Status login(::grpc::ServerContext* context, const ::rcr::LoginRequest* request, ::rcr::LoginResponse* response) override;
    grpc::Status chPropertyType(::grpc::ServerContext* context, const ::rcr::ChPropertyTypeRequest* request, ::rcr::OperationResponse* response) override;
    grpc::Status cardQuery(::grpc::ServerContext* context, const ::rcr::CardQueryRequest* request, ::rcr::CardQueryResponse* response) override;
    grpc::Status cardPush(::grpc::ServerContext* context, ::grpc::ServerReader< ::rcr::CardRequest>* reader, ::rcr::OperationResponse* response) override;
    grpc::Status getDictionaries(::grpc::ServerContext* context, const ::rcr::DictionariesRequest* request, ::rcr::DictionariesResponse* response) override;
    grpc::Status getBox(::grpc::ServerContext* context, const ::rcr::BoxRequest* request, ::rcr::BoxResponse* response) override;
    // ------------------ back office ------------------
    grpc::Status lsUser(grpc::ServerContext* context, const rcr::UserRequest* request, grpc::ServerWriter< rcr::User>* writer) override;
};

#endif
