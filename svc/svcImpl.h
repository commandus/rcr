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

using namespace odb::core;

#include <odb/database.hxx>
#include <odb/transaction.hxx>
#include <odb/pgsql/database.hxx>

#include "pbjson.hpp"


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
class OneWayTicketImpl : public onewayticket::OneWayTicket::Service
{
private:
	struct ServiceConfig *mConfig;
	/// get security ticket from the metadata. Reserved.
	std::string getAuthTicket(
		ServerContext *context				///< Service context
	);
	/// find user identifier by phone number string.
	uint64_t findUserIdByCN (
		const std::string &cn				///< normalized phone number (without '+', '-', '(', ')', spaces)
	);

	uint64_t findUserByCN (
		const std::string &cn, 
		User *retval
	);

	/**
	 * Return user role flags
	 */
	int getAuthUser (
		ServerContext *context, 			///< Service context
		UserIds *retuser					///< return user identifier, group and role flags.
	);
	/**
	 * Check is user have rights by the role, open database transaction.
	 * role mask: ORG_ROLE_MASK_SVC_ADMIN,..
	 */
	bool hasRole (
		ServerContext *context,				///< Service context
		int rolemask						///< flag(s) to check
	);

	// SSL
	std::string mSSLKey;					///< Service SSL private key
	std::string mSSLCertificate;			///< Service SSL certificate
	std::string mSSLCAKey;					///< CA private key used in User certificate issue procedure
	std::string mSSLCACacertificate;		///< CA certificate
	
	// OpenSSL buffers
	EVP_PKEY *mCAPKey;						///< OpenSSL specific
	X509_NAME *mCAName;						///< OpenSSL specific

protected:
	/// return to client status: no permission
	static const Status& STATUS_NO_GRANTS;
	/// return to client status: method not implemented yet
	static const Status& STATUS_NOT_IMPLEMENTED;

	/// generate a new certificate with specified id as common name
	bool mkCertificate(
		UserCertificate *certificate,		///< return generated certificate
		uint64_t id							///< user identifier
	);

	/// issue a new certificate
	uint64_t newCertificate(
		UserCertificate *certificate		///< return certificate
	);

	/**
	 * @brief return created/updated trip id
	 * @param request
	 * @param response
	 */
	void doSetTripSeats
	(
		const SetTripSeatsRequest *request,
		OperationResponse *response
	);

public:
	/// ODB database
	odb::database *mDb;
	explicit OneWayTicketImpl(struct ServiceConfig *config);
	virtual ~OneWayTicketImpl();

	/// Return service configuration
	struct ServiceConfig *getConfig();

	/// check does user exists
	bool existsUser
	(
		uint64_t uid					///< User identifier
	);

	/// persistent object load, log error etc. Return NULL if fails

	template <class T>
	std::unique_ptr<T> load (
		uint64_t id						///< ODB object identifier
	);

	/**
	 * Find ticket(s) by the trip identifier.
	 * See also FindTicket
	 */
	void listTripTickets
	(
		uint64_t tripId,
		TicketList *response
	);

	/**
	 * Find all seat(s) by the trip identifier
	 */
	void listTripSeats
	(
		const Trip *trip,
		SeatResponse *response
	);

	/**
	 * Find available seat(s) by the trip identifier
	 */
	void listTripAvailableSeats
	(
		const Trip *trip,
		SeatResponse *response
	);

	/**
	 * Get available seat(s).
	 * Return false if seat(s) occupied
	 */
	bool isSeatsAvailable
	(
		const Trip *trip,
		const BookingRequest *request,
		SeatResponse *seats
	);

	/**
	 * Find sold ticket(s) by the trip identifier
	 */
	void listTripOccupiedSeats
	(
		uint64_t tripId,
		SeatResponse *response
	);

	/**
	 * No discounts by ticket class, always 0
	 */
	uint64_t discountByTicketClassNumber
	(
		uint64_t sum,
		uint32_t ticketClassNumber
	);

	/**
	 * Hardcoded discounts by seat class.
	 * 0- adult, no discount
	 * 1- child, 50% off
	 * 2- employee, 100% off
	 */
	uint64_t discountByClassNumber
	(
		uint64_t sum,
		uint32_t ticketClassNumber
	);

