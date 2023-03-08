/*
 * SvcImpl.cpp
 */

#include <atomic>
#include <cmath>
#include <iostream>
#include <sstream>
#include <thread>
#include <algorithm>

#include <grpc++/server_context.h>
#include <grpc++/security/credentials.h>
#include <grpc++/security/server_credentials.h>

#include "svcImpl.h"
#include "passphrase.h"

#include "gen/rcr.pb-odb.hxx"

using grpc::StatusCode;
using odb::query;

const int DEF_LIST_SIZE = 1000;


// default list size
#define DEF_LABEL_LIST_SIZE		100
#define MAX_LABEL_LIST_SIZE		10000

typedef uint64_t NOTIFICATION_TYPE;

#define CHECK_PERMISSION(permission) \
	UserIds uids; \
	int flags = getAuthUser(context, &uids); \
	if ((flags == 0) && (permission != 0)) \
		return Status(StatusCode::PERMISSION_DENIED, ERR_SVC_PERMISSION_DENIED);

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
		pbjson::pb2json(msg, s);
		ss << " input message: " << s;
	}
	return ss.str();
}

#define BEGIN_GRPC_METHOD(signature, requestMessage, transact) \
		transaction transact; \
		inc_requestcount(); \
		try { \
			LOG(INFO) << logString(signature, "", "", requestMessage);

#define END_GRPC_METHOD(signature, requestMessage, transaction) \
		transaction.commit(); \
	} \
	catch (const odb::exception& oe) { \
		inc_errorcount(); \
		LOG(ERROR) << logString(signature, "odb::exception", oe.what(), requestMessage); \
		transaction.rollback(); \
	} \
	catch (const std::exception& se) { \
		inc_errorcount(); \
		LOG(ERROR) << logString(signature, "std::exception", se.what(), requestMessage); \
		transaction.rollback(); \
	} \
	catch (...) { \
		inc_errorcount(); \
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

/*
template <class TResponse, class T> void listResponse(TResponse* response, const ListRequest* list, odb::result<T> *dataset, T*(*callback)(TResponse*))
{
	INIT_CHECK_RANGE(list, r, sz, ofs)
	for (auto i(dataset->begin()); i != dataset->end(); i++)
	{
		CHECK_RANGE(r, sz, ofs)
		T* n = callback(response);
		*n = *i;
		r++;
	}
}

template <class TResponse, class T> void findResponse(TResponse* response, const SearchRequest* search, odb::result<T> *dataset, T*(*callback)(TResponse*))
{
	const ListRequest* list = &search->listrequest();
	INIT_CHECK_RANGE(list, r, sz, ofs)
	for (auto i(dataset->begin()); i != dataset->end(); i++)
	{
		CHECK_RANGE(r, sz, ofs)
		T* n = callback(response);
		*n = *i;
		r++;
	}
}
*/

// --------------------- Helper functions ---------------------

const double PI180 = 0.017453292519943295769236907684886;	// radians
const double EARTH_RADIUS = 6372795;	// meters

// --------------------- Helper functions ---------------------

/**
  * Return distance between two geographic location (polar coordinates) in meters
  * 	latitude1
  */
double distance(
	double	latitude1,	///< first point polar coordinates, latitude, degrees
	double	longitude1,	///< first point polar coordinates, longitude, degrees
	double	latitude2,	///< second point polar coordinates, degrees, double
	double	longitude2	///< second point polar coordinates, longitude, degrees
) {
	// convert degrees to radians
	double lat1 = latitude1 * PI180;
	double lat2 = latitude2 * PI180;
	double lon1 = longitude1 * PI180;
	double lon2 = longitude2 * PI180;
	// pre-calc
	double cl1 = cos(lat1);
	double cl2 = cos(lat2);
	double sl1 = sin(lat1);
	double sl2 = sin(lat2);
	double delta = lon2 - lon1;
	double cDelta = cos(delta);
	double sDelta = sin(delta);
	double p1 = cl2 * sDelta;
	double p2 = (cl1 * sl2) - (sl1 * cl2 * cDelta);
	// calc
	return EARTH_RADIUS * atan2(sqrt(p1 * p1 + p2 * p2), sl1 * sl2 + cl1 * cl2 * cDelta);
}

/**
 * NanoMessage PUB topic. May extend later.
 *		uidTo	user to
 *
 */
struct NNNotificationTopic
{
	uint64_t uidTo;
};

#define CMD_MASK			15
// #define NOTIFY_FLAG_EMPTY	0
#define NOTIFY_FLAG_EVENT	1
#define NOTIFY_FLAG_ACTION	2
#define NOTIFY_FLAG_CHAT	3
#define NOTIFY_FLAG_BBS		4
#define NOTIFY_FLAG_BBS_RM	5

/**
 * NNNotificationMessage header is NNNotificationTopic
 */
struct NNNotificationMessage
{
	NNNotificationTopic topic;
	uint64_t uidFrom;
	uint64_t value;		///< event, chat, bbs id
	uint64_t param;		///< action id
	uint8_t flags;		///< Bits 0-2: 1- event, 2-action, 3- chat, 4- bbs, 5- bbs remove. See NOTIFY_FLAG_XXX
};

/**
 * Send notification to the user by id
 */
bool notifyUser(NanoPub *pub, struct NNNotificationMessage *notification)
{
	LOG(INFO) << "notifyUser " << notification->topic.uidTo
			<< " from " << notification->uidFrom
			<< " value:" << notification->value
			<< " param:" << notification->param
			<< " -cmd-:" << (((int) notification->flags) & CMD_MASK)
			<< " flags:" << ((int) notification->flags);
	pub->send(notification, sizeof(struct NNNotificationMessage));
	return true;
}

/**
 * Return 0 if not found
 */
uint64_t OneWayTicketImpl::findUserIdByCN (
	const std::string &cn
)
{
	try
	{
		// odb::result<User> r(mDb->query<User>("cn = '" + cn + "'"));
		odb::result<User> r(mDb->query<User>(odb::query<User>::cn == cn));
		odb::result<User>::iterator it(r.begin());
		if (it != r.end())
			return it->id();
	}
	catch (const odb::exception &e)
	{
		LOG(ERROR) << "findUserIdByCN " << cn << " error: " << e.what();
	}
	catch (...)
	{
		LOG(ERROR) << "findUserIdByCN " << cn << " unknown error";
	}
	return 0;
}

uint64_t OneWayTicketImpl::findUserByCN (
	const std::string &cn,
	User *retval)
{
	uint64_t r = findUserIdByCN(cn);
    if (r)
		mDb->load(r, *retval);
	return r;
}

/**
 * TODO optimize: use native view
 */
bool OneWayTicketImpl::existsUser(uint64_t uid)
{
	User r;
	try
	{
		mDb->load(uid, r);
		return true;
	}
	catch (const std::exception &e)
	{
		LOG(ERROR) << "existsUser() error: " << e.what();
	}
	return false;
}

template <class T>
std::unique_ptr<T> OneWayTicketImpl::load(uint64_t id)
{
	try
	{
		std::unique_ptr<T> r(mDb->load<T>(id));
		return r;
	}
	catch (const std::exception &e)
	{
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

OneWayTicketImpl::~OneWayTicketImpl()
{
	if (mDb)
		delete mDb;
	if (mCAPKey)
		EVP_PKEY_free(mCAPKey);
}

struct ServiceConfig *OneWayTicketImpl::getConfig()
{
	return mConfig;
}

uint64_t OneWayTicketImpl::newCertificate(UserCertificate *certificate)
{
	uint64_t id = mDb->persist(*certificate);
	bool ok = mkCertificate(certificate, id);
	certificate->set_id(id);
	if (ok)
		mDb->update(*certificate);
	else
		// delete on failure
		mDb->erase(*certificate);
	return id;
}

bool OneWayTicketImpl::mkCertificate(UserCertificate *certificate, uint64_t id)
{
	if (!certificate)
		return false;
	time_t period = 10 * 365 * 24 * 60 *60;

	time_t start;
	time(&start);

	std::string spkey;
	std::string scert;

	if (mkcert(id, mCAName, mCAPKey, &spkey, &scert))
	{
		certificate->set_id(id);
		certificate->set_start((int) start);
		certificate->set_finish(start + period);
		certificate->set_pkey(spkey);
		certificate->set_cert(scert);
		return true;
	}
	return false;
}

Status OneWayTicketImpl::getUser (
	ServerContext* context, 
	const User* request, 
	User* response
)
{
	if ((request == NULL) || (response == NULL))
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);

	uint64_t uid = 0;
	BEGIN_GRPC_METHOD("getUser", request, t)

	UserIds uids;

	int flags = getAuthUser(context, &uids);
	if (flags == 0)
		return Status(StatusCode::PERMISSION_DENIED, ERR_SVC_PERMISSION_DENIED);

	t.reset(mDb->begin());

	// TODO add fimd user by identifier
	uid = findUserByCN(request->cn(), response);
	UserCertificate *c = response->mutable_cert();
	c->clear_pkey();
	c->clear_cert();

	END_GRPC_METHOD("getUser", request, t)
	return uid > 0 ? Status::OK : Status(StatusCode::NOT_FOUND, "");
}

Status OneWayTicketImpl::updateCertificate (
	ServerContext* context, 
	const EmptyRequest* request, 	///< nothing 
	UserCertificate* response)
{
	if (request == NULL)
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("updateCertificate", request, t)
	UserIds uids;
	int flags = getAuthUser(context, &uids);
	if (flags == 0)
		return Status(StatusCode::PERMISSION_DENIED, ERR_SVC_PERMISSION_DENIED);

	// get a User
	t.reset(mDb->begin());

	std::unique_ptr<User> v = load<User>(uids.id);
	if (!v)
	{
		t.rollback();
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	}

	// new certificate
	if (mkCertificate(v->mutable_cert(), v->id()))
	{
		UserCertificate uc(v->cert());
		mDb->persist(uc);
	}

	END_GRPC_METHOD("updateCertificate", request, t)
	return Status::OK;
}

// Trip
Status OneWayTicketImpl::lsTrip (
		ServerContext* context,			///< server context
		const TripListRequest* request,	///< 
		TripListResponse* response		///< return found trips. 
)
{
	if (request == NULL)
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("lsTrip", request, t)
	// CHECK_PERMISSION(0)

	// get a User
	t.reset(mDb->begin());

	odb::query<Trip> q(odb::query<Trip>::show_in_list);

	// if route provided, limit
	if (request->has_route())
	{
		if (request->route().id())
		{
			q = q && (odb::query<Trip>::route == request->route().id());
		}
	}

	// if time provided, limit
	if (request->time_finish())
	{
		q = q && (odb::query<Trip>::time_departure >= request->time_start()) && (odb::query<Trip>::time_departure <= request->time_finish());
	}

	odb::result<Trip> r(mDb->query<Trip>(q));
	for (odb::result<Trip>::iterator it(r.begin()); it != r.end(); ++it)
	{
		Trip *v = response->add_value();
		*v = *it;
		// v->clear_user();
	}

	END_GRPC_METHOD("lsTrip", request, t)
	return Status::OK;
}

// List bus stops
Status OneWayTicketImpl::lsRoutePoint
(
	ServerContext *context,                 ///< server context
	const RoutePointRequest *request,
	RoutePointList *response
) 
{
	if (request == NULL)
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("lsRoutePoint", request, t)

	// CHECK_PERMISSION(0)

	t.reset(mDb->begin());
	
	uint64_t route_tag_id = 0;
	route_tag_id = request->route_tag().id();
	if (route_tag_id == 0)
	{
		if (!request->route_tag().name().empty())
		{
			// TODO find rou tag by name
			route_tag_id = 1;
		}
		else
		{
			route_tag_id = 1;
		}
	}

	uint32_t sell_flags = request->sell_flags() & 3;

	std::string qs;
	if (!request->start_route_point_name().empty())
	{
		// LOG(INFO) << "Start point at " << request->start_route_point_name();
		qs = "object_id IN (SELECT DISTINCT object_id FROM \"Route_route_points\" WHERE value_name = '" + request->start_route_point_name() + "')";
	}
	else
	{
		qs = "1=1";
	}

	// SELECT value_name FROM "Route_route_points" ORDER BY value_name
	// or 
	// odb::result<Route> r(mDb->query<Route>(odb::query<Route>::route_tag == route_tag_id));

	std::string old_bus_stop_name = "~";
	odb::result<RoutePointNameLatitudeLongitude> r(mDb->query<RoutePointNameLatitudeLongitude>(qs));
	for (odb::result<RoutePointNameLatitudeLongitude>::iterator it(r.begin()); it != r.end(); ++it)
	{
		if (!(it->sell_flags & sell_flags))
			continue;
		if (old_bus_stop_name == it->name)
			continue; // DISTINCT helper ;)

		RoutePoint *r = response->add_route_points();
		r->set_name(it->name);
		r->set_latitude(it->latitude);
		r->set_longitude(it->longitude);

		old_bus_stop_name = it->name;
	}

	END_GRPC_METHOD("lsRoutePoint", request, t)
	return Status::OK;
}

Status OneWayTicketImpl::lsAvailableSeats (
	ServerContext* context,		            ///< server context
	const Trip* request,
	SeatResponse* response                  ///< return found trips. 
)
{
	if (request == NULL)
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("lsAvailableSeats", request, t)
	// CHECK_PERMISSION(0)

	// check does trip exists and ready
	t.reset(mDb->begin());
	std::unique_ptr<Trip> v = load<Trip>(request->id());

	if (!v)
	{
		t.rollback();
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	}

	time_t tm;
	time(&tm);

	// tm < v->time_sale_start()) || (tm > v->time_sale_finish()
	if (v->stage() != TRIP_STAGE_SALE)
	{
		LOG(ERROR) << "request available seats on stage: " << v->stage() << " time " << tm <<
				" [" << v->time_sale_start() << ", " << v->time_sale_finish() << "]";
		t.rollback();
		return Status(StatusCode::DEADLINE_EXCEEDED, ERR_SVC_DEADLINE_EXCEEDED);
	}

	// list of tickets
	listTripAvailableSeats(v.get(), response);

	int avail_seats = response->value_size();
	if (avail_seats > 0) {

		// check limit seat_count for model. We can not sale more than limit.
		int allowed_seats = v->vehicle().model().seat_count();

		if (allowed_seats == 0)
		{
			LOG(WARNING) << "listTripSeats no seat limit assigned to the vehicle model";
		}

		int total_seats = v->seats_size();
		if (allowed_seats > total_seats)
		{
			LOG(ERROR) << "listTripSeats invalid seat limit " << allowed_seats << " of " << total_seats << " total";
			allowed_seats = total_seats;
		}

		if (total_seats - avail_seats >= allowed_seats) {
			LOG(INFO) << "listTripSeats trip " << request->id()
					<< " sale " << total_seats << " seats limit "
					<< allowed_seats << " occurred. Available "
					<< avail_seats << ", return nothing";
			// do not show any available seats
			response->clear_value();
		}
	}

	END_GRPC_METHOD("lsAvailableSeats", request, t)
	return Status::OK;
}

Status OneWayTicketImpl::lsOccupiedSeats (
	ServerContext* context,		            ///< server context
	const Trip* request,
	SeatResponse* response                  ///< return found trips.
)
{
	if (request == NULL)
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("lsOccupiedSeats", request, t)
	// CHECK_PERMISSION(0)

	t.reset(mDb->begin());
	// occupied seats
	listTripOccupiedSeats(request->id(), response);

	END_GRPC_METHOD("lsOccupiedSeats", request, t)
	return Status::OK;
}


Status OneWayTicketImpl::bookingTicket (
	ServerContext* context,		            ///< server context
	const BookingRequest *request,
	BookingResponse *response
)
{
	if (request == NULL)
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("bookingTicket", request, t)
	
	// CHECK_PERMISSION(0)

	t.reset(mDb->begin());
	// check does trip exists and ready
	std::unique_ptr<Trip> trip = load<Trip>(request->trip_id());
	if (!trip)
	{
		t.rollback();
		LOG(ERROR) << "Booking ticket: no trip provided";
		return Status(StatusCode::INVALID_ARGUMENT, "Booking ticket: no trip provided");
	}

	// check points request
	if (request->route_point_names_size() == 0)
	{
		t.rollback();
		LOG(ERROR) << "Booking ticket: no destination point provided";
		return Status(StatusCode::INVALID_ARGUMENT, "Booking ticket: no any seat(s) provided");
	}

	// check passenger's seat request
	if (request->seat_names_size() == 0)
	{
		t.rollback();
		LOG(ERROR) << "Booking ticket: no any seat(s) provided";
		return Status(StatusCode::INVALID_ARGUMENT, "Booking ticket: no any seat(s) provided");
	}

	// check trip time
	time_t tm;
	time(&tm);
	if (trip->stage() != TRIP_STAGE_SALE) // || (trip->time_sale_start() > tm) || (trip->time_sale_finish() < tm)
	{
		t.rollback();
		LOG(ERROR) << "Booking ticket: stage " << trip->stage() << " but " << TRIP_STAGE_SALE << " required";
		return Status(StatusCode::INVALID_ARGUMENT, "Booking ticket: stage incorrect");
	}

	/*
	 * Why?
	// check trip departure & arrival
	if (!trip->has_ticket_offer())
	{
		t.rollback();
		LOG(ERROR) << "Booking ticket: no ticket offer provided";
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	}


	if (!trip->ticket_offer().has_route_point_time())
	{
		t.rollback();
		LOG(ERROR) << "Booking ticket: no route point time provided";
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	}
	*/
	// Must at least from, to
	if (request->route_point_names_size() < 2)
	{
		t.rollback();
		LOG(ERROR) << "Booking ticket: at least 2 route points required";
		return Status(StatusCode::INVALID_ARGUMENT, "Booking ticket: at least 2 route points required");
	}

	// check available seats
	SeatResponse seats;
	if (!isSeatsAvailable(trip.get(), request, &seats))
	{
		t.rollback();
		LOG(ERROR) << "Booking ticket: no available seats anymore";
		return Status(StatusCode::INVALID_ARGUMENT, "Booking ticket: no available seats anymore");
	}

	LOG(INFO) << "Booking ticket, requested seats available";

	Ticket ticket;
	if (request->has_ticket())
	{
		// copy template
		ticket = request->ticket();
	}
	ticket.set_stage(TICKET_STAGE_RESERVED);
	ticket.set_time_reserved(tm);
	bool direction_backward;
	uint64_t copecks = bookingTicket(trip.get(), request, &seats, &ticket, &direction_backward);

	if (direction_backward)
		LOG(INFO) << "Ticket backward direction";

	// Set cashier later
	ticket.set_user_id_booking(0);
	// save first time
	uint64_t id = mDb->persist(ticket);

	ticket.set_id(id);
	std::string n = toString(id);
	ticket.set_number(n);
	ticket.set_name(n);
	ticket.set_pass_phrase(makePassPhrase(trip.get(), id));
	// save again
	mDb->update(ticket);

	LOG(INFO) << "Booking ticket " << id << " price " << copecks;

	response->mutable_response()->set_code(0);
	response->mutable_response()->set_id(id);
	response->set_ticket_id(id);
	response->set_total(copecks);
	response->set_time_booking(tm);
	*response->mutable_ticket() = ticket;

	END_GRPC_METHOD("bookingTicket", request, t)
	return Status::OK;
}

Status OneWayTicketImpl::returnTicket(
		ServerContext* context,		            ///< server context
		const Ticket *request,
		OperationResponse *response
)
{
	if (request == NULL)
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("returnTicket", request, t)
	
	// CHECK_PERMISSION(0)
	
	// check does trip exists and ready
	t.reset(mDb->begin());
	std::unique_ptr<Trip> v = load<Trip>(request->id());
	if (!v)
	{
		t.rollback();
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	}

	// check trip time
	time_t tm;
	time(&tm);
	if ((v->stage() != TRIP_STAGE_SALE) || (v->time_sale_start() > tm) || (v->time_sale_finish() < tm))
	{
		t.rollback();
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	}

	std::unique_ptr<Ticket> ticket = load<Ticket>(request->id());
	if (!ticket)
	{
		t.rollback();
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	}

	if (ticket->stage() != TICKET_STAGE_RESERVED)
	{
		t.rollback();
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	}
	
	mDb->erase(ticket.get());

	END_GRPC_METHOD("returnTicket", request, t)
	return Status::OK;
}

// you can get only booked ticket
Status  OneWayTicketImpl::payTicket (
	ServerContext* context,		            ///< server context
	const PaymentRequest *request,
	PaymentResponse *response
) 
{
	if (request == NULL)
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("payTicket", request, t)
	CHECK_PERMISSION(0)
	if (!request->has_payment())
	{
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_PAY_PAID_ALREADY);
	}
	// check does trip exists and ready
	t.reset(mDb->begin());
	std::unique_ptr<Ticket> v = load<Ticket>(request->ticket_id());
	if (!v)
	{
		t.rollback();
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_PAY_NO_TICKET);
	}

	uint64_t total = v->total() + calcExtraFeesByRouteTag(v->trip().route().route_tag().id(), v->total());

	if (total < request->payment().total())
	{
		t.rollback();
		std::stringstream ss;
		ss << " must: " << total << " receive: " << request->payment().total();
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_PAY_TOTAL_WRONG + ss.str());
	}
	v->set_stage(TICKET_STAGE_PAID);

	// Make payment
	// override time_paid
	time_t paid_time = time(NULL);
	v->set_time_paid(paid_time);

	Payment payment(request->payment());
	// override ticket id
	payment.set_ticket_id(request->ticket_id());
	payment.set_time_paid(paid_time);

	uint64_t payment_id = mDb->persist(payment);
	*v->mutable_payment1() = payment;
	mDb->update(*v.get());

	response->mutable_response()->set_code(0);
	response->mutable_response()->set_id(payment_id);
	*response->mutable_value() = payment;
	*response->mutable_ticket() = *v.get();

	inc_ticketssold();
	END_GRPC_METHOD("payTicket", request, t)
	return Status::OK;
}

