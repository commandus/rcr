/**
  *	ODB native views used in search queries
  */
#include <string>
#include <stdint.h>
#include <odb/core.hxx>

namespace rcr
{
	// --------------------- Native views ---------------------
#pragma db view query("SELECT count(id) FROM \"Card\" " \
    "WHERE (?)")
    struct CardCount
    {
        std::size_t count;
    };
}
