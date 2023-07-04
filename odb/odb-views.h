/**
  *	ODB native views used in search queries
  */
#include <string>
#include <stdint.h>
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

}
