/**
  *	ODB native views used in search queries
  */
#include <string>
#include <stdint.h>
#include <odb/core.hxx>

#include "config.h"

namespace mgp
{
	// --------------------- Native views ---------------------

	#pragma db view query("SELECT DISTINCT value_name, value_latitude, value_longitude, value_sell_flags FROM \"Route_route_points\" " \
		"WHERE (?) ORDER BY value_name")
	struct RoutePointNameLatitudeLongitude
	{
		#pragma db type("text")
		std::string name;
		#pragma db type("double precision")
		double latitude;
		#pragma db type("double precision")
		double longitude;
		#pragma db type("integer")
		uint32_t sell_flags;
	};

#ifdef DB_POSTGRES
	/*
	 * PostgreSQL
	 * routetag.id = ?             route tag
	 * (trip.time_departure >= ?)
	 * (trip.time_departure <= ?)
	 * (value_name = '') from
	 * (value_name = '') to
	 * */
	#pragma db view query("SELECT trip.id, trip.route, trip.stage, trip.time_departure, trip.time_sale_start, \
trip.time_sale_finish, trip.departure_gate, \
trip.vehicle, trip.rate, trip.driver, \
trip.show_in_list, \
route.route_tag, route.\"name\", route.route_number, route.passenger_identification, route.default_rate, \
routetag.name routetag_name, \
veh.model, veh_model.name, veh.org_owner, veh.\"name\" vehicle_name, veh.license_plate, veh.vin, veh.\"year\", veh.eco_class_number, \
veh.use_classes, veh.use_seats \
FROM \"Trip\" trip, \"Route\" route, \"RouteTag\" routetag, \"Vehicle\" veh, \"VehicleModel\" veh_model \
WHERE (?) ORDER BY trip.id DESC")
#else
#error  Only PostgreSQL struct FindTrip1 native view defined, for specific database system add SELECT
#endif

	struct FindTrip1
	{
		#pragma db type("bigint")
		uint64_t trip_id;
		#pragma db type("bigint")
		uint64_t trip_route_id;
		#pragma db type("integer")
		uint32_t trip_stage;
		#pragma db type("integer")
		uint32_t trip_time_departure;
		#pragma db type("integer")
		uint32_t trip_time_sale_start;
		#pragma db type("integer")
		uint32_t trip_time_sale_finish;
		#pragma db type("integer")
		uint32_t trip_departure_gate;
		#pragma db type("bigint")
		uint64_t trip_vehicle_id;
		#pragma db type("bigint")
		uint64_t trip_rate_id;
		#pragma db type("bigint")
		uint64_t trip_driver_id;
		#pragma db type("boolean")
		bool trip_show_in_list;

		#pragma db type("bigint")
		uint64_t route_route_tag;
		#pragma db type("text")
		std::string route_name;
		#pragma db type("text")
		std::string route_route_number;
		#pragma db type("integer")
		uint32_t route_passenger_identification;
		#pragma db type("bigint")
		uint64_t route_default_rate;
		#pragma db type("text")
		std::string routetag_name;

		#pragma db type("bigint")
		uint64_t vehicle_model;
		#pragma db type("text")
		std::string vehicle_model_name;
		#pragma db type("bigint")
		uint64_t vehicle_org_owner;
		#pragma db type("text")
		std::string vehicle_name;
		#pragma db type("text")
		std::string vehicle_license_plate;
		#pragma db type("text")
		std::string vehicle_vin;
		#pragma db type("integer")
		uint32_t vehicle_year;
		#pragma db type("integer")
		uint32_t vehicle_eco_class_number;

		#pragma db type("boolean")
		bool vehicle_use_classes;
		#pragma db type("boolean")
		bool vehicle_use_seats;
	};

#ifdef DB_POSTGRES
	#pragma db view query("SELECT id FROM \"Trip\" trip WHERE (?) ORDER BY id")
#else
	#error  Only PostgreSQL struct FindTrip1 native view defined, for specific database system add SELECT
#endif
	struct FindTripId
	{
		#pragma db type("bigint")
		uint64_t trip_id;
	};

}
