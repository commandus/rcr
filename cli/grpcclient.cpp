/*
 * grpcclient.cpp
 */

#include <atomic>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>
#include <stack>

#include "utilstring.h"
#include "grpcclient.h"
#include "TicketCredentials.h"

#include "pbjson.hpp"

#include <openssl/pem.h>
#include "sslhelper.h"
#include "get_rss.h"

using grpc::Channel;
using grpc::ChannelCredentials;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::InsecureChannelCredentials;
using grpc::SslCredentials;
using grpc::SslCredentialsOptions;
using grpc::CallCredentials;
using grpc::CompositeChannelCredentials;
using grpc::MetadataCredentialsPlugin;

#define DEBUG_MEMORY_USAGE() \
		// std::cout << getCurrentRSS() / 1024 << " KB" << std::endl;

OneWayTicketClient::OneWayTicketClient(
		const std::string &intface,
		int port,
		const std::string &username,
		const std::string &password,
		bool sslOn,
		const std::string &keypem,
		const std::string &certificatepem,
		const std::string &rootCAlistpem,
		int arepeats)
{
	// target host name and port
	std::stringstream ss;
	ss << intface << ":" << port;
	std::string target(ss.str());

	repeats = arepeats;

	std::shared_ptr<Channel> channel;
	if (sslOn)
	{
		SslCredentialsOptions sslOpts;
		if (keypem.length())
			sslOpts.pem_private_key = keypem;
		if (certificatepem.length()) {
			sslOpts.pem_cert_chain = certificatepem;
			cn = getCertificateCNAsInt(certificatepem);
		}
		// Server use self-signed certificate, so client must trust CA, issued server certificate
		if (rootCAlistpem.length())
			sslOpts.pem_root_certs = rootCAlistpem;
		std::shared_ptr<ChannelCredentials> channelCredentials = SslCredentials(sslOpts);
		std::shared_ptr<CallCredentials> callCredentials = MetadataCredentialsFromPlugin(std::unique_ptr<MetadataCredentialsPlugin>(
			new TicketMetadataCredentialsPlugin(username, password)));
		std::shared_ptr<ChannelCredentials> compositeChannelCredentials = CompositeChannelCredentials(channelCredentials, callCredentials);
		channel = grpc::CreateChannel(target, compositeChannelCredentials);
	}
	else
	{
		channel = grpc::CreateChannel(target, InsecureChannelCredentials());
	}
	mStub = OneWayTicket::NewStub(channel);
}

OneWayTicketClient::~OneWayTicketClient()
{
}

bool OneWayTicketClient::getUser(const std::string &cn, User* reply)
{
	User u;
	u.set_cn(cn);
	Status status;
	for (int i = 0; i < repeats; i++)
	{
		ClientContext context;
		status = mStub->getUser(&context, u, reply);
	}

	if (!(status.ok() && (reply->id() != 0)))
	{
		std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
		return false;
	}
	else
		return true;
}

uint64_t OneWayTicketClient::addUser(
		const std::string &cn,
		const std::string &phone,
		const std::string &gcmid,
		User &reply,
		Status &status
)
{
	User u;
	u.set_cn(cn);

	for (int i = 0; i < repeats; i++)
	{
		ClientContext context;
		status = mStub->addUser(&context, u, &reply);
	}

	if (status.ok() && (reply.id() != 0))
	{
		// write to file private key and certificate
		std::string ids(toString(reply.cert().id()));
		// client.key, client.crt (last added user as default)
		string2file(DEF_FNPEMKEY, reply.cert().pkey());
		string2file(DEF_FNPEMCERTIFICATE, reply.cert().cert());
		// same, but with certificate identifier
		string2file(ids + ".crt", reply.cert().cert());
		string2file(ids + ".key", reply.cert().pkey());
	}
	else
	{
		std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
		return -1;
	}
	return reply.cert().id();
}

bool OneWayTicketClient::rmUser()
{
	User v;
	OperationResponse reply;
	Status status;
	for (int i = 0; i < repeats; i++)
	{
		ClientContext context;
		status = mStub->rmUser(&context, v, &reply);
	}

	if (!(status.ok() && (reply.code() == 0)))
	{
		std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
		return false;
	}
	else
		return true;
}

