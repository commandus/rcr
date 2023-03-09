/**
 * ./mkdb connection-string
 */
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

#include <time.h>

#include <third-party/argtable3/argtable3.h>
#include <grpc++/grpc++.h>
#include "gen/rcr.pb-odb.hxx"


const char* progname = "mkdb";

#define DEF_PORT		        50051
#define DEF_ADDRESS			    "127.0.0.1"

/**
 * Parse command line into struct ClientConfig
 * Return 0- success
 *        1- show help and exit, or command syntax error
 *        2- output file does not exists or can not open to write
 **/
int parseCmd
(
	int argc,
	char* argv[],
	struct ClientConfig *value
)
{
	struct arg_str *a_interface = arg_str0("i", "hostname", "<service host name>", "service host name. Default onewayticket.commandus.com");
	struct arg_int *a_port = arg_int0("l", "listen", "<port>", "service port. Default 50051");
	// SSL
	struct arg_lit *a_sslon = arg_lit0("s", "sslon", "SSL on");
	struct arg_file *a_fnpemkey = arg_file0("k", "keyfile", "<file name>", "Optional client private key PEM file. Default client.key");
	struct arg_file *a_fnpemcertificate = arg_file0("c", "certificate", "<file name>", "Client certificate PEM file. Default client.crt");
	struct arg_file *a_fnpemcacertificate = arg_file0("r", "rootca", "<file name>", "CA certificates PEM file. Force client SSL certificate validation. Default ca.crt.");

	struct arg_lit *a_verbose = arg_litn("v", "verbose", 0, 5, "Verbose level");

	struct arg_lit *a_help = arg_lit0("h", "help", "Show this help");
	struct arg_end *a_end = arg_end(20);

	void* argtable[] = { a_interface, a_port,
		a_sslon,
		a_fnpemkey, a_fnpemcertificate, a_fnpemcacertificate,
		// commands
		a_find, a_add, a_rm, a_set,
		// command parameters
		a_jsonspar, a_filejsonspar, a_route_tag,
		// other parameters
		a_getuser, a_adduser, a_rmuser, a_setuser,
		a_lsroutetag, a_lsroutepoint, a_lstrip, a_lsavailableseats, a_lsoccupiedseats,
		a_rmexpired,
		a_booking_ticket, a_pay, a_cancel_payment,
		// options
		a_phonenumber, a_gcmid, a_lat, a_lon, a_start, a_finish,
		a_point, a_seat, a_discount,
		// assign a new trip
		a_set_trip_seats, a_vehicle, a_rate, a_driver, a_gate, a_stage,
		//
		a_gtfs_folder, a_gtfs_commit, a_gtfs_check, a_gtfs_print, a_time_zone_offset,

		a_repeats, a_verbose,
		a_help, a_end };

	int nerrors;

	// verify the argtable[] entries were allocated successfully
	if (arg_nullcheck(argtable) != 0)
	{
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return 1;
	}
	// Parse the command line as defined by argtable[]
	nerrors = arg_parse(argc, argv, argtable);

	// special case: '--help' takes precedence over error reporting
	if ((a_help->count) || nerrors)
	{
		if (nerrors)
			arg_print_errors(stderr, a_end, progname);
		printf("Usage: %s\n",  progname);
		arg_print_syntax(stdout, argtable, "\n");
		printf("onewayticket CLI client\n");
		arg_print_glossary(stdout, argtable, "  %-25s %s\n");
		arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
		return 1;
	}

	if (a_gtfs_commit->count > 0 || a_gtfs_print->count > 0)
	{
		if ((!a_start->count) || (!a_finish->count))
		{
			std::cerr << "Error: missed arguments: --start, --finish" << std::endl;
			arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
			return 1;
		}
	}

	if (a_interface->count)
		value->intface = *a_interface->sval;
	else
		value->intface = DEF_ADDRESS;

	if (a_port->count)
		value->port = *a_port->ival;
	else
		value->port = DEF_PORT;

	// SSL

	value->sslon = a_sslon->count > 0;

	if (a_fnpemkey->count)
		value->fnpemkey = *a_fnpemkey->filename;
	else
		value->fnpemkey = DEF_FNPEMKEY;

	if (a_fnpemcertificate->count)
		value->fnpemcertificate = *a_fnpemcertificate->filename;
	else
		value->fnpemcertificate= DEF_FNPEMCERTIFICATE;

	if (a_fnpemcacertificate->count)
		value->fnpemcacertificate = *a_fnpemcacertificate->filename;
	else
		value->fnpemcacertificate = DEF_FNPEMCACERTIFICATE;

	// commands
	value->command = CMD_NONE;
	value->phone = "";
	value->gcmid = "";
	value->sn = NULL;
	value->radius = 0;
	value->lat = DEF_LAT;
	value->lon = DEF_LON;


	if (a_jsonspar-> count)
		value->jsonpar = *a_jsonspar->sval;
	else
		if (a_filejsonspar->count)
			value->jsonpar = file2string(*a_filejsonspar->filename);
		else
			value->jsonpar = "";


	if (a_getuser->count)
	{
		value->command = CMD_GETUSER;
		value->phone = *a_getuser->sval;
	}
	if (a_adduser->count)
	{
		value->command = CMD_ADDUSER;
		value->cn = *a_adduser->sval;
	}
	if (a_rmuser->count)
	{
		value->command = CMD_RMUSER;
	}
	if (a_setuser->count)
	{
		value->command = CMD_SETUSER;
		value->cn = *a_setuser->sval;
	}

	// commands
	if (a_find->count)
	{
		value->command = CMD_FIND;
		value->obj = string2objname(*a_find->sval);
	}

	if (a_add->count)
	{
		value->command = CMD_ADD;
		value->obj = string2objname(*a_add->sval);
	}

	if (a_rm->count)
	{
		value->command = CMD_RM;
		value->obj = string2objname(*a_rm->sval);
	}

	if (a_set->count)
	{
		value->command = CMD_SET;
		value->obj = string2objname(*a_set->sval);
	}

	// other commands
	if (a_lsroutetag->count)
	{
		value->command = CMD_LSROUTETAG;
	}
	if (a_lsroutepoint->count)
	{
		value->command = CMD_LSROUTEPOINT;
	}


	if (a_route_tag->count)
	{
		value->route_tag = *a_route_tag->ival;
	}
	else
		value->route_tag = 1;

	if (a_lstrip->count)
	{
		value->command = CMD_LSTRIP;
		value->ids.push_back(*a_lstrip->ival);
		time_t dt;
		if (a_start->count)
			dt = parseDate(*a_start->sval);
		else
			dt = 0;
		value->ids.push_back(dt);
		if (a_finish->count)
			dt = parseDate(*a_finish->sval);
		else
			dt = 0;
		value->ids.push_back(dt);
	}

	if (a_lsavailableseats->count)
	{
		value->command = CMD_LSAVAILABLESEATS;
		value->ids.push_back(*a_lsavailableseats->ival);
	}

	if (a_lsoccupiedseats->count)
	{
		value->command = CMD_LSOCCUPIEDSEATS;
		value->ids.push_back(*a_lsoccupiedseats->ival);
	}

	if (a_rmexpired->count)
	{
		value->command = CMD_RM_EXPIRED;
		value->ids.push_back(*a_rmexpired->ival);
	}

	if (a_booking_ticket->count)
	{
		value->command = CMD_BOOKINGTICKET;
		value->ids.push_back(*a_booking_ticket->ival);
		if ((a_seat->count == 0) || (a_point->count < 2))
		{
			// discount is optional, seat(s) and points required
			nerrors++;
		}
		else
		{
			if (a_discount->count > 0)
				value->discount = *a_discount->ival;
			else
				value->discount = 0;

			for (int i = 0; i < a_point->count; i++)
			{
				value->points.push_back(a_point->sval[i]);
			}
			for (int i = 0; i < a_seat->count; i++)
			{
				value->seats.push_back(a_seat->sval[i]);
			}
		}
	}

	if (a_pay->count)
	{
		value->command = CMD_PAY;
		value->ids.push_back(*a_pay->ival);
		if (value->jsonpar.empty())
		{
			// no JSON is provided
			nerrors++;
		}
	}

	if (a_cancel_payment->count)
	{
		value->command = CMD_CANCEL_PAYMENT;
		value->ids.push_back(*a_cancel_payment->ival);
	}

	// assign a new trip
	// , a_vehicle, a_rate, a_driver, a_gate,
	if (a_set_trip_seats->count)
	{
		value->command = CMD_SET_TRIP_SEATS;
		value->ids.push_back(*a_set_trip_seats->ival);	// route id
		if (a_start->count == 0)
		{
			// all others optional
			nerrors++;
		}
		else
		{
			value->ids.push_back(parseDate(*a_start->sval));
			if (a_vehicle->count > 0)
				value->ids.push_back(*a_vehicle->ival);
			else
				value->ids.push_back(0);
			if(a_rate->count > 0)
				value->ids.push_back(*a_rate->ival);
			else
				value->ids.push_back(0);
			if (a_driver->count > 0)
				value->ids.push_back(*a_driver->ival);
			else
				value->ids.push_back(0);
			if (a_gate->count > 0)
				value->ids.push_back(*a_gate->ival);
			else
				value->ids.push_back(0);
			if (a_stage->count > 0)
				value->ids.push_back(*a_stage->ival);
			else
				value->ids.push_back(0);

			for (int i = 0; i < a_seat->count; i++)
			{
				value->seats.push_back(a_seat->sval[i]);
			}
		}
	}

	if (a_time_zone_offset->count)
	{
		value->timezone_offset = *a_time_zone_offset->ival;
	}
	else
	{
		value->timezone_offset = getCurrentTimeOffset();
	}
	
	// GTFS operation requires dates
	if (a_gtfs_commit->count)
	{
		value->command = CMD_GTFS_COMMIT;
		time_t dt;
		if (a_start->count)
			dt = parseDate(*a_start->sval);
		else
			dt = 0;
		value->ids.push_back(dt);
		if (a_finish->count)
			dt = parseDate(*a_finish->sval);
		else
			dt = 0;
		value->ids.push_back(dt);

	}

	if (a_gtfs_folder->count)
		value->gtfs_path = *a_gtfs_folder->filename;
	else
		value->gtfs_path = DEF_GTFS_FOLDER;
	
	if (a_gtfs_check->count)
	{
		value->command = CMD_GTFS_CHECK;
	}

	// GTFS operation requires dates
	if (a_gtfs_print->count)
	{
		value->command = CMD_GTFS_PRINT;
		value->format = *a_gtfs_print->ival;
		
		time_t dt;
		if (a_start->count)
			dt = parseDate(*a_start->sval);
		else
			dt = 0;
		value->ids.push_back(dt);
		if (a_finish->count)
			dt = parseDate(*a_finish->sval);
		else
			dt = 0;
		value->ids.push_back(dt);

	}

	// options
	if (a_phonenumber->count)
	{
		value->phone = *a_phonenumber->sval;
	}
	if (a_gcmid->count)
	{
		value->gcmid = *a_gcmid->sval;
	}
	if (a_lat->count)
	{
		value->lat = *a_lat->dval;
	}
	if (a_lon->count)
	{
		value->lon = *a_lon->dval;
	}

	if (a_repeats->count)
	{
		value->repeats = *a_repeats->ival;
	}
	else
		value->repeats = 1;

	value->verbose = a_verbose->count;

	arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
	return 0;
}