// you can cancel paid ticket only
Status OneWayTicketImpl::cancelPayment
(
	ServerContext *context,		            ///< server context
	const CancelPaymentRequest *request,
	OperationResponse *response
)
{
	if (request == NULL)
			return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("cancelPayment", request, t)

	CHECK_PERMISSION(0)

	// check does trip exists and ready
	t.reset(mDb->begin());
	std::unique_ptr<Ticket> v = load<Ticket>(request->id());
	if (!v)
	{
		t.rollback();
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_PAY_NO_TICKET);
	}

	v->set_stage(TICKET_STAGE_RESERVED);
	v->set_notes(v->notes() + " " + request->notes());
	v->clear_payment1();
	mDb->update(*v.get());

	response->set_code(0);
	response->set_id(request->id());
	END_GRPC_METHOD("cancelPayment", request, t)
	return Status::OK;
}

// get list of route tags
Status OneWayTicketImpl::lsRouteTag(
	ServerContext* context,		            ///< server context
	const EmptyRequest *request,
	RouteTagList *response
)
{
	BEGIN_GRPC_METHOD("lsRouteTag", request, t)

	CHECK_PERMISSION(0)

	t.reset(mDb->begin());
	odb::result<RouteTag> r(mDb->query<RouteTag>());
	for (odb::result<RouteTag>::iterator it(r.begin()); it != r.end(); ++it)
	{
		RouteTag *v = response->add_value();
		*v = *it;
	}

	END_GRPC_METHOD("lsRouteTag", request, t)
	return Status::OK;
}