bool OneWayTicketClient::setUser(const std::string &cn, const std::string &phone, const std::string &gcmid)
{
	User u;
	OperationResponse reply;
	u.set_cn(cn);
	Status status;
	for (int i = 0; i < repeats; i++)
	{
		ClientContext context;
		status = mStub->setUser(&context, u, &reply);
	}
	if (!(status.ok() && (reply.code() == 0)))
	{
		std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
		return false;
	}
	else
		return true;
}

// Commands

#define FIND_OBJECT_JSON(OBJ) \
{ \
	OBJ ## Request request; \
	c = pbjson::json2pb(parameter, &request, err); \
	if (c != 0) \
		break; \
	OBJ ## List reply; \
	Status status; \
	for (int i = 0; i < repeats; i++) { \
		ClientContext context; \
		status = mStub->find ## OBJ(&context, request, &reply); \
		DEBUG_MEMORY_USAGE(); \
	} \
	if (status.ok()) \
		pbjson::pb2json(&reply, r); \
	else \
		std::cerr << "Error " << status.error_code() << ": " << status.error_message() << std::endl; \
}

std::string OneWayTicketClient::find_json (
	enum Obj obj,
	const std::string &parameter
)
{
	std::string r, err;
	int c = -1;
	Status status;

	switch (obj) {
		case OBJ_ROUTETAG:
			{
				EmptyRequest request;
				RouteTagList reply;
				for (int i = 0; i < repeats; i++)
				{
					ClientContext context;
					status = mStub->lsRouteTag(&context, request, &reply);
					DEBUG_MEMORY_USAGE();
				}
				if (status.ok())
					pbjson::pb2json(&reply, r);
				else
					std::cerr << "Error " << status.error_code() << ": " << status.error_message() << std::endl;
			}
			break;
		case OBJ_ORG:
			FIND_OBJECT_JSON(Org)
			break;
		case OBJ_EMPLOYEE:
			FIND_OBJECT_JSON(Employee)
			break;
		case OBJ_VEHICLEMODEL:
			FIND_OBJECT_JSON(VehicleModel)
			break;
		case OBJ_VEHICLE:
			FIND_OBJECT_JSON(Vehicle)
			break;
		case OBJ_RATE:
			FIND_OBJECT_JSON(Rate)
			break;
		case OBJ_ROUTE:
			FIND_OBJECT_JSON(Route)
			break;
		case OBJ_TRIP:
			FIND_OBJECT_JSON(Trip)
			break;
		case OBJ_TICKET:
			FIND_OBJECT_JSON(Ticket)
			break;
		case OBJ_PASSENGER:
			FIND_OBJECT_JSON(Passenger)
			break;
		default:
			break;
	}

	if (!status.ok())
	{
		r = "{\"code\": " + toString(status.error_code()) + ", \"error\": \"" + status.error_message() + "\"}";
	}

	if (c != 0)
	{
		r = "{\"code\": " + toString(c) + ", \"error\": \"" + err + "\"}";
	}

	return r;
}

#define RM_OBJECT(OBJ) \
{ \
	OBJ request; \
	c = pbjson::json2pb(parameter, &request, err); \
	if (c != 0) \
		break; \
	OperationResponse reply; \
	Status status; \
	for (int i = 0; i < repeats; i++) { \
		ClientContext context; \
		status = mStub->rm ## OBJ(&context, request, &reply); \
		DEBUG_MEMORY_USAGE(); \
	} \
	if (status.ok()) \
		pbjson::pb2json(&reply, r); \
	else \
		std::cerr << "Error " << status.error_code() << ": " << status.error_message() << std::endl; \
}

std::string OneWayTicketClient::rm (
	enum Obj obj,
	const std::string &parameter
)
{
	std::string r, err;
	int c = -1;
	Status status;

	switch (obj) {
		case OBJ_USER:
			RM_OBJECT(User)
			break;
		case OBJ_ROUTETAG:
			RM_OBJECT(RouteTag)
			break;
		case OBJ_ORG:
			RM_OBJECT(Org)
			break;
		case OBJ_EMPLOYEE:
			RM_OBJECT(Employee)
			break;
		case OBJ_VEHICLEMODEL:
			RM_OBJECT(VehicleModel)
			break;
		case OBJ_VEHICLE:
			RM_OBJECT(Vehicle)
			break;
		case OBJ_RATE:
			RM_OBJECT(Rate)
			break;
		case OBJ_ROUTE:
			RM_OBJECT(Route)
			break;
		case OBJ_TRIP:
			RM_OBJECT(Trip)
			break;
		case OBJ_TICKET:
			RM_OBJECT(Ticket)
			break;
		case OBJ_PASSENGER:
			RM_OBJECT(Passenger)
			break;
		default:
			break;
	}

	if (!status.ok())
	{
		r = "{\"code\": " + toString(status.error_code()) + ", \"error\": \"" + status.error_message() + "\"}";
	}

	if (c != 0)
	{
		r = "{\"code\": " + toString(c) + ", \"error\": \"" + err + "\"}";
	}

	return r;
}

