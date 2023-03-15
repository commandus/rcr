/*
 * SvcImpl.cpp
 */
#include <atomic>
#include <cmath>
#include <iostream>
#include <sstream>
#include <thread>
#include <algorithm>

#include <google/protobuf/util/json_util.h>
#include <grpc++/server_context.h>
#include <grpc++/security/credentials.h>
#include <grpc++/security/server_credentials.h>

#include "svcImpl.h"
#include "passphrase.h"
#include "gen/rcr.pb-odb.hxx"

#include <odb/database.hxx>
#include <odb/transaction.hxx>
#ifdef ENABLE_SQLITE
#include <odb/sqlite/database.hxx>
#endif
#ifdef ENABLE_PG
#include <odb/pgsql/database.hxx>
#endif

using namespace odb::core;
using grpc::StatusCode;
using odb::query;

const int DEF_LIST_SIZE = 1000;
const std::string ERR_SVC_INVALID_ARGS = "Invalid arguments";

// default list size
#define DEF_LABEL_LIST_SIZE		100
#define MAX_LABEL_LIST_SIZE		10000
#define LOG(x) std::cerr

typedef uint64_t NOTIFICATION_TYPE;

/**
 * gRPC published methods error catch log function
 * Parameters:
 * 	signature	method name
 * 	cause		ODB, stdlib or other exception ane
 * 	what		Error description
 * 	msg			Protobuf incoming message
 */
std::string logString (
    const std::string &signature,
    const std::string &cause,
    const std::string &what,
    const ::google::protobuf::Message *msg)
{
    std::stringstream ss;
    if (!signature.empty())
        ss << signature;

    if (!cause.empty())
        ss << " cause(" << cause << ")";
    if (!what.empty())
        ss << " what(" << what << ")";

    if (msg) {
        std::string s;
        google::protobuf::util::JsonPrintOptions options;
        options.add_whitespace = true;
        options.always_print_primitive_fields = true;
        options.preserve_proto_field_names = true;
        google::protobuf::util::MessageToJsonString(*msg, &s, options);
        ss << " input message: " << s;
    }
    return ss.str();
}

#define CHECK_PERMISSION(permission) \
	UserIds uids; \
	int flags = getAuthUser(context, &uids); \
	if ((flags == 0) && (permission != 0)) \
		return Status(StatusCode::PERMISSION_DENIED, ERR_SVC_PERMISSION_DENIED);

#define BEGIN_GRPC_METHOD(signature, requestMessage, transact) \
		transaction transact; \
		try { \
			LOG(INFO) << logString(signature, "", "", requestMessage);

#define END_GRPC_METHOD(signature, requestMessage, transaction) \
		transaction.commit(); \
	} \
	catch (const odb::exception& oe) { \
		LOG(ERROR) << logString(signature, "odb::exception", oe.what(), requestMessage); \
		transaction.rollback(); \
	} \
	catch (const std::exception& se) { \
		LOG(ERROR) << logString(signature, "std::exception", se.what(), requestMessage); \
		transaction.rollback(); \
	} \
	catch (...) { \
		LOG(ERROR) << logString(signature, "unknown exception", "??", requestMessage); \
		transaction.rollback(); \
	}

#define INIT_CHECK_RANGE(list, r, sz, ofs)  \
	int sz = list.size(); \
	if (sz <= 0) \
		sz = DEF_LIST_SIZE; \
	int ofs = list.start(); \
	if (ofs < 0) \
		ofs = 0; \
	sz += ofs; \
	int r = 0;


#define CHECK_RANGE(r, sz, ofs) \
	if (r >= sz) \
		break; \
	if (r < ofs) \
	{ \
		r++; \
		continue; \
	}

template <class T>
std::unique_ptr<T> RcrImpl::load(uint64_t id)
{
	try	{
		std::unique_ptr<T> r(mDb->load<T>(id));
		return r;
	} catch (const std::exception &e) {
		LOG(ERROR) << "load object " << id << " error: " << e.what();
		// return nullptr
		std::unique_ptr<T> r;
		return r;
	}
}

// Find ticket(s) by the trip identifier
void OneWayTicketImpl::listTripTickets
(
	uint64_t tripId,
	TicketList *response
)
{
	try
	{
		odb::result<Ticket> qr(mDb->query<Ticket>(odb::query<Ticket>::trip == tripId));
		for (odb::result<Ticket>::iterator i(qr.begin()); i != qr.end(); i++)
		{
			Ticket*n = response->add_value();
			*n = *i;
		}
	}
	catch (const odb::exception &e)
	{
		LOG(ERROR) << "listTripTicket error: " << e.what();
	}
	catch (...)
	{
		LOG(ERROR) << "listTripTicket unknown error";
	}
}