	/***
	 * Calc ticket price.
	 * Return ticket price in copecks.
	 * Set route_point_misplaced to true (if not NULL) if start and stop are misplaced
	 */

	uint64_t calcTicket
	(
		const Trip *trip,
		const std::string &busStopFrom,
		const std::string &busStopTo,
		bool *route_point_misplaced
	);

	/**
	 * calc ticket price
	 * Return ticket price in copecks
	 */
	uint64_t calcTicket
	(
		const Trip *trip,
		const std::string &busStopFrom,
		const std::string &busStopTo,
		const SeatResponse *seats,
		uint32_t ticketClassNumber,
		bool *route_point_misplaced
	);

	/**
	 * Add extra fees to each ticket
	 * Return sum of all fees in copecks.
	 * Ticket must contain trip, class
	 */
	uint64_t calcExtraFees
	(
			Ticket *ticket
	);

	/**
	 * до 999 рублей - сервисный сбор (7,5%)
	 * от 1000 до 2000 - сервисный сбор (6,5%)
	 * свыше 2000 до 20000 - сервисный сбор 6%
	 */
	uint64_t calcExtraFeesByRouteTag
	(
			uint64_t route_tag,
			uint64_t price
	);

	/**
	 * Booking ticket
	 * Return ticket price in copecks
	 */
	uint64_t bookingTicket
	(
		const Trip *trip,
		const BookingRequest *request,
		const SeatResponse *seats,
		Ticket *ticket,
		bool *route_point_misplaced
	);

	/**
	 * Find route by bus stops
	 */
	void findRoutesByStop
	(
		std::vector<Route> &routes,
		const uint64_t route_tag_id,
		const std::string &route_point_start,
		const std::string &route_point_finish,
		bool dir		///< MUST be true
	);

	void findRouteIdsByStop
	(
		std::vector<uint64_t> &routeIds,
		const uint64_t route_tag_id,
		const std::string &route_point_start,
		const std::string &route_point_finish,
		bool dir		///< MUST be true
	);

	/**
	 * @brief Delete expired ticket
	 * @param db
	 * @param route_tag_id
	 * @param expiration_seconds
	 * @return
	 */
	static size_t doRmExpiredTickets
	(
		odb::database *db,
		uint64_t route_tag_id,
		uint32_t expiration_seconds
	);

	/**
	 * @brief Close sales (mark stage = 5 - online sale off, sale in bus)
	 * @param db
	 * @param route_tag_id
	 * @param before_trip_seconds
	 * @return
	 */
	static size_t doFinishSales
	(
		odb::database *db,
		uint64_t route_tag_id,
		uint32_t before_trip_seconds
	);

	/**
	 * @brief Open sales (mark stage = 4 - online sale on, sale in bus)
	 * @param db
	 * @param route_tag_id
	 * @param before_trip_seconds
	 * @return
	 */
	static size_t doStartSales
	(
		odb::database *db,
		uint64_t route_tag_id,
		uint32_t before_trip_seconds
	);

	/// get User by CN or identifier
	Status getUser
	(
		ServerContext *context,                 ///< server context
		const User *request,                    ///< set identifier or cn string
		User* response                          ///< return found user. Check Status first.
	) override;

	/// Add a new user. Issue a new certificate. You need use a "common" certificate to obtain personal certificate.
	Status addUser
	(
		ServerContext *context,                 ///< server context
		const User *request,                    ///< User. Common name must be set, Phone is optional
		User* response                                  ///< A new created User with certificate
	) override;

	/// Remove account from the service
	Status rmUser
	(
		ServerContext *context,                 ///< server context
		const User *request,                    ///< User. You need set identifier to avoid occasional remove.
		OperationResponse *response             ///< result
	) override;

	/// Update account
	Status setUser
	(
		ServerContext *context,                 ///< server context
		const User *request,                    ///< User. If phones, medias not provided, not changed.
		OperationResponse *response             ///< result
	) override;

	// RPC methods implementation

	/// update client certificate request 
	Status updateCertificate
	(
		ServerContext *context,                 ///< server context
		const EmptyRequest *request,            ///< nothing
		UserCertificate *response               ///< status 
	) override;