#define ADD_OBJECT(OBJ) \
{ \
	OBJ request; \
	c = pbjson::json2pb(parameter, &request, err); \
	if (c != 0) \
		break; \
	OBJ reply; \
	Status status; \
	for (int i = 0; i < repeats; i++) { \
		ClientContext context; \
		status = mStub->add ## OBJ(&context, request, &reply); \
		DEBUG_MEMORY_USAGE(); \
	} \
	if (status.ok()) \
		pbjson::pb2json(&reply, r); \
	else \
		std::cerr << "Error " << status.error_code() << ": " << status.error_message() << std::endl; \
}

std::string OneWayTicketClient::add (
	enum Obj obj,
	const std::string &parameter
)
{
	std::string r, err;
	int c = -1;
	Status status;

	switch (obj) {
		case OBJ_USER:
			ADD_OBJECT(User)
			break;
		case OBJ_ROUTETAG:
			ADD_OBJECT(RouteTag)
			break;
		case OBJ_ORG:
			ADD_OBJECT(Org)
			break;
		case OBJ_EMPLOYEE:
			ADD_OBJECT(Employee)
			break;
		case OBJ_VEHICLEMODEL:
			ADD_OBJECT(VehicleModel)
			break;
		case OBJ_VEHICLE:
			ADD_OBJECT(Vehicle)
			break;
		case OBJ_RATE:
			ADD_OBJECT(Rate)
			break;
		case OBJ_ROUTE:
			ADD_OBJECT(Route)
			break;
		case OBJ_TRIP:
			ADD_OBJECT(Trip)
			break;
		case OBJ_TICKET:
			ADD_OBJECT(Ticket)
			break;
		case OBJ_PASSENGER:
			ADD_OBJECT(Passenger)
			break;
		default:
			break;
	}

	if (!status.ok())
	{
		r = "{\"code\": " + toString(status.error_code()) + ", \"error\": \"" + status.error_message() + "\"}";
	}

	if (c != 0)
	{
		r = "{\"code\": " + toString(c) + ", \"error\": \"" + err + "\"}";
	}

	return r;
}

#define SET_OBJECT(OBJ) \
{ \
	OBJ request; \
	c = pbjson::json2pb(parameter, &request, err); \
	if (c != 0) \
		break; \
	OperationResponse reply; \
	Status status; \
    for (int i = 0; i < repeats; i++) { \
    	ClientContext context; \
		status = mStub->set ## OBJ(&context, request, &reply); \
		DEBUG_MEMORY_USAGE(); \
    } \
	if (status.ok()) \
		pbjson::pb2json(&reply, r); \
	else \
		std::cerr << "Error " << status.error_code() << ": " << status.error_message() << std::endl; \
}

std::string OneWayTicketClient::set (
	enum Obj obj,
	const std::string &parameter
)
{
	std::string r, err;
	int c = -1;
	Status status;

	switch (obj) {
		case OBJ_USER:
			SET_OBJECT(User)
			break;
		case OBJ_ROUTETAG:
			SET_OBJECT(RouteTag)
			break;
		case OBJ_ORG:
			SET_OBJECT(Org)
			break;
		case OBJ_EMPLOYEE:
			SET_OBJECT(Employee)
			break;
		case OBJ_VEHICLEMODEL:
			SET_OBJECT(VehicleModel)
			break;
		case OBJ_VEHICLE:
			SET_OBJECT(Vehicle)
			break;
		case OBJ_RATE:
			SET_OBJECT(Rate)
			break;
		case OBJ_ROUTE:
			SET_OBJECT(Route)
			break;
		case OBJ_TRIP:
			SET_OBJECT(Trip)
			break;
		case OBJ_TICKET:
			SET_OBJECT(Ticket)
			break;
		case OBJ_PASSENGER:
			SET_OBJECT(Passenger)
			break;
		default:
			break;
	}

	if (!status.ok())
	{
		r = "{\"code\": " + toString(status.error_code()) + ", \"error\": \"" + status.error_message() + "\"}";
	}

	if (c != 0)
	{
		r = "{\"code\": " + toString(c) + ", \"error\": \"" + err + "\"}";
	}

	return r;
}