// Find all seat(s) by the trip identifier. COPY!
void OneWayTicketImpl::listTripSeats
(
	const Trip *trip,
	SeatResponse *response
)
{
	if (!trip)
		return;
	try
	{

		if (!trip->has_vehicle())
		{
			LOG(ERROR) << "listTripSeats no vehicle assigned to the trip";
			return;
		}

		if (!trip->vehicle().has_model())
		{
			LOG(ERROR) << "listTripSeats no vehicle model assigned to the vehicle";
			return;
		}

		int c = trip->seats_size();
		for (int i = 0; i < c; i++ )
		{
			Seat *v = response->add_value();
			// LOG(INFO) << "Seat " << i << " of " << trip->seat_count();
			*v = trip->seats(i);
		}
	}
	catch (const odb::exception &e)
	{
		LOG(ERROR) << "listTripSeats error: " << e.what();
	}
	catch (...)
	{
		std::string s;
		pbjson::pb2json(trip, s);

		LOG(ERROR) << "listTripSeats unknown error, trip: " << s;
	}
}

/***
 * Check is seat name in the list
 */
const Seat *isSeatNameIn
(
	const std::string &seatName,
	SeatResponse *seats)
{
	for (int i = 0; i < seats->value_size(); i++)
	{
		if (seatName == seats->value(i).name())
		{
			return &seats->value(i);
		}
	}
	return NULL;
}

/***
 * Check is seat in the list
 */
const Seat *isSeatIn
(
	const Seat &seat,
	SeatResponse *seats)
{
	return isSeatNameIn(seat.name(), seats);
}

// Find available seat(s) by the trip identifier
void OneWayTicketImpl::listTripAvailableSeats
(
	const Trip *trip,
	SeatResponse *response
)
{
	SeatResponse seats;
	// all vehicle model seats
	listTripSeats(trip, &seats);
	SeatResponse occupied;
	// occupied seats
	listTripOccupiedSeats(trip->id(), &occupied);

	for (int i = 0; i < seats.value_size(); i++)
	{
		const Seat *s = isSeatIn(seats.value(i), &occupied);
		if (s == NULL)
		{
			Seat *v = response->add_value();
			*v = seats.value(i);
		}
	}
}

// Find sold ticket(s) by the trip identifier
void OneWayTicketImpl::listTripOccupiedSeats
(
	uint64_t tripId,
	SeatResponse *response
)
{
	try
	{
		odb::result<Ticket> r(mDb->query<Ticket>(odb::query<Ticket>::trip == tripId));
		for (odb::result<Ticket>::iterator it(r.begin()); it != r.end(); ++it)
		{
			for (int i = 0; i < it->seat_names().size(); i++)
			{
				Seat *v = response->add_value();
				v->set_name(it->seat_names(i));
			}
		}
	}
	catch (const odb::exception &e)
	{
		LOG(ERROR) << "listTripOccupiedSeats error: " << e.what();
	}
	catch (...)
	{
		LOG(ERROR) << "listTripOccupiedSeats unknown error";
	}
}

// check seats in the trip
bool OneWayTicketImpl::isSeatsAvailable (
	const Trip *trip,
	const BookingRequest *request,
	SeatResponse *seats
)
{
	if ((trip == NULL) || (request == NULL) || (seats == NULL) || (request->seat_names_size() == 0))
		return false;
	// Get list of available seats
	SeatResponse availableSeats;
	listTripAvailableSeats(trip, &availableSeats);

	for (int s = 0; s < request->seat_names_size(); s++)
	{
		const Seat *seat = isSeatNameIn(request->seat_names(s), &availableSeats);
		if (seat == NULL)
		{
			return false;
		}
		Seat *r = seats->add_value();
		*r = *seat;
	}
	std::cerr << "true " << std::endl;
	return true;
}

/**
 * Sums bus stop price EXCEPT first
 */
uint64_t sumBusPricesExceptFirst(
		const Route &route,
		const std::string &busStopFrom,
		const std::string &busStopTo,
		bool ascendant
)
{
	uint64_t r = 0;
	// LOG(ERROR) << "sumBusPricesExceptFirst " << busStopFrom << " - " << busStopTo;

	bool firstFound = false;
	if (ascendant)
	{
		for (int i = 0; i < route.route_points_size(); i++)
		{
			if (busStopFrom == route.route_points(i).name())
			{
				firstFound = true;
				r = 0;
				continue;
			}
			if (!firstFound)
			{
				continue;
			}
			// LOG(ERROR) << "sumBusPricesExceptFirst " << route.route_points(i).name() << " - " << route.route_points(i).price();
			r += route.route_points(i).price();
			if (busStopTo == route.route_points(i).name())
			{
				break;
			}
		}
	}
	else
	{
		for (int i = route.route_points_size(); i >= 0; i--)
		{
			if (busStopFrom == route.route_points(i).name())
			{
				firstFound = true;
				r = 0;
				continue;
			}
			if (!firstFound)
			{
				continue;
			}
			r += route.route_points(i).price();
			if (busStopTo == route.route_points(i).name())
			{
				break;
			}
		}
	}
	return r;
}

