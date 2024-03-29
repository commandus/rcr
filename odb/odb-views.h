/**
  *	ODB native views used in search queries
  */
#include <string>
#include <cstdint>
#include <odb/core.hxx>
#include "gen/rcr.pb.h"

namespace rcr
{
	// --------------------- Native views ---------------------
#pragma db view query("SELECT count(id) FROM \"Card\" WHERE (?)")
struct CardCount {
    std::size_t count;
};

// WHERE (?) LIMIT(?) OFFSET(?)
#pragma db view query("SELECT id, name, uname, symbol_id, nominal FROM \"Card\" WHERE (?) ORDER BY uname, nominal")
struct CardQuery {
    uint64_t id;
    std::string name;
    std::string uname;
    uint64_t symbol_id;
    uint64_t nominal;
};

#pragma db view query("SELECT id, dt, user_id, package_id, operation_symbol, value FROM \"Journal\" WHERE (?) order by id desc")
struct JournalQuery {
    uint64_t id;
    uint64_t dt;
    uint64_t user_id;
    uint64_t package_id;
    std::string operation_symbol;
    int64_t value = 6;                            ///< amount
};

#pragma db view query("SELECT count(id) FROM \"Journal\" WHERE (?)")
struct JournalCount {
    std::size_t count;
};

#pragma db view query("SELECT count(id) FROM \"Box\" WHERE (?)")
    struct BoxCount {
        std::size_t count;
    };
}