	/// Planned trip list
	Status lsTrip
	(
		ServerContext *context,                 ///< server context
		const TripListRequest *request,         ///< 
		TripListResponse *response              ///< return found trips. 
	) override;

    // List bus stops
    Status lsRoutePoint
	(
		ServerContext *context,                 ///< server context
		const RoutePointRequest *request,
		RoutePointList *response
	) override;

    // Find trip from to
    Status findTrip
	(
		ServerContext *context,                 ///< server context
		const TripRequest *request,
		TripList *response
    ) override;

    Status lsAvailableSeats
	(
		ServerContext *context,                 ///< server context
		const Trip *request,
		SeatResponse *response
	) override;

	Status lsOccupiedSeats
	(
		ServerContext *context,                 ///< server context
		const Trip *trip,
		SeatResponse *response
	) override;

    Status bookingTicket
	(
		ServerContext *context,		            ///< server context
		const BookingRequest *request,
		BookingResponse *response
	) override;

    // only booked ticket can be cancelled
    Status returnTicket
	(
		ServerContext *context,		            ///< server context
		const Ticket *request,
		OperationResponse *response
	) override;

    // you can get only booked ticket
    Status payTicket
	(
		ServerContext *context,		            ///< server context
		const PaymentRequest *request,
		PaymentResponse *response
	) override;

    // you can cancel paid ticket only
    Status cancelPayment
	(
		ServerContext *context,		            ///< server context
		const CancelPaymentRequest *request,
		OperationResponse *response
	) override;

    // get list of route tags
    Status lsRouteTag
	(
		ServerContext *context,		            ///< server context
		const EmptyRequest *request,
		RouteTagList *response
	) override;

    // ------------------ backoffice ------------------

	Status addRouteTag
	(
		ServerContext *context,		            ///< server context,
		const RouteTag *request,
		RouteTag *response
	) override;

	Status setRouteTag
	(
		ServerContext *context,		            ///< server context
		const RouteTag *request,
		OperationResponse *response
	) override;

	Status rmRouteTag
	(
		ServerContext *context,		            ///< server context
		const RouteTag *request,
		OperationResponse *response
	) override;

    // Org
    Status findOrg
	(
    	ServerContext *context,		            ///< server context
		const OrgRequest *request,
		OrgList *response
	) override;

	Status addOrg
	(
		ServerContext *context,		            ///< server context
		const Org *request,
		Org *response
	) override;

	Status setOrg
	(
		ServerContext *context,		            ///< server context
		const Org *request,
		OperationResponse *response
	) override;

	Status rmOrg
	(
		ServerContext *context,		            ///< server context
		const Org *request,
		OperationResponse *response
	) override;

    // Employee
    Status findEmployee
	(
    	ServerContext *context,		            ///< server context
		const EmployeeRequest *request,
		EmployeeList *response
	) override;

	Status addEmployee
	(
		ServerContext *context,		            ///< server context
		const Employee *request,
		Employee *response
		) override;

	Status setEmployee
	(
		ServerContext *context,		            ///< server context
		const Employee  *request,
		OperationResponse *response
	) override;

	Status rmEmployee
	(
		ServerContext *context,		            ///< server context
		const Employee  *request,
		OperationResponse *response
	) override;

    // Vehicle model
    Status findVehicleModel
	(
    	ServerContext *context,		            ///< server context
		const VehicleModelRequest  *request,
		VehicleModelList *response
	) override;

	Status addVehicleModel
	(
		ServerContext *context,		            ///< server context
		const VehicleModel  *request,
		VehicleModel *response
	) override;

	Status setVehicleModel
	(
		ServerContext *context,		            ///< server context
		const VehicleModel  *request,
		OperationResponse *response
	) override;

	Status rmVehicleModel
	(
		ServerContext *context,		            ///< server context
		const VehicleModel  *request,
		OperationResponse *response
	) override;

    // Vehicle
    Status findVehicle
	(
    	ServerContext *context,		            ///< server context
		const VehicleRequest  *request,
		VehicleList *response
	) override;

	Status addVehicle
	(
		ServerContext *context,		            ///< server context
		const Vehicle  *request,
		Vehicle *response
	) override;

