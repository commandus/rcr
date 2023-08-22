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
#include "login-plugin.h"

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
	ServiceConfig *mConfig;
    LoginPlugins loginPlugins;
    int loadDictionaries(
        rcr::DictionariesResponse *pResponse,
        MEASURE_LOCALE locale
    );
    /**
     * Set user id if user found and password is correct
     * @param retUserId return user id if not null
     * @param db
     * @param user
     * @return -1 if user not found or password is incorrect
     */
    int checkUserRights(
        uint64_t *retUserId,
        odb::database *db,
        const rcr::User &user
    );

    bool checkCredentialsNSetToken(
        odb::transaction &t,
        odb::database *db,
        rcr::User *retVal
    );
    bool setCredentialsNSetToken(
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
        uint64_t userId,
        const std::string &symbol,
        const rcr::ExcelFile &file,
        uint64_t prefixBox,
        bool numberInFilename,
        rcr::DictionariesResponse &dictionaries
    );
    void removePropertyFromCards(
        odb::database *db,
        odb::transaction &t,
        uint64_t id
    );
    int removePackagesFromBox(
        odb::database *db,
        odb::transaction &t,
        uint64_t id
    );
    void updateCardPackages(
        odb::database *db,
        odb::transaction &t,
        const rcr::ChCardRequest *request
    );
    void updateCardPackage(
        odb::database *db,
        odb::transaction &t,
        const rcr::ChCardRequest *request,
        uint64_t packageId
    );

    bool loadUser(
        rcr::User *retVal,
        odb::database *db,
        uint64_t id
    );

    bool loadPackage(
        rcr::Package *retval,
        odb::database *db,
        uint64_t id
    );

    bool loadCard(
        rcr::Card *retval,
        odb::database *db,
        uint64_t id
    );

    /**
     * Move packages from one box to another
     * @param db database
     * @param t transaction
     * @param oldBox source
     * @param newBox destination
     * @return 0- success
     */
    int changePackageBox(
        odb::database *db,
        odb::transaction &t,
        uint64_t oldBox,
        uint64_t newBox
    );

    bool removeCard(
        odb::database *db,
        odb::transaction &t,
        const rcr::ChCardRequest *request
    );
    /**
     * Return true if card is empty, false-
     * @param db
     * @param t
     * @param request
     * @return
     */
    bool removePackage(
        odb::database *db,
        odb::transaction &t,
        const rcr::ChCardRequest *request
    );
    int exportExcelFile(
        rcr::ExportExcelResponse *retVal,
        odb::transaction &t,
        odb::database *db,
        const std::string &query,
        const std::string &symbolName,
        time_t timestamp,
        rcr::DictionariesResponse *dictionaries
    );

public:
	/// ODB database
	odb::database *mDb;
	explicit RcrImpl(ServiceConfig *config);
	virtual ~RcrImpl();

	/// Return service configuration
	ServiceConfig *getConfig();

    // ------------------ front office ------------------

    grpc::Status login(grpc::ServerContext* context, const rcr::LoginRequest* request, rcr::LoginResponse* response) override;
    grpc::Status chPropertyType(::grpc::ServerContext* context, const ::rcr::ChPropertyTypeRequest* request, ::rcr::OperationResponse* response) override;
    grpc::Status chBox(grpc::ServerContext* context, const rcr::ChBoxRequest* request, rcr::OperationResponse* response) override;
    grpc::Status getCard(grpc::ServerContext* context, const rcr::GetItemRequest* request, rcr::CardNPropetiesPackages* response) override;
    grpc::Status chCard(grpc::ServerContext* context, const rcr::ChCardRequest* request, rcr::OperationResponse* response) override;
    grpc::Status cardQuery(::grpc::ServerContext* context, const rcr::CardQueryRequest* request, rcr::CardQueryResponse* response) override;
    grpc::Status cardPush(::grpc::ServerContext* context, grpc::ServerReader< rcr::CardRequest>* reader, rcr::OperationResponse* response) override;
    grpc::Status getDictionaries(::grpc::ServerContext* context, const rcr::DictionariesRequest* request, rcr::DictionariesResponse* response) override;
    grpc::Status getBox(::grpc::ServerContext* context, const ::rcr::BoxRequest* request, rcr::BoxResponse* response) override;
    grpc::Status importExcel(grpc::ServerContext* context, const rcr::ImportExcelRequest* request, rcr::OperationResponse* response);
    grpc::Status exportExcel(grpc::ServerContext* context, const rcr::ExportExcelRequest* request, rcr::ExportExcelResponse* response);
    // ------------------ back office ------------------
    grpc::Status lsUser(grpc::ServerContext* context, const rcr::UserRequest* request, rcr::UserResponse* response) override;
    grpc::Status chUser(grpc::ServerContext* context, const rcr::UserRequest* request, rcr::OperationResponse* response) override;
    grpc::Status lsJournal(grpc::ServerContext* context, const rcr::JournalRequest* request, rcr::JournalResponse* response) override;
};

#endif