/// Route tag

size_t OneWayTicketClient::lsRouteTag(
	RouteTagList *response
)
{
	EmptyRequest request;

	size_t sz = 0;
	for (int i = 0; i < repeats; i++)
	{
		ClientContext context;
		Status status = mStub->lsRouteTag(&context, request, response);
		if (!status.ok())
			std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
		else
			sz = response->value_size();
		DEBUG_MEMORY_USAGE();
	}
	return sz;
}

/// Route list
size_t OneWayTicketClient::lsRoute
(
	RouteList *response
)
{
	RouteRequest request;

	size_t sz = 0;
	for (int i = 0; i < repeats; i++)
	{
		ClientContext context;
		Status status = mStub->findRoute(&context, request, response);
		if (!status.ok())
			std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
		else
			sz = response->value_size();
		DEBUG_MEMORY_USAGE();
	}
	return sz;
}



/// Route stop

size_t OneWayTicketClient::lsRoutePoint
(
	int route_tag,
	RoutePointList *response
)
{
	RoutePointRequest request;
	request.mutable_route_tag()->set_id(route_tag);
	request.set_sell_flags(3);

	size_t sz = 0;
	for (int i = 0; i < repeats; i++)
	{
		ClientContext context;
		Status status = mStub->lsRoutePoint(&context, request, response);
		if (!status.ok())
			std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
		else
			sz = response->route_points().size();
		DEBUG_MEMORY_USAGE();
	}
	return sz;
}

/// Trip

size_t OneWayTicketClient::lsTrip(
	TripListResponse *response,
	uint64_t route_id,
	uint32_t time_start,
	uint32_t time_finish
)
{
	size_t sz = 0;
	TripListRequest request;
	request.set_time_start(time_start);
	request.set_time_finish(time_finish);
	for (int i = 0; i < repeats; i++)
	{
		ClientContext context;
		if (route_id)
			request.mutable_route()->set_id(route_id);
		Status status = mStub->lsTrip(&context, request, response);

		if (!status.ok())
			std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
		else
			sz = response->value_size();
		DEBUG_MEMORY_USAGE();
	}
	return sz;
}

size_t OneWayTicketClient::lsAvailableSeats(
		SeatResponse *response,
		uint64_t trip_id
)
{
	Trip request;
	request.set_id(trip_id);

	size_t sz = 0;
	for (int i = 0; i < repeats; i++)
	{
		ClientContext context;
		Status status = mStub->lsAvailableSeats(&context, request, response);
		if (!status.ok())
			std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
		else
			sz = response->value_size();
		DEBUG_MEMORY_USAGE();
	}
	return sz;
}

size_t OneWayTicketClient::lsOccupiedSeats(
		SeatResponse *response,
		uint64_t trip_id
)
{
	Trip request;
	request.set_id(trip_id);

	size_t sz = 0;
	for (int i = 0; i < repeats; i++)
	{
		ClientContext context;
		Status status = mStub->lsOccupiedSeats(&context, request, response);
		if (!status.ok())
			std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
		else
			sz = response->value_size();
		DEBUG_MEMORY_USAGE();
	}
	return sz;
}

uint64_t OneWayTicketClient::bookingTicket(
		BookingResponse *response,
		uint64_t trip_id,
		const std::vector<std::string> &points,
		const std::vector<std::string> &seats,
		uint64_t discount_id
)
{
	BookingRequest request;
	request.set_trip_id(trip_id);
	for (int i = 0; i < points.size(); i++)
	{
		*request.mutable_route_point_names()->Add() = points[i];

	}
	for (int i = 0; i < seats.size(); i++)
	{
		*request.mutable_seat_names()->Add() = seats[i];
	}

	if (discount_id)
	{
		request.mutable_discounts()->Add()->set_id(discount_id);
	}

	uint64_t total = 0;
	for (int i = 0; i < repeats; i++)
	{
		ClientContext context;
		Status status = mStub->bookingTicket(&context, request, response);
		if (!status.ok())
			std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
		else
			total = response->total();
		DEBUG_MEMORY_USAGE();
	}

	return total;
}

