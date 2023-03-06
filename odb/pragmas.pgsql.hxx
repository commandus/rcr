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

#define ODB_VALUE(C) \
	_Pragma(STRINGIFY(db value(C))) \
	_Pragma(STRINGIFY(db member(C::_internal_metadata_) transient)) \
	_Pragma(STRINGIFY(db member(C::_cached_size_) transient))

#define ODB_TABLE(C) \
	_Pragma(STRINGIFY(db object(C))) \
	_Pragma(STRINGIFY(db member(C::id_) get(id) set(set_id) id auto)) \
	_Pragma(STRINGIFY(db member(C::_internal_metadata_) transient)) \
	_Pragma(STRINGIFY(db member(C::_cached_size_) transient))

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

#define ODB_NUMBER(c, m) \
    _Pragma(STRINGIFY(db member(c::m ## _) get(m) set(set_ ## m)))

#define ODB_BOOL(c, m) \
    ODB_NUMBER(c, m)

// Protobuf since version XX changed call set_ to _internal_set_
#define ODB_STRING(c, m) \
    _Pragma(STRINGIFY(db member(c::m ## _) transient)) \
    _Pragma(STRINGIFY(db member(c::m) virtual(std::string) get(m) set(_internal_set_ ## m)))

// big text or blob
// Protobuf since version XX changed call set_ to _internal_set_
#define ODB_TEXT(c, m) \
    _Pragma(STRINGIFY(db member(c::m ## _) transient)) \
    _Pragma(STRINGIFY(db member(c::m) virtual(std::string) get(m) set(_internal_set_ ## m) type("TEXT")))

#define ODB_OBJECT(C, M, c, m) \
    _Pragma(STRINGIFY(db member(C::M ## _) access(M ## _))) \
	ODB_IDX(C, c, m)

// not tested
#define ODB_VECTOR(c, m) \
    _Pragma(STRINGIFY(db member(c::m ## _) transient)) \
    _Pragma(STRINGIFY(db member(c::m) get(get ## m) set(set ## m))) 

#define ODB_ARRAY(c, m) \
    _Pragma(STRINGIFY(db member(c::m ## _) ))


namespace rcr
{
	ODB_TABLE(Operation)
		ODB_STRING(Operation, symbol)
		ODB_STRING(Operation, description)

	ODB_TABLE(Symbol)
		ODB_STRING(Symbol, sym)
		ODB_STRING(Symbol, unit)
		ODB_NUMBER(Symbol, pow10)

	ODB_TABLE(Component)
		ODB_OBJECT(Component, symbol, component, symbol)
		ODB_STRING(Component, name)

	ODB_TABLE(PropertyType)
		ODB_STRING(PropertyType, key)
		ODB_STRING(PropertyType, description)

	ODB_TABLE(Property)
		ODB_OBJECT(Property, typ, property, typ)
		ODB_STRING(Property, value)

	ODB_TABLE(Card)
		ODB_STRING(Card, name)
		ODB_OBJECT(Card, component, card, component)
		ODB_NUMBER(Card, nominal)
		ODB_ARRAY(Card, properties)
		ODB_ARRAY(Card, packages)

	ODB_TABLE(Package)
		ODB_OBJECT(Package, card, package, card)
		ODB_NUMBER(Package, boxes)
		ODB_NUMBER(Package, qty)

}