uint64_t OneWayTicketImpl::discountByClassNumber
(
		uint64_t sum,
		uint32_t classNumber
)
{
	switch (classNumber)
	{
	case 1:
		return sum / 2;
	default:
		return sum;
	}
}

uint64_t OneWayTicketImpl::discountByTicketClassNumber
(
		uint64_t sum,
		uint32_t ticketClassNumber
)
{
	return 0;
}

/***
 * Calc ticket price without fees
 * Return ticket price in copecks.
 * Set route_point_misplaced to true (if not NULL) if start and stop are misplaced
 */

uint64_t OneWayTicketImpl::calcTicket
(
		const Trip *trip,
		const std::string &busStopFrom,
		const std::string &busStopTo,
		bool *route_point_misplaced
)
{
	if (route_point_misplaced != NULL)
		*route_point_misplaced = true;

	uint64_t r1;
	if ((trip->rate().calc_rule() & 1) != 0)
		r1 = trip->rate().boarding_price();
	else
		r1 = 0;

	if ((trip->rate().calc_rule() & 2) != 0)
	{
		uint64_t r_perstop = sumBusPricesExceptFirst(
				trip->route(),
				busStopFrom,
				busStopTo, true);
		if (r_perstop == 0) {
			r_perstop = sumBusPricesExceptFirst(
				trip->route(),
				busStopFrom,
				busStopTo, false);
			if ((route_point_misplaced != NULL) && (r_perstop != 0))
				*route_point_misplaced = true;
		}

		r1 += r_perstop;
	}
	return r1;
}


/***
 * Calc ticket price
 * Return ticket price in copecks.
 * Set route_point_misplaced to true (if not NULL) if start and stop are misplaced
 */

uint64_t OneWayTicketImpl::calcTicket
(
		const Trip *trip,
		const std::string &busStopFrom,
		const std::string &busStopTo,
		const SeatResponse *seats,
		uint32_t ticketClassNumber,
		bool *route_point_misplaced
)
{
	if (route_point_misplaced != NULL)
		*route_point_misplaced = true;

	uint64_t r = 0;
	uint64_t basePrice = calcTicket(trip, busStopFrom, busStopTo, route_point_misplaced);

	for (int i = 0; i < seats->value_size(); i++)
	{
		r = r + discountByClassNumber(basePrice, seats->value(i).class_number()) - discountByTicketClassNumber(r, ticketClassNumber);
	}
}

/**
 * Reserved.
 * Return highest seat class
 * Highest is 0, lowest is 3
 */
uint32_t getHighestSeatClass
(
	const SeatResponse *seats
)
{
	int r = 10000;
	if (seats == NULL)
		return r;
	for (int i = 0; i < seats->value_size(); i++)
	{
		if (seats->value(i).class_number() < r)
		{
			r = seats->value(i).class_number();
		}
	}
	return r;
}

/**
 * до 999 рублей - сервисный сбор (7,5%)
 * от 1000 до 2000 - сервисный сбор (6,5%)
 * свыше 2000 до 20000 - сервисный сбор 6%
 */
uint64_t OneWayTicketImpl::calcExtraFeesByRouteTag
(
		uint64_t route_tag,
		uint64_t total
)
{
	uint64_t fee_1 = 0;
	switch(route_tag)
	{
	case 1:	// Avtovokzal
		{
			uint64_t permille;
			if (total > 2000 * CURRENCY_UNIT)
				permille = 60;
			else
				if (total >= 1000 * CURRENCY_UNIT)
					permille = 65;
				else
					permille = 75;
			fee_1 = (total * permille) / 1000;	// 6% - 7.5%
		}
		break;
	default:
		LOG(ERROR) << "Invalid route tag" << route_tag;
		break;
	}
	return fee_1;
}

/**
 * до 999 рублей - сервисный сбор (7,5%)
 * от 1000 до 2000 - сервисный сбор (6,5%)
 * свыше 2000 до 20000 - сервисный сбор 6%
 */
