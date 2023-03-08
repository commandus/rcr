/*
 * grpclient.h
 */

#ifndef GRPCCLIENT_H_
#define GRPCCLIENT_H_

#include <iostream>
#include <memory>
#include <string>
#include <grpc++/grpc++.h>

#include "onewayticket.grpc.pb.h"

#include "cliconfig.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using namespace onewayticket;

using onewayticket::Attribute;
using onewayticket::OperationResponse; 
using onewayticket::ListRequest; 
using onewayticket::SearchRequest; 
using onewayticket::UserCertificate;
using onewayticket::User;
using onewayticket::MediaFile;
using onewayticket::Seat;
using onewayticket::LuggageSeat;
using onewayticket::VehicleModel;
using onewayticket::Person;
using onewayticket::Employee;
using onewayticket::Passenger;
using onewayticket::Org;
using onewayticket::Vehicle;
using onewayticket::RoutePoint;
using onewayticket::RoutePointRequest;
using onewayticket::RouteTag;
using onewayticket::Rate;
using onewayticket::Route;
using onewayticket::Payment;
using onewayticket::Discount;
using onewayticket::Fee;
using onewayticket::LuggageTicket;
using onewayticket::Ticket;
using onewayticket::Trip;
using onewayticket::EmptyRequest; 
using onewayticket::Phone;
using onewayticket::RouteTagList;
using onewayticket::RoutePointList;
using onewayticket::TripListResponse;

// SSL
#define DEF_FNPEMKEY			"client.key"
#define DEF_FNPEMCERTIFICATE	"client.crt"
#define DEF_FNPEMCACERTIFICATE	"ca.crt"

class OneWayTicketClient
{
private:
	std::unique_ptr<OneWayTicket::Stub> mStub;
	int repeats;
public:
	uint64_t cn;	//< user common name as integer
	OneWayTicketClient(
		const std::string &intface, 
		int port, 
		const std::string &username, 
		const std::string &password,
		bool sslOn,
		const std::string &keypem, 
		const std::string &certificatepem, 
		const std::string &rootCAlistpem,
		int repeats
	);
	virtual ~OneWayTicketClient();
	// method wrappers
	bool getUser(
		const std::string &phone,
		User* reply
	);
	uint64_t addUser(
		const std::string &cn,
		const std::string &phone,
		const std::string &gcmid,
		User &reply,				///< return user if success
		Status &status				///< return status of operation
	);
	bool rmUser();
	bool setUser(
		const std::string &cn, 
		const std::string &phone,
		const std::string &gcmid
	);

	// commands
	std::string find_json (
		enum Obj obj,
		const std::string &parameter
	);

	std::string add (
		enum Obj obj,
		const std::string &parameter
	);

	std::string rm (
		enum Obj obj,
		const std::string &parameter
	);

	std::string set (
		enum Obj obj,
		const std::string &parameter
	);

	// other commands

	size_t lsRouteTag(
		RouteTagList *response
	);

	size_t lsRoute(
		RouteList *response
	);

	size_t lsRoutePoint
	(
		int route_tag,
		RoutePointList *response
	);

	size_t lsTrip(
		TripListResponse *response,
		uint64_t route_id,
		uint32_t time_start,
		uint32_t time_finish
	);

	size_t lsAvailableSeats(
			SeatResponse *response,
			uint64_t trip_id
	);

	size_t lsOccupiedSeats(
			SeatResponse *response,
			uint64_t trip_id
	);

	uint64_t bookingTicket(
			BookingResponse *response,
			uint64_t trip_id,
			const std::vector<std::string> &points,
			const std::vector<std::string> &seats,
			uint64_t discount_id
	);

	uint64_t rmExpiredTickets(
			OperationResponse *response,
			uint64_t route_tag_id
	);

	/**
	 * Set or create a new trip
	 * @param response
	 * @param ids
	 * 			route id
	 * 			vehicle
	 * 			rate
	 * 			driver
	 * 			gate
	 * @param seat_names
	 * @return trip identifier
	 */
	uint64_t setTripSeats
	(
			OperationResponse *response,
			const std::vector<uint64_t> &ids,
			const std::vector<std::string> &seat_names
	);

	/**
	 * Cancel payment
	 * @param response
	 * @param ticket_id
	 * @return
	 */
	bool cancelPayment(
			OperationResponse *response,
			uint64_t ticket_id
	);

	/**
	 * Add payment to the ticket
	 * @param response
	 * @param ticket_id
	 * @param parameter
	 * @return
	 */
	bool payTicket
	(
			PaymentResponse *response,
			uint64_t ticket_id,
			const std::string &parameter
	);

	int32_t addGTFSTrips
	(
		const GTFSTrips &gtfs_trips
	);

};

#endif /* GRPCCLIENT_H_ */