	Status setVehicle
	(
		ServerContext *context,		            ///< server context
		const Vehicle  *request,
		OperationResponse *response
	) override;

	Status rmVehicle
	(
		ServerContext *context,		            ///< server context
		const Vehicle  *request,
		OperationResponse *response
	) override;

	// Rate
    Status findRate
	(
    	ServerContext *context,		            ///< server context
		const RateRequest  *request,
		RateList *response
	) override;

	Status addRate
	(
		ServerContext *context,		            ///< server context
		const Rate  *request,
		Rate *response
	) override;

	Status setRate
	(
		ServerContext *context,		            ///< server context
		const Rate  *request,
		OperationResponse *response
	) override;

	Status rmRate
	(
		ServerContext *context,		            ///< server context
		const Rate  *request,
		OperationResponse *response
	) override;

	// Route
    Status findRoute
	(
    	ServerContext *context,		            ///< server context
		const RouteRequest  *request,
		RouteList *response
	) override;

	Status addRoute
	(
		ServerContext *context,		            ///< server context
		const Route  *request,
		Route *response
	) override;

	Status setRoute
	(
		ServerContext *context,		            ///< server context
		const Route  *request,
		OperationResponse *response
	) override;

	Status rmRoute
	(
		ServerContext *context,		            ///< server context
		const Route  *request,
		OperationResponse *response
	) override;

	// Trip
	Status addTrip
	(
		ServerContext *context,		            ///< server context
		const Trip  *request,
		Trip *response
	) override;

	Status setTrip
	(
		ServerContext *context,		            ///< server context
		const Trip  *request,
		OperationResponse *response
	) override;

	Status rmTrip
	(
		ServerContext *context,		            ///< server context
		const Trip  *request,
		OperationResponse *response
	) override;

	// Ticket
    Status findTicket
	(
    	ServerContext *context,		            ///< server context
		const TicketRequest  *request,
		TicketList *response
	) override;

	Status addTicket
	(
		ServerContext *context,		            ///< server context
		const Ticket  *request,
		Ticket *response
	) override;

	Status setTicket
	(
		ServerContext *context,		            ///< server context
		const Ticket  *request,
		OperationResponse *response
	) override;

	Status rmTicket
	(
		ServerContext *context,		            ///< server context
		const Ticket  *request,
		OperationResponse *response
	) override;

	Status rmExpiredTickets
	(
    	ServerContext *context,		            ///< server context
		const ExpiredTicketRequest *request,
		OperationResponse *response
	) override;

	// Passenger
    Status findPassenger
	(
    	ServerContext *context,		            ///< server context
		const PassengerRequest  *request,
		PassengerList *response
	) override;

	Status addPassenger
	(
		ServerContext *context,		            ///< server context
		const Passenger  *request,
		Passenger *response
	) override;

	Status setPassenger
	(
		ServerContext *context,		            ///< server context
		const Passenger  *request,
		OperationResponse *response
	) override;

	Status rmPassenger
	(
		ServerContext *context,		            ///< server context
		const Passenger  *request,
		OperationResponse *response
	) override;

	Status setTripSeats
	(
		ServerContext *context,		            ///< server context
		const SetTripSeatsRequest *request,
		OperationResponse *response
	) override;

	Status setTripsSeats
	(
		ServerContext *context,		            ///< server context
		const SetTripsSeatsRequest *request,
		SetTripsSeatsResponse *responses
	) override;

	// GTFS helper function

	/**
	* @brief return first route's trip at specified time
	* @param route_id route identifier
	* @param t time unix epoch
	*/
	uint64_t getTripByRouteNTime1
	(
		uint64_t route_id,
		uint64_t t
	);

	uint32_t doAddGTFSTrip
	(
		uint64_t route_tag,
		int timezone_offset,
		const GTFSTrip &trip
	);

	// GTFS helper function
	uint32_t doAddGTFSTripDay
	(
		const GTFSTrip &trip,
		time_t date_midnight
	);

	// GTFS batch
	Status addGTFSTrips
	(
		ServerContext *context,		            ///< server context
		const GTFSTrips *request,
		AddGTFSTripsResponse *responses
	) override;
};

#endif
