/**
 * pragmas.pgsql.hxx
 * ODB compiler pragmas
 * Use getter/setters instead of direct access to private member:
 *
 * 		Numbers:
 * 			#pragma db member(Message::memberInt_) get(memberInt) set(set_memberInt)
 * 		String:
 * 			#pragma db member(Message::memberString_) transient
 * 			#pragma db member(Message::memberString) virtual(std::string) get(memberString) set(set_memberString)
 * 		Message (Object):
 * 			#pragma db member(Message::object_) access(object_)
 * 			#pragma db index(Message::"message_object_idx") member(object_)
 * 		Stream (vector) of messages (objects):
 * 			#pragma db member(Message::headers_) transient
 * 			#pragma db member(Message::mheaders) get(getheaders) set(setheaders)
 *			Add to the end of prepare-pb-odb script next line:
 *				addvector Message headers HeaderType
 *
 * #pragma db value(Symbol)
 * #pragma db member(Symbol::id) virtual(uint64_t) get(id) set(set_id)
 * #pragma db member(Symbol::sym) virtual(std::string) get(sym) set(_internal_set_sym)
 * #pragma db object(Component) transient
 * #pragma db member(Component::id) virtual(uint64_t) get(id) set(set_id) id auto
 * #pragma db member(Component::symbol) virtual(PSymbol) get(::rcr::Symbol (mutable_symbol)) set(::rcr::Symbol (set_allocated_symbol))
 * #pragma db member(Component::symbol) virtual(PSymbol) get(::rcr::Symbol (mutable_symbol)) set(::rcr::Symbol (set_allocated_symbol))
 *
 */
// #pragma db value(google::protobuf::internal::ArenaStringPtr) type ("VARCHAR(255)")
#pragma db value(google::protobuf::internal::ArenaStringPtr) type ("TEXT")

#include <memory>
#include <vector>

/*
 * Non-persistent messages:
 *	Attribute OperationResponse ListRequest SearchRequest Credentials User Phone
 * Persistent messages:
 * 
 */

#define STRINGIFY2(a) STRINGIFY(a)
#define STRINGIFY(a) #a

#define ODB_VALUE(C, T) \
	_Pragma(STRINGIFY(db value(C) virtual(T)))

#define ODB_TABLE(C) \
	_Pragma(STRINGIFY(db object(C) transient)) \
	_Pragma(STRINGIFY(db member(C::id) virtual(uint64_t) get(id) set(set_id) id auto))

#define ODB_TRANSIENT(C, c) \
	_Pragma(STRINGIFY(db member(C::c) transient))

#define IDX(C, c, m) \
    db index(C::STRINGIFY2(c ## _ ## m ## _idx)) member(m ## _)

#define IDX_UNIQUE(C, c, m) \
    db index(C::STRINGIFY2(c ## _ ## m ## _idx)) unique member(m ## _)

#define ODB_IDX_UNIQUE(C, c, m) \
    _Pragma(STRINGIFY2(IDX_UNIQUE(C, c, m)))

#define ODB_IDX(C, c, m) \
    _Pragma(STRINGIFY2(IDX(C, c, m)))

#define ODB_NUMBER(c, m, T) \
    _Pragma(STRINGIFY(db member(c::m) virtual(T) get(m) set(set_ ## m)))

#define ODB_BOOL(c, m) \
    ODB_NUMBER(c, m, int)

// Protobuf since version XX changed call set_ to _internal_set_
#define ODB_STRING(c, m) \
    _Pragma(STRINGIFY(db member(c::m) virtual(std::string) get(m) set(_internal_set_ ## m)))

// big text or blob
// Protobuf since version XX changed call set_ to _internal_set_
#define ODB_TEXT(c, m) \
    _Pragma(STRINGIFY(db member(c::m) virtual(std::string) get(m) set(_internal_set_ ## m) type("TEXT")))

#define ODB_OBJECT(C, M, c, m) \
    _Pragma(STRINGIFY(db member(C::M) virtual(*Symbol) access(mutable_ ## M))) \
	ODB_IDX(C, c, m)

#define ODB_OBJECT_2(C, S, c, s) \
	_Pragma(STRINGIFY(db member(C::s) virtual(P ## S) get(S (mutable_ ## s)) set(S (set_allocated_ ## s)) )) \
	_Pragma(STRINGIFY(db index(C) member(s)))

// not tested
#define ODB_VECTOR(c, m) \
    _Pragma(STRINGIFY(db member(c::m ## _) transient)) \
    _Pragma(STRINGIFY(db member(c::m) get(get ## m) set(set ## m))) 

#define ODB_ARRAY(c, m) \
    _Pragma(STRINGIFY(db member(c::m ## _) ))

namespace rcr {
	ODB_TABLE(Operation)
		ODB_STRING(Operation, symbol)
		ODB_STRING(Operation, description)

	ODB_TABLE(Symbol)
		ODB_STRING(Symbol, sym)
		ODB_STRING(Symbol, description)
		ODB_STRING(Symbol, unit)
		ODB_NUMBER(Symbol, pow10, int)

	ODB_TABLE(PropertyType)
		ODB_STRING(PropertyType, key)
		ODB_STRING(PropertyType, description)

	ODB_TABLE(Package)
		ODB_NUMBER(Package, card_id, uint64_t)
		ODB_NUMBER(Package, box, uint64_t)
		ODB_NUMBER(Package, qty, uint64_t)
		#pragma db index(Package) members(card_id)
		#pragma db index(Package) members(box)

	ODB_TABLE(Property)
		ODB_NUMBER(Property, card_id, uint64_t)
		ODB_NUMBER(Property, property_type_id, uint64_t)
		ODB_STRING(Property, value)

	ODB_TABLE(Card)
		ODB_STRING(Card, name)
		ODB_STRING(Card, uname)
		ODB_NUMBER(Card, symbol_id, uint64_t)
		ODB_NUMBER(Card, nominal, uint64_t)

	ODB_TABLE(Box)
		ODB_NUMBER(Box, box_id, uint64_t)
		ODB_STRING(Box, name)
		ODB_STRING(Box, uname)
		#pragma db index(Box) members(box_id)
}