#define STRINGIFY2(a) STRINGIFY(a)
#define STRINGIFY(a) #a

// ------------------------------- Remove -------------------------------

#define RM_OBJECT(OBJECT) \
Status OneWayTicketImpl::rm ## OBJECT(ServerContext* context, const OBJECT *request, OperationResponse *response) \
{ \
	if (request == NULL) \
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS); \
	BEGIN_GRPC_METHOD(STRINGIFY(rm ## OBJECT), request, t) \
	t.reset(mDb->begin()); \
	mDb->erase(request); \
	END_GRPC_METHOD(STRINGIFY(rm ## OBJECT), request, t) \
	return Status::OK; \
}

Status OneWayTicketImpl::rmUser(ServerContext* context, const User* request, OperationResponse* response)
{
	if (request == NULL)
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("rmUser", request, t)
	UserIds uids;
	int flags = getAuthUser(context, &uids);
	if (flags == 0)
		return Status(StatusCode::PERMISSION_DENIED, ERR_SVC_PERMISSION_DENIED);
	// just make sure
	if (request->id() != uids.id)
		return Status::CANCELLED;

	t.reset(mDb->begin());

	mDb->erase(request);
	response->clear_id();
	response->set_code(0);
	END_GRPC_METHOD("rmUser", request, t)
	return Status::OK;
}

RM_OBJECT(RouteTag)

RM_OBJECT(Org)

RM_OBJECT(Employee)

RM_OBJECT(VehicleModel)

RM_OBJECT(Vehicle)

RM_OBJECT(Rate)

RM_OBJECT(Route)

RM_OBJECT(Trip)

RM_OBJECT(Ticket)

RM_OBJECT(Passenger)

// ------------------------------- Add -------------------------------

#define ADD_OBJECT(OBJECT) \
Status OneWayTicketImpl::add ## OBJECT(ServerContext* context, const OBJECT *request, OBJECT *response) \
{ \
	if (request == NULL) \
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS); \
	BEGIN_GRPC_METHOD(STRINGIFY(add ## OBJECT), request, t) \
	response->CopyFrom(*request); \
	t.reset(mDb->begin()); \
	int64_t id = mDb->persist(response); \
	response->set_id(id); \
	END_GRPC_METHOD(STRINGIFY(add ## OBJECT), request, t) \
	return Status::OK; \
}

#define ADD_OBJECT_PERSON(OBJECT) \
Status OneWayTicketImpl::add ## OBJECT(ServerContext* context, const OBJECT *request, OBJECT *response) \
{ \
	if (request == NULL) \
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS); \
	BEGIN_GRPC_METHOD(STRINGIFY(add ## OBJECT), request, t) \
	response->CopyFrom(*request); \
	t.reset(mDb->begin()); \
	if (response->has_person()) { \
		Person *p = response->mutable_person(); \
		if (!p->phone_mobile().empty()) \
			p->set_phone_mobile(E164ToString(p->phone_mobile())); \
		int64_t person_id = mDb->persist(p); \
		p->set_id(person_id); \
	} \
	int64_t id = mDb->persist(response); \
	response->set_id(id); \
	END_GRPC_METHOD(STRINGIFY(add ## OBJECT), request, t) \
	return Status::OK; \
}

Status OneWayTicketImpl::addUser(ServerContext* context, const User* request, User* response)
{
	if (request == NULL)
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("addUser", request, t)

	response->CopyFrom(*request);

	t.reset(mDb->begin());

	// new certificate
	int64_t certid = newCertificate(response->mutable_cert());

	// get identifier for certificate
	response->clear_id();
	// save
	int64_t id = mDb->persist(response);
	END_GRPC_METHOD("addUser", request, t)
	return Status::OK;
}

// ------------------------------- Add -------------------------------

ADD_OBJECT(RouteTag)

ADD_OBJECT(Org)

ADD_OBJECT(Employee)

ADD_OBJECT(VehicleModel)

ADD_OBJECT(Vehicle)

ADD_OBJECT(Rate)

ADD_OBJECT(Route)

ADD_OBJECT(Trip)

ADD_OBJECT(Ticket)

ADD_OBJECT_PERSON(Passenger)

// ------------------------------- Set -------------------------------

#define SET_OBJECT(OBJECT) \
Status OneWayTicketImpl::set ## OBJECT(ServerContext* context, const OBJECT *request, OperationResponse* response) \
{ \
	if (request == NULL) \
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS); \
	BEGIN_GRPC_METHOD(STRINGIFY(set ## OBJECT), request, t) \
	t.reset(mDb->begin()); \
	std::unique_ptr<OBJECT> v = load<OBJECT>(request->id()); \
	if (!v) \
	{ \
		t.rollback(); \
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS); \
	} \
	mDb->update(*request); \
	response->set_code(0); \
	END_GRPC_METHOD(STRINGIFY(set ## OBJECT), request, t) \
	return Status::OK; \
}

Status OneWayTicketImpl::setUser(ServerContext* context, const User* request, OperationResponse* response)
{
	if (request == NULL)
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("setUser", request, t)
	UserIds uids;
	int flags = getAuthUser(context, &uids);
	if (flags == 0)
		return Status(StatusCode::PERMISSION_DENIED, ERR_SVC_PERMISSION_DENIED);

	t.reset(mDb->begin());

	std::unique_ptr<User> v = load<User>(uids.id);
	if (!v)
	{
		t.rollback();
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	}

	if (!request->cn().empty())
		v->set_cn(request->cn());

	mDb->update(*v);
	response->set_code(0);
	END_GRPC_METHOD("setUser", request, t)
	return Status::OK;
}

SET_OBJECT(RouteTag)

SET_OBJECT(Org)

SET_OBJECT(Employee)

SET_OBJECT(VehicleModel)

SET_OBJECT(Vehicle)

SET_OBJECT(Rate)

SET_OBJECT(Route)

SET_OBJECT(Trip)

SET_OBJECT(Ticket)

// verify phone number
Status OneWayTicketImpl::setPassenger(ServerContext* context, const Passenger *request, OperationResponse* response)
{
	if (request == NULL)
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("setPassenger", request, t)
	t.reset(mDb->begin());
	std::unique_ptr<Passenger> v = load<Passenger>(request->id());
	if (!v)
	{
		t.rollback();
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	}
	Passenger r(*request);
	if (r.has_person()) {
		Person *p = r.mutable_person();
		if (!p->phone_mobile().empty())
			p->set_phone_mobile(E164ToString(p->phone_mobile()));
		/*
		 // Not sure
		if (p->id() == 0)
		{
			int64_t person_id = mDb->persist(p);
			p->set_id(person_id);
		}
		*/
	}

	mDb->update(r);
	response->set_code(0);
	END_GRPC_METHOD("setPassenger", request, t)
	return Status::OK;
}


// ------------------------------- Find -------------------------------

// TODO Enhance!
#define FIND_OBJECT(OBJECT) \
Status OneWayTicketImpl::find ## OBJECT(ServerContext* context, const OBJECT ## Request *request, OBJECT ## List *response) \
{ \
	if (request == NULL) \
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS); \
	BEGIN_GRPC_METHOD(STRINGIFY(find ## OBJECT), request, t) \
	t.reset(mDb->begin()); \
	std::string n(request->value().name()); \
	std::replace(n.begin(), n.end(), '*', '%'); \
	if (n.empty()) \
		n = "%"; \
	odb::result<OBJECT> qr(mDb->query<OBJECT>(odb::query<OBJECT>::name.like(n))); \
	INIT_CHECK_RANGE(request->listrequest(), r, sz, ofs)  \
	for (odb::result<OBJECT>::iterator i(qr.begin()); i != qr.end(); i++) \
	{ \
		CHECK_RANGE(r, sz, ofs) \
		OBJECT *n = response->add_value(); \
		*n = *i; \
	} \
	END_GRPC_METHOD(STRINGIFY(find ## OBJECT), request, t) \
	return Status::OK; \
}

// Find trip from to
Status OneWayTicketImpl::findTrip
(
	ServerContext *context,                 ///< server context
	const TripRequest *request,
	TripList *response
)
{
	if (request == NULL)
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("findTrip", request, t)

	// CHECK_PERMISSION(0)

	t.reset(mDb->begin());


	if (request->has_value() && (request->value().id()))
	{
		Trip *trip = response->add_trip();
		mDb->load(request->value().id(), *trip);
	}
	else
	{
		int ts;
		int tf;

		if (request->has_route_point_time())
		{
			ts = request->route_point_time().time_start();
			tf = request->route_point_time().time_finish();
		}
		else
		{
			ts = 0;          // time(NULL) - 1 * 24 * 60 * 60;
			tf = 2147483647; // ts + 30 * 24 * 60 * 60;
		}

		uint64_t route_tag_id = 0;	// any route tag
		if (request->has_value() && (request->value().has_route()))
			route_tag_id = request->value().route().route_tag().id();

#ifdef DB_POSTGRES
			odb::result<FindTrip1> qr(mDb->query<FindTrip1>(
				"trip.route = route.id AND route.route_tag = routetag.id AND veh.model = veh_model.id" +
					((request->has_value() && (request->value().id())) ? " AND trip.id = " + toString(request->value().id()) : "") +
					(route_tag_id ? (" AND routetag.id = " + toString(route_tag_id)) : "") +
					" AND trip.time_departure >= " + toString(ts) +
					" AND trip.time_departure <= " + toString(tf) +
					" AND trip.stage = 4 \
					AND trip.show_in_list = true \
					AND trip.vehicle = veh.id \
					AND \
					(2  = (SELECT COUNT(*) cnt \
					FROM \
					( \
					SELECT value_name \
					FROM \"Route_route_points\" \
					WHERE object_id = trip.route " +
					" AND value_name = '" + request->route_point_time().route_point_start() + "' \
					UNION \
					SELECT value_name \
					FROM \"Route_route_points\" \
					WHERE object_id = trip.route " +
					" AND value_name = '" + request->route_point_time().route_point_finish() + "') AS cnt))"));

			INIT_CHECK_RANGE(request->listrequest(), r, sz, ofs)
			for (odb::result<FindTrip1>::iterator it(qr.begin()); it != qr.end(); ++it)
			{
				CHECK_RANGE(r, sz, ofs)
				Trip *t = response->add_trip();
				t->set_id(it->trip_id);
				t->set_stage(it->trip_stage);
				t->set_time_departure(it->trip_time_departure);
				t->set_time_sale_start(it->trip_time_sale_start);
				t->set_time_sale_finish(it->trip_time_sale_finish);
				t->set_departure_gate(it->trip_departure_gate);
				t->mutable_vehicle()->set_id(it->trip_vehicle_id);

				t->mutable_driver()->set_id(it->trip_driver_id);
				t->set_show_in_list(it->trip_show_in_list);
				t->mutable_route()->mutable_route_tag()->set_id(it->route_route_tag);
				t->mutable_route()->mutable_route_tag()->set_name(it->routetag_name);
				t->mutable_route()->set_name(it->route_name);
				t->mutable_route()->set_route_number(it->route_route_number);
				t->mutable_route()->set_passenger_identification(it->route_passenger_identification);
				t->mutable_route()->mutable_default_rate()->set_id(it->route_default_rate);
				t->mutable_vehicle()->mutable_model()->set_id(it->vehicle_model);
				t->mutable_vehicle()->mutable_model()->set_name(it->vehicle_model_name);
				t->mutable_vehicle()->mutable_org_owner()->set_id(it->vehicle_org_owner);
				t->mutable_vehicle()->set_name(it->vehicle_name);
				t->mutable_vehicle()->set_license_plate(it->vehicle_license_plate);
				t->mutable_vehicle()->set_vin(it->vehicle_vin);
				t->mutable_vehicle()->set_year(it->vehicle_year);
				t->mutable_vehicle()->set_eco_class_number(it->vehicle_eco_class_number);
				t->mutable_vehicle()->set_use_classes(it->vehicle_use_classes);
				t->mutable_vehicle()->set_use_seats(it->vehicle_use_seats);

				// t->mutable_rate()->set_id(it->trip_rate_id);
				Rate *rate = t->mutable_rate();
				mDb->load(it->trip_rate_id, *rate);
				// t->mutable_route()->set_id(it->trip_route_id);
				Route *route = t->mutable_route();
				mDb->load(it->trip_route_id, *route);

				// ticket offer
				TicketOffer *to = t->mutable_ticket_offer();
				uint64_t price = calcTicket(t, request->route_point_time().route_point_start(), request->route_point_time().route_point_finish(), NULL);
				// LOG(ERROR) << t->id() << " " << request->route_point_time().route_point_start() << " " << request->route_point_time().route_point_finish() << " " << price;
				// show extra fees
				if (route->has_route_tag())
					route_tag_id = route->route_tag().id();
				to->set_total(price + calcExtraFeesByRouteTag(route_tag_id, price));
				*to->mutable_route_point_time() = request->route_point_time();
			}
#else
			std::vector<uint64_t> route_ids;
			findRouteIdsByStop(route_ids, route_tag_id,
					request->route_point_time().route_point_start(),
					request->route_point_time().route_point_finish(),
					true);

			LOG(INFO) << "findTrip route tag " << route_tag_id
					<< " start " << ts
					<< " finish " << tf
					<< " from " << request->route_point_time().route_point_start()
					<< " to " << request->route_point_time().route_point_finish()
					<< " in " << route_ids.size() <<  " route(s)";

			if (route_ids.size() <= 0)
			{
				LOG(INFO) << "no trip";
				return Status::OK;
			}

			for (std::vector<uint64_t>::iterator rout(route_ids.begin()); rout < route_ids.end(); rout++)
			{
				LOG(INFO) << "route id: " << *rout;
			}

			odb::query<Trip> q((odb::query<Trip>::time_departure >= ts) && (odb::query<Trip>::time_departure <= tf));

			if (request->has_value() && (request->value().id()))
					q = q && (odb::query<Trip>::id == request->value().id());

			// limit routes
			q = q && (odb::query<Trip>::route.in_range(route_ids.begin(), route_ids.end()));

			odb::result<Trip> qr(mDb->query<Trip>(q));

			INIT_CHECK_RANGE(request->listrequest(), r, sz, ofs)
			for (odb::result<Trip>::iterator i(qr.begin()); i != qr.end(); i++)
			{
				CHECK_RANGE(r, sz, ofs)
				Trip *t = response->add_trip();
				*t = *i;
			}
#endif
	}
	END_GRPC_METHOD("findTrip", request, t)
	return Status::OK;
}

FIND_OBJECT(Org)

FIND_OBJECT(Employee)

FIND_OBJECT(VehicleModel)

FIND_OBJECT(Vehicle)

FIND_OBJECT(Rate)

FIND_OBJECT(Route)

FIND_OBJECT(Passenger)

// FIND_OBJECT(Ticket)
// Find ticket(s) by value.trip.id, value.id, value.number, value.name
Status OneWayTicketImpl::findTicket
(
	ServerContext *context,                 ///< server context
	const TicketRequest *request,
	TicketList *response
)
{
	if (request == NULL)
		return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("findTicket", request, t)

	// MUST have!
	if (!request->has_value())
			return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);

	// CHECK_PERMISSION(0)

	t.reset(mDb->begin());

	odb::query<Ticket> q("1=1");
	if (request->value().id())
		q = q && (odb::query<Ticket>::id == request->value().id());
	if (!request->value().number().empty())
		q = q && (odb::query<Ticket>::number == request->value().number());
	if (!request->value().name().empty())
		q = q && (odb::query<Ticket>::name == request->value().name());

	if (request->value().has_trip())
	{
		if (request->value().trip().id())
			q = q && (odb::query<Ticket>::trip == request->value().trip().id());
	}

	if (request->value().has_passenger() && request->value().passenger().has_person())
	{
		std::string ph = request->value().passenger().person().phone_mobile();
		if (!ph.empty())
		{
			ph = E164ToString(ph);
			Person p;
			if (mDb->query_one<Person>(odb::query<Person>::phone_mobile == ph, p))
			{
				Passenger pa;
				if (mDb->query_one<Passenger>(odb::query<Passenger>::person == p.id(), pa))
				{
					q = q && (odb::query<Ticket>::passenger == pa.id());
				}
			}
		}
	}

	odb::result<Ticket> qr(mDb->query<Ticket>(q));
	INIT_CHECK_RANGE(request->listrequest(), r, sz, ofs)
	for (odb::result<Ticket>::iterator i(qr.begin()); i != qr.end(); i++)
	{
		CHECK_RANGE(r, sz, ofs)
		Ticket *n = response->add_value();
		if (i->medias_size())
		{
			i->clear_medias();
		}
		if (i->has_trip())
		{
			i->mutable_trip()->clear_medias();
			if (i->mutable_trip()->has_vehicle())
			{
				i->mutable_trip()->mutable_vehicle()->clear_medias();
				if (i->mutable_trip()->mutable_vehicle()->has_model())
				{
					i->mutable_trip()->mutable_vehicle()->mutable_model()->clear_medias();
				}
			}
			if (i->mutable_trip()->has_route())
			{
				i->mutable_trip()->mutable_route()->clear_medias();
			}
		}

		*n = *i;
	}
	END_GRPC_METHOD("findTicket", request, t)
	return Status::OK;
}

size_t OneWayTicketImpl::doRmExpiredTickets
(
	odb::database *db,
	uint64_t route_tag_id,
	uint32_t expiration_seconds
)
{
	if (!db)
		return 0;
	// MUST have!
	uint32_t expiration_time = time(NULL) - expiration_seconds;

	odb::query<Ticket> q((odb::query<Ticket>::stage == 0)
			&& (odb::query<Ticket>::time_reserved < expiration_time));

	odb::result<Ticket> qr(db->query<Ticket>(q));
	size_t r = 0;
	for (odb::result<Ticket>::iterator i(qr.begin()); i != qr.end(); i++)
	{
		i->set_stage(10);		// mark as deleted
		i->clear_seat_names();
		db->update(*i);
		r++;
	}
	return r;
}

/**
 * @brief Close sales (mark stage = 5 - online sale off, sale in bus)
 * @param db
 * @param route_tag_id
 * @param expiration_seconds
 * @return
 */
size_t OneWayTicketImpl::doFinishSales
(
	odb::database *db,
	uint64_t route_tag_id,
	uint32_t before_trip_seconds
)
{
	if (!db)
		return 0;
	// MUST have!
	uint32_t c_time = time(NULL) + before_trip_seconds;

	odb::query<Trip> q((odb::query<Trip>::stage == 4)
		&& (odb::query<Trip>::time_departure < c_time));

	odb::result<Trip> qr(db->query<Trip>(q));
	size_t r = 0;
	for (odb::result<Trip>::iterator i(qr.begin()); i != qr.end(); i++)
	{
		i->set_stage(5);		// mark as online sale off
		db->update(*i);
		r++;
	}
	return r;
}

/**
 * @brief Open sales (mark stage = 4 - online sale on, sale in bus)
 * @param db
 * @param route_tag_id
 * @param before_trip_seconds
 * @return
 */
size_t OneWayTicketImpl::doStartSales
(
	odb::database *db,
	uint64_t route_tag_id,
	uint32_t before_trip_seconds
)
{
	if (!db)
		return 0;
	// MUST have!
	uint32_t c_time = time(NULL) + before_trip_seconds;

	odb::query<Trip> q((odb::query<Trip>::stage < 4)
		&& (odb::query<Trip>::time_departure < c_time));

	odb::result<Trip> qr(db->query<Trip>(q));
	size_t r = 0;
	for (odb::result<Trip>::iterator i(qr.begin()); i != qr.end(); i++)
	{
		i->set_stage(4);		// mark as online sale off
		db->update(*i);
		r++;
	}
	return r;
}


Status OneWayTicketImpl::rmExpiredTickets
(
	ServerContext *context,		            ///< server context
	const ExpiredTicketRequest *request,
	OperationResponse *response
)
{
	if (request == NULL)
			return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("rmExpiredTickets", request, t)

	// CHECK_PERMISSION(0)
	uint64_t rtid;
	if (request->has_route_tag())
		rtid = request->route_tag().id();
	else
		rtid = 1;

	t.reset(mDb->begin());

	size_t count = doRmExpiredTickets(mDb, rtid, 30 * 60);

	response->set_code(0);
	response->set_id(count); // return count of deleted tickets

	END_GRPC_METHOD("rmExpiredTickets", request, t)
	return Status::OK;
}

void OneWayTicketImpl::doSetTripSeats
(
	const SetTripSeatsRequest *request,
	OperationResponse *response
)
{
	odb::result<Trip> qr(mDb->query<Trip>((odb::query<Trip>::route == request->route_id())
			&& (odb::query<Trip>::time_departure == request->time_departure())));
	Trip trip;
	bool tripExists = qr.begin() != qr.end();
	if (tripExists)
	{
		// trip already exists
		trip = *qr.begin();

		if (request->route_id())
			trip.mutable_route()->set_id(request->route_id());
		if (request->vehicle_id())
			trip.mutable_vehicle()->set_id(request->vehicle_id());
		if (request->rate_id())
			trip.mutable_rate()->set_id(request->rate_id());
		if (request->time_departure())
			trip.set_time_departure(request->time_departure());
		// server-side time
		if (request->time_departure())
		{
			trip.set_time_sale_finish(request->time_departure() - (mConfig->sales_finish_mins * 60));
			trip.set_time_sale_start(time(NULL));
		}
		if (request->stage())
			trip.set_stage(request->stage());
		if (request->departure_gate())
			trip.set_departure_gate(request->departure_gate());
		if (request->driver_id())
			trip.mutable_driver()->set_id(request->driver_id());
		trip.set_show_in_list(true);
	}
	else
	{
		trip.mutable_route()->set_id(request->route_id());
		trip.mutable_vehicle()->set_id(request->vehicle_id());
		trip.mutable_rate()->set_id(request->rate_id());
		trip.set_time_departure(request->time_departure());
		// server-side time
		trip.set_time_sale_finish(request->time_departure() - (mConfig->sales_finish_mins * 60));
		trip.set_time_sale_start(time(NULL));

		trip.set_stage(request->stage() == 0 ? 4 : request->stage());
		trip.set_departure_gate(request->departure_gate());
		trip.mutable_driver()->set_id(request->driver_id());
		trip.set_show_in_list(true);
	}

	if (request->seats_size())
	{
		// set provided seats
		trip.clear_seats();
		for (int s = 0; s < request->seats_size(); s ++)
		{
			*trip.mutable_seats()->Add() = request->seats(s);
		}
	}
	else
	{
		// no seats provided.
		if (!tripExists)
		{
			// for new trip, copy VehicleModel
			// load Vehicle model
			VehicleModel m;
			try {
				mDb->load(trip.vehicle().id(), m);
			} catch(...) {

			}
			// copy model to the vehicle
			*trip.mutable_vehicle()->mutable_model() = m;
			// copy seats from the VehicleModel
			for (int s = 0; s < trip.vehicle().model().seats_size(); s ++)
			{
				*trip.mutable_seats()->Add() = trip.vehicle().model().seats(s);
			}
		}
		else
		{
			// if trip updated and NO seats provided, do nothing.
		}

	}
	if (tripExists)
		mDb->update(trip);
	else
		trip.set_id(mDb->persist(trip));
	response->set_code(trip.id() == 0 ? 400 : 0);
	response->set_id(trip.id()); // return trip id
}

Status OneWayTicketImpl::setTripSeats
(
	ServerContext *context,		            ///< server context
	const SetTripSeatsRequest *request,
	OperationResponse *response
)
{
	if (request == NULL)
			return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("setTripSeats", request, t)

	// CHECK_PERMISSION(0)

	t.reset(mDb->begin());

	doSetTripSeats(request, response);

	END_GRPC_METHOD("setTripSeats", request, t)
	return Status::OK;
}

Status OneWayTicketImpl::setTripsSeats
(
	ServerContext *context,		            ///< server context
	const SetTripsSeatsRequest *request,
	SetTripsSeatsResponse *responses
)
{
	if (request == NULL)
			return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("setTripsSeats", request, t)

	// CHECK_PERMISSION(0)

	t.reset(mDb->begin());

	for (int i = 0; i < request->value_size(); i++)
	{
		doSetTripSeats(&request->value(i), responses->add_response());
	}

	END_GRPC_METHOD("setTripsSeats", request, t)
	return Status::OK;
}

//------------------------ GTFS -----------------------------

/**
 * @brief return first route's trip at specified time
 * @param route_id route identifier
 * @param t time unix epoch
 */
uint64_t OneWayTicketImpl::getTripByRouteNTime1
(
	uint64_t route_id,
	uint64_t t
)
{
	uint64_t id;
#ifdef DB_POSTGRES
	odb::result<FindTripId> qr(mDb->query<FindTripId>(	"route = " + toString(route_id) + " AND time_departure = " + toString(t)));
	for (odb::result<FindTripId>::iterator it(qr.begin()); it != qr.end(); ++it)
	{
		id = it->trip_id;
		break;
	}
	return id;
#else
	odb::query<Trip> q((odb::query<Trip>::time_departure == t) && (odb::query<Trip>::route == route_id));
	odb::result<Trip> qr(mDb->query<Trip>(q));
	for (odb::result<Trip>::iterator i(qr.begin()); i != qr.end(); i++)
	{
		id = i->trip_id;
	}
#endif
}

/**
 * @brief check is time is midnight in the list
 * @param dates list of midnights
 * @param r time
 */
bool gtfsTimeInDay
(
	const std::vector<time_t> &dates,
	time_t t
)
{
	for (std::vector<time_t>::const_iterator it(dates.begin()); it != dates.end(); it++)
	{
		if (t == *it)
			return true;
	}
	return false;
}

uint32_t OneWayTicketImpl::doAddGTFSTrip
(
	uint64_t route_tag,
	int timezone_offset,
	const GTFSTrip &trip
)
{
	time_t start = trip.start();
	time_t finish = trip.finish();
	
	// convert holydays midnight time
	std::vector<time_t> holydays;
	for (int i = 0; i < trip.holydays_size(); i++)
	{
		holydays.push_back(gtfs::GTFSDate::toTime(timezone_offset, trip.holydays(i)));
	}

	if (trip.weekdays() != 0)
	{
		// get week day of the first 
		int weekday = gtfs::GTFSDate::wdayInTimezone(start, timezone_offset);

		time_t date_midnight = start;
		// by week days
		while (date_midnight <= finish)
		{
			if ((trip.weekdays() & (1 << weekday)) && (!gtfsTimeInDay(holydays, date_midnight)))
			{
				doAddGTFSTripDay(trip, date_midnight);
			}
			// next day (and week day)
			date_midnight = start + 86400;
			weekday++;
			if (weekday > 6)
				weekday = 0;
		}
	}
	// by working days
	for (int i = 0; i < trip.workdays_size(); i++)
	{
		time_t t = gtfs::GTFSDate::toTime(timezone_offset, trip.workdays(i));
		doAddGTFSTripDay(trip, t);
	}
	return 0;
}

/**
 * @brief GTFS trip has 1 or more departure times, e.g. 8:00AM, 4:00PM
 */
uint32_t OneWayTicketImpl::doAddGTFSTripDay
(
	const GTFSTrip &trip,
	time_t date_midnight
)
{
	// loop hours
	for (uint32_t dh = 0; dh < trip.depart_hours_size(); dh++)
	{
		uint32_t hm = trip.depart_hours(dh);
		uint32_t h = hm / 100;
		uint32_t m = hm % 100;
		time_t s = date_midnight + (h * 60 * 60) + (m * 60);
		if (s > trip.finish())
			break;
		
		uint64_t trip_exists = getTripByRouteNTime1(trip.route_id(), s);
		if (trip_exists > 0)
		{
			LOG(INFO) << "GTFS trip id " << trip_exists << " route: " << trip.route_id() << " time: " << s << " already exists";
		}
		else
		{
			Trip t;
			t.set_time_departure(s);
			t.mutable_route()->set_id(trip.route_id());
			int64_t id = mDb->persist(t);
			t.set_id(id);
			mDb->update(t);
		}
	}
	return 0;
}

// GTFS batch
Status OneWayTicketImpl::addGTFSTrips
(
	ServerContext *context,		            ///< server context
	const GTFSTrips *request,
	AddGTFSTripsResponse *responses
)
{
	if (request == NULL)
			return Status(StatusCode::INVALID_ARGUMENT, ERR_SVC_INVALID_ARGS);
	BEGIN_GRPC_METHOD("addGTFSTrips", request, t)

	// CHECK_PERMISSION(0)

	t.reset(mDb->begin());
	
	for (int i = 0; i < request->trips_size(); i++)
	{
		uint32_t code = 0;
		doAddGTFSTrip(request->route_tag(), request->timezone_offset(), request->trips(i));
		responses->add_codes(code);
	}
	
	END_GRPC_METHOD("addGTFSTrips", request, t)
	return Status::OK;
}