uint64_t OneWayTicketImpl::calcExtraFees
(
		Ticket *ticket
)
{
	if (ticket == NULL)
		return 0;

	uint64_t total = ticket->total();
	uint64_t fee_1 = 0;

	if (ticket->has_trip()
			&& ticket->trip().has_route()
			&& ticket->trip().route().has_route_tag())
	{
		uint64_t route_tag = ticket->trip().route().route_tag().id();
		fee_1 = calcExtraFeesByRouteTag(route_tag, total);
	}
	else
		LOG(ERROR) << "Can not calculate extra fees: no trip/route provided";

	if (fee_1 > 0)
	{
		Fee *fee = ticket->add_fees();
		fee->set_name("Сбор за информационные и прочие услуги");
		fee->set_value(fee_1);
		fee->set_rule(1);
	}
	return fee_1;
}

// Booking ticket
// Return discounted ticket price in copecks
uint64_t OneWayTicketImpl::bookingTicket
(
		const Trip *trip,
		const BookingRequest *request,
		const SeatResponse *seats,
		Ticket *ticket,
		bool *route_point_misplaced
)
{
	// trip
	*ticket->mutable_trip() = *trip;

	// Seats
	ticket->clear_seat_names();
	for (int i = 0; i < seats->value_size(); i++)
	{
		ticket->add_seat_names(seats->value(i).name());
	}

	// class
	uint32_t ticketClassNumber = 0;
	ticket->set_class_number(ticketClassNumber);

	// total cost. Also detect back line(reverse order of)
	uint64_t r = calcTicket(trip,
			request->route_point_names(0),
			request->route_point_names(request->route_point_names_size() - 1),
			seats, ticketClassNumber, route_point_misplaced);
	ticket->set_total(r);

	// VAT
	ticket->set_vat(0);

	// copy route points in order user selected. Otherwise, use route_point_misplaced. If true, reverse order.
	for (int i = 0; i < request->route_point_names_size(); i++)
		ticket->mutable_route_points()->Add()->set_name(request->route_point_names(i));

	// discounts. Request can contains 0, 1 or more discounts, only 1 selected
	// Company provides discount: ticket->org
	// get max discount
	uint64_t max_discount = 0;
	int discount_selected = -1;
	for (int d = 0; d < request->discounts_size(); d++)
	{
		if (request->discounts(d).value_percent()) {
			uint64_t current_discount = r * request->discounts(d).value() / 100;
			if (current_discount > max_discount)
			{
				max_discount = current_discount;
				discount_selected = d;
			}
		}
	}

	// if discount found, add ONE to the ticket
	if ((discount_selected >= 0) && (max_discount > 0))
	{
		// copy selected discount to the ticket
		*ticket->mutable_discount1() = request->discounts(discount_selected);
	}

	// extra fees
	uint64_t sum_fees = calcExtraFees(ticket);
	if (sum_fees <= 0)
		LOG(ERROR) << "No extra fees";

	return r + sum_fees - max_discount;
}

/**
 * Find route by bus stops
 */
void OneWayTicketImpl::findRoutesByStop(
	std::vector<Route> &routes,
	const uint64_t route_tag_id,
	const std::string &route_point_start,
	const std::string &route_point_finish,
	bool dir
)
{
	try
	{
		odb::query<Route> q;
		if (route_tag_id)
		{
			q = q && (odb::query<Route>::route_tag == route_tag_id);
		}

		odb::result<Route> qr(mDb->query<Route>(q));

		bool foundStart, foundFinish;
		for (odb::result<Route>::iterator i(qr.begin()); i != qr.end(); i++)
		{
			foundStart = false;
			int ps;
			if (dir)
			{
				for (ps = 0; ps < i->route_points_size(); ps++)
				{
					foundStart = (i->route_points(ps).name() == route_point_start);
					if (foundStart)
						break;
				}
			}
			else
			{
				for (ps = i->route_points_size() - 1; ps >= 0; ps--)
				{
					foundStart = (i->route_points(ps).name() == route_point_start);
					if (foundStart)
						break;
				}
			}
			if (!foundStart)
			{
				continue;
			}

			foundFinish = false;
			if (dir)
			{
				for (; ps < i->route_points_size(); ps++)
				{
					foundFinish = (i->route_points(ps).name() == route_point_finish);
					if (foundFinish)
						break;
				}
			}
			else
			{
				for (; ps >= 0; ps--)
				{
					foundFinish = (i->route_points(ps).name() == route_point_finish);
					if (foundFinish)
						break;
				}
			}
			if (!foundFinish )
				continue;
			routes.push_back(*i);
		}
	}
	catch (const odb::exception &e)
	{
		LOG(ERROR) << "findRoutesByStop error: " << e.what();
	}
	catch (...)
	{
		LOG(ERROR) << "findRoutesByStop unknown error";
	}
}