/**
 * Open file to read, skip UTF-8 BOM if exists.
 */
std::ifstream *openUtf8BOM(const std::string &fn)
{
	std::ifstream *ret = new std::ifstream(fn, std::ifstream::in);
	// remove byte order mark (BOM) 0xef 0xbb 0xbf
	unsigned char bom[3];
	ret->read((char*) bom, 3);
	if (!((bom[0] == 0xef) && (bom[1] == 0xbb) && (bom[2] == 0xbf)))
		ret->seekg(0);
	return ret;
}

int main(int argc, char** argv)
{
	struct ClientConfig config;
	int r;
	if (r = parseCmd(argc, argv, &config))
		exit(r);

	if (config.verbose > 1)
	{
		 std::cout << "Version: " << SSLeay_version(SSLEAY_VERSION) << std::endl;
		 if (config.command == CMD_LSTRIP)
		 {
			 std::cerr << "route: " << config.ids[0] << " start: " << config.ids[1] << " finish: " << config.ids[2] << std::endl;
		 }
	}
	OneWayTicketClient rpc(config.intface, config.port, "n/a", "n/a",
			config.sslon,
			file2string(config.fnpemkey),
			file2string(config.fnpemcertificate),
			file2string(config.fnpemcacertificate),
			config.repeats);

	// Print out client identifier
	// std::cout << "User: " << rpc.cn << std::endl;

	int code = 0;
	std::ifstream *strmin;
	std::ostream *strmout;
	switch (config.command)
	{
		case CMD_NONE:
			std::cerr << "No command specified, exit." << std::endl;
			break;
		case CMD_GETUSER:
			{
				User ret;
				bool r = rpc.getUser(config.phone, &ret);
				if (r)
				{
					std::string out;
					pbjson::pb2json(&ret, out);
					std::cout << out << std::endl;
				}
				else
					std::cerr << "user not found" << std::endl;
			}
			break;
		case CMD_ADDUSER:
			{
				User retUser;
				Status status;
				uint64_t r = rpc.addUser(config.cn, config.phone, config.gcmid, retUser, status);
				if (status.ok())
				{
					std::string out;
					retUser.clear_cert();
					pbjson::pb2json(&retUser, out);
					std::cout << out << std::endl;
				}
				else
				{
					std::cerr << status.error_code() << ": " << status.error_message() << std::endl;
				}
			}
			break;
		case CMD_RMUSER:
			{
				bool  r = rpc.rmUser();
				std::cout << r << std::endl;
			}
			break;
		case CMD_SETUSER:
			{
				bool r = rpc.setUser(config.cn, config.phone, config.gcmid);
				std::cout << r << std::endl;
			}
			break;

		// commands: user|routetag|org|employee|vehiclemodel|vehicle|rate|route|trip|ticket
		case CMD_FIND:
			{
				std::string r = rpc.find_json(config.obj, config.jsonpar);
				std::cout << r << std::endl;
			}
			break;
		case CMD_ADD:
			{
				config.jsonpar = "{\"person\":{\"first_name\":\"First Name\"}}";
				std::string r = rpc.add(config.obj, config.jsonpar);
				std::cout << r << std::endl;
			}
			break;
		case CMD_RM:
			{
				std::string r = rpc.rm(config.obj, config.jsonpar);
				std::cout << r << std::endl;
			}
			break;
		case CMD_SET:
			{
				std::string r = rpc.set(config.obj, config.jsonpar);
				std::cout << r << std::endl;
			}
			break;
		// other commands
		// route tag
		case CMD_LSROUTETAG:
			{
				RouteTagList l;
				size_t r = rpc.lsRouteTag(&l);
				if (r)
				{
					std::string out;
					pbjson::pb2json(&l, out);
					std::cout << out << std::endl;
				}
			}
			break;
		// route stop
		case CMD_LSROUTEPOINT:
			{
				RoutePointList l;
				size_t r = rpc.lsRoutePoint(config.route_tag, &l);
				if (r)
				{
					std::string out;
					pbjson::pb2json(&l, out);
					std::cout << out << std::endl;
				}
			}
			break;
		// trip
		case CMD_LSTRIP:
			{
				TripListResponse l;
				size_t r = rpc.lsTrip(&l, config.ids[0], config.ids[1], config.ids[2]);
				if (r)
				{
					std::string out;
					pbjson::pb2json(&l, out);
					std::cout << out << std::endl;
				}
			}
			break;
		case CMD_LSAVAILABLESEATS:
			{
				SeatResponse sr;
				size_t r = rpc.lsAvailableSeats(&sr, config.ids[0]);
				if (r)
				{
					std::string out;
					pbjson::pb2json(&sr, out);
					std::cout << out << std::endl;
				}
			}
			break;
		case CMD_LSOCCUPIEDSEATS:
			{
				SeatResponse sr;
				size_t r = rpc.lsOccupiedSeats(&sr, config.ids[0]);
				if (r)
				{
					std::string out;
					pbjson::pb2json(&sr, out);
					std::cout << out << std::endl;
				}
			}
			break;
		case CMD_BOOKINGTICKET:
			{
				BookingResponse sr;
				uint64_t r = rpc.bookingTicket(&sr, config.ids[0],
						config.points,
						config.seats,
						config.discount);
				std::string out;
				pbjson::pb2json(&sr, out);
				std::cout << r << std::endl
						<< out << std::endl;
			}
			break;

		case CMD_PAY:
			{
				PaymentResponse sr;
				uint64_t r = rpc.payTicket(&sr, config.ids[0], config.jsonpar);
				std::string out;
				pbjson::pb2json(&sr, out);
				std::cout << r << std::endl
						<< out << std::endl;
			}
			break;

		case CMD_CANCEL_PAYMENT:
			{
				OperationResponse sr;
				uint64_t r = rpc.cancelPayment(&sr, config.ids[0]);
				std::string out;
				pbjson::pb2json(&sr, out);
				std::cout << r << std::endl
						<< out << std::endl;
			}
			break;

		case CMD_RM_EXPIRED:
		{
			OperationResponse ret;
			uint64_t r = rpc.rmExpiredTickets(&ret, config.ids[0]);
			std::string out;
			pbjson::pb2json(&ret, out);
			std::cout << r << std::endl
					<< out << std::endl;
		}
		break;
		case CMD_SET_TRIP_SEATS:
		{
			OperationResponse ret;
			uint64_t r = rpc.setTripSeats(&ret, config.ids, config.seats);
			std::string out;
			pbjson::pb2json(&ret, out);
			std::cout << r << std::endl
					<< out << std::endl;
		}
		break;

		case CMD_GTFS_CHECK:
		{
			gtfs::GTFSBuilder b(config.gtfs_path);
			b.read();

			RouteTagList routetag_list;
			rpc.lsRouteTag(&routetag_list);

			RouteList route_list;
			rpc.lsRoute(&route_list);

			gtfs_check(b, config.route_tag, routetag_list, route_list);

			b.write();
			if (b.errorCode())
			{
				std::cout << "Error code: " << b.errorCode() << std::endl;
			}
			std::cout << "Check files in the " << config.gtfs_path << " using tools/gtfs-check.sh" << std::endl;
		}
		break;

		case CMD_GTFS_COMMIT:
		{
			gtfs::GTFSBuilder b(config.gtfs_path, config.ids[0], config.ids[1]);
			b.read();
			// override default time zone
			b.time_zone_offset = config.timezone_offset;

			GTFSTrips gtfs_trips = gtfs_commit(b, config.route_tag);
			rpc.addGTFSTrips(gtfs_trips);

			if (b.errorCode())
			{
				std::cout << "Error code: " << b.errorCode() << std::endl;
			}
			std::cout << "Check service database " << std::endl;
		}
		break;

		case CMD_GTFS_PRINT:
		{
			gtfs::GTFSBuilder b(config.gtfs_path, config.ids[0], config.ids[1]);
			b.read();
			// override default time zone
			b.time_zone_offset = config.timezone_offset;
			switch (config.format) {
				case 1:
				{
					b.stop_times.rmExceptStart();
					std::cout
						<< b.toJSON()
						<< std::endl;
				}
				break;
				case 2:
				{
					std::cout
						<< b.toJSON()
						<< std::endl;
				}
				break;
				default:
				{
					GTFSTrips gtfs_trips = gtfs_commit(b, config.route_tag);
					std::string out;
					pbjson::pb2json(&gtfs_trips, out);
					std::cout << out;
				}	
			}
		}
		break;

	default:
			std::cerr << "Unknown command, exit." << std::endl;
			break;
	}

	if (code != 0)
		std::cerr << "Error: " << code << std::endl;

	return 0;
}
