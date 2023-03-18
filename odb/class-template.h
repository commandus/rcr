#include <odb/core.hxx>
// #include "gen/rcr.pb.h"

// odb -o gen -d sqlite -x -std=c++14 --options-file odb\options.pgsql --fkeys-deferrable-mode not_deferrable --generate-query --generate-schema -I. -I odb -I C:\git\vcpkg\installed\x64-windows\include odb/class-template.h
namespace rcr {
class Operation;

#pragma db value(rcr::Operation) transient

#pragma db object transient
class Operation
{
private:
int a;
// #pragma db member transient
// rcr::Operation pb;
public:
#pragma db member(id) virtual(uint64_t) get(pb.id) set(pb.set_id) id auto
// #pragma db member(Operation::symbol) virtual(std::string) get(pb.symbol) set(pb.set_symbol)
private:
    friend class odb::access;
};

}