void OneWayTicketImpl::findRouteIdsByStop(
	std::vector<uint64_t> &routeIds,
	const uint64_t route_tag_id,
	const std::string &route_point_start,
	const std::string &route_point_finish,
	bool dir		///< MUST be true
)
{
	std::vector<Route> routes;

	findRoutesByStop(routes, route_tag_id, route_point_start, route_point_finish, dir);

	for (auto r = routes.begin(); r < routes.end(); r++)
	{
		routeIds.push_back(r->id());
	}
}


// --------------------- OneWayTicketImpl ---------------------

const grpc::string ERR_NO_GRANTS("No grants to call");
const grpc::string ERR_NOT_IMPLEMENTED("Not implemented");
const Status& OneWayTicketImpl::STATUS_NO_GRANTS = Status(StatusCode::PERMISSION_DENIED, ERR_NO_GRANTS);
const Status& OneWayTicketImpl::STATUS_NOT_IMPLEMENTED = Status(StatusCode::UNIMPLEMENTED, ERR_NOT_IMPLEMENTED);

std::string OneWayTicketImpl::getAuthTicket(ServerContext* context)
{
	const std::multimap<grpc::string_ref, grpc::string_ref>& metadata = context->client_metadata();
	auto f = metadata.find(TicketMetadataCredentialsPlugin::getMetaTicketName());
	if (f != metadata.end()) {
		const grpc::string_ref& p = f->second;
		return grpc::string(p.data(), p.size());
	}
	return "";
}

// x509_common_name
int OneWayTicketImpl::getAuthUser(ServerContext* context, UserIds *retuser)
{
	std::vector<grpc::string_ref> a = context->auth_context()->FindPropertyValues(TicketMetadataCredentialsPlugin::getMetaUserName());
	// std::vector<grpc::string_ref> a = context->auth_context()->FindPropertyValues("x509_common_name");
	if (a.size())
	{
		std::stringstream ss(a[0].data());
		if (retuser != NULL)
		{
			ss >> retuser->oid >> retuser->id >> retuser->roleflags;
			return retuser->roleflags;
		}
		else
		{
			int64_t id = 0, oid = 0, roleflags = 0;
			ss >> oid >> id >> roleflags;
			return roleflags;
		}
	}
	return 0;
}

bool OneWayTicketImpl::hasRole(ServerContext* context, int rolemask)
{
	std::vector<grpc::string_ref> a = context->auth_context()->FindPropertyValues(TicketMetadataCredentialsPlugin::getMetaUserName());
	return a.size() > 0;
}

OneWayTicketImpl::OneWayTicketImpl(struct ServiceConfig *config)
{
	mConfig = config;
	mDb = odbconnect(config);

	mSSLKey = file2string(config->fnpemkey);
	mSSLCertificate = file2string(config->fnpemcertificate);
	mSSLCAKey = file2string(config->fnpemcakey);
	mSSLCACacertificate = file2string(config->fnpemcacertificate);

	// OpenSSL buffers
	mCAPKey = getPKey(mSSLCAKey);
	mCAName = getCertificateCN(mSSLCACacertificate, -1);
}

RcrImpl::~RcrImpl()
{
	if (mDb)
		delete mDb;
}

struct ServiceConfig *RcrImpl::getConfig()
{
	return mConfig;
}

::grpc::Status RcrImpl::cardSearchEqual(
    ::grpc::ServerContext* context,
    const ::rcr::EqualSearchRequest* request,
    ::rcr::CardResponse* response
)
{
	if (nullptr == nullptr)
		return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	uint64_t uid = 0;
	BEGIN_GRPC_METHOD("cardSearchEqual", request, t)
	t.reset(mDb->begin());

	END_GRPC_METHOD("cardSearchEqual", request, t)
	return uid > 0 ? grpc::Status::OK : grpc::Status(StatusCode::NOT_FOUND, "");
}

::grpc::Status RcrImpl::chPropertyType(
    ::grpc::ServerContext* context,
    const ::rcr::ChPropertyTypeRequest* request,
    ::rcr::OperationResponse* response
)
{
    if (nullptr == nullptr)
        return grpc::Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
    uint64_t uid = 0;
    BEGIN_GRPC_METHOD("chPropertyType", request, t)
        t.reset(mDb->begin());

    END_GRPC_METHOD("chPropertyType", request, t)
    return uid > 0 ? grpc::Status::OK : grpc::Status(StatusCode::NOT_FOUND, "");
}