/**
 * Cancel payment
 * @param response
 * @param ticket_id
 * @return
 */
bool OneWayTicketClient::cancelPayment(
		OperationResponse *response,
		uint64_t ticket_id
)
{
	CancelPaymentRequest request;
	request.set_id(ticket_id);
	bool r = false;
	for (int i = 0; i < repeats; i++)
	{
		ClientContext context;
		Status status = mStub->cancelPayment(&context, request, response);
		r = status.ok();
		if (!r)
			std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
		DEBUG_MEMORY_USAGE();
	}

	return r;
}

bool OneWayTicketClient::payTicket
(
		PaymentResponse *response,
		uint64_t ticket_id,
		const std::string &parameter
)
{
	PaymentRequest request;
	std::string err;
	int c = pbjson::json2pb(parameter, request.mutable_payment(), err);
	if (c != 0)
		return 0;
	request.set_ticket_id(ticket_id);
	bool r = false;
	for (int i = 0; i < repeats; i++)
	{
		ClientContext context;
		Status status = mStub->payTicket(&context, request, response);
		r = status.ok();
		if (!r)
			std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
		DEBUG_MEMORY_USAGE();
	}

	return r;
}



uint64_t OneWayTicketClient::rmExpiredTickets(
		OperationResponse *response,
		uint64_t route_tag_id
)
{
	ExpiredTicketRequest request;
	request.mutable_route_tag()->set_id(route_tag_id);

	uint64_t total = 0;
	for (int i = 0; i < repeats; i++)
	{
		ClientContext context;
		Status status = mStub->rmExpiredTickets(&context, request, response);
		if (!status.ok())
			std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
		else
			total = response->id();
		DEBUG_MEMORY_USAGE();
	}

	return total;
}

/**
 * Set or create a new trip
 * @param response
 * @param ids
 *  		route id
 * 	        start
 * 			vehicle
 * 			rate
 * 			driver
 * 			gate
 * @param seat_names
 * @return trip identifier
 */
uint64_t OneWayTicketClient::setTripSeats
(
		OperationResponse *response,
		const std::vector<uint64_t> &ids,
		const std::vector<std::string> &seat_names
)
{
	SetTripSeatsRequest request;
	request.set_route_id(ids[0]);
	request.set_time_departure(ids[1]);
	if (ids[2])
		request.set_vehicle_id(ids[2]);
	if (ids[3])
		request.set_rate_id(ids[3]);
	if (ids[4])
		request.set_driver_id(ids[4]);
	if (ids[5])
		request.set_departure_gate(ids[5]);
	if (ids[6])
		request.set_stage(ids[6]);
	for (int i = 0; i < seat_names.size(); i++)
	{
		request.mutable_seats()->Add()->set_name(seat_names[i]);
	}

	uint64_t id = 0;
	for (int i = 0; i < repeats; i++)
	{
		ClientContext context;
		Status status = mStub->setTripSeats(&context, request, response);
		if (!status.ok())
			std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
		else
			id = response->id();
		DEBUG_MEMORY_USAGE();
	}

	return id;
}

/**
 * Batch add trips
 * @param gtfs_trips
 * @return 0- success, -1- fatal error, >0- count of warnings(unsuccessful trips)
 */
int32_t OneWayTicketClient::addGTFSTrips
(
	const GTFSTrips &gtfs_trips
)
{
	uint32_t r = 0;
	for (int i = 0; i < repeats; i++)
	{
		ClientContext context;
		AddGTFSTripsResponse response;
		Status status = mStub->addGTFSTrips(&context, gtfs_trips, &response);
		if (!status.ok())
		{
			std::cerr << "Error: " << status.error_code() << " " << status.error_message() << std::endl;
			return -1;
		}
		for (int i = 0; i < response.codes_size(); i++)
		{
			if (response.codes(i))
				r++;
		}
		DEBUG_MEMORY_USAGE();
	}
	return r;
}
