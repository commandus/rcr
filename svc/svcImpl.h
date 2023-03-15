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
class RcrImpl : public rcr::Rcr::Service
{
private:
	struct ServiceConfig *mConfig;
protected:
	/// return to client status: no permission
	static const grpc::Status& STATUS_NO_GRANTS;
	/// return to client status: method not implemented yet
	static const grpc::Status& STATUS_NOT_IMPLEMENTED;

    template <class T>
    std::unique_ptr<T> load(uint64_t id);
public:
	/// ODB database
	odb::database *mDb;
	explicit RcrImpl(struct ServiceConfig *config);
	virtual ~RcrImpl();

	/// Return service configuration
	struct ServiceConfig *getConfig();

    // ------------------ front office ------------------

    /// get User by CN or identifier
    ::grpc::Status cardSearchEqual(::grpc::ServerContext* context, const ::rcr::EqualSearchRequest* request, ::rcr::CardResponse* response) override;
    ::grpc::Status chPropertyType(::grpc::ServerContext* context, const ::rcr::ChPropertyTypeRequest* request, ::rcr::OperationResponse* response) override;
    // ------------------ back office ------------------
};

#endif
