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


namespace mgp
{
	// Credentials
	ODB_TABLE(Credentials)
		ODB_NUMBER(Credentials, start)
		ODB_NUMBER(Credentials, finish)
		ODB_NUMBER(Credentials, lastlogin)
		ODB_STRING(Credentials, mk)
		ODB_STRING(Credentials, mv)

	// User
	ODB_TABLE(User)
		ODB_STRING(User, cn)
		ODB_OBJECT(User, credentials, user, credentials)
		ODB_NUMBER(User, role)

	// MediaFile
	ODB_VALUE(MediaFile)
		ODB_STRING(MediaFile, content_type)
		ODB_STRING(MediaFile, uri)
		ODB_NUMBER(MediaFile, content_length)
		ODB_TEXT(MediaFile, data)
		ODB_NUMBER(MediaFile, tag)

	// Person
	ODB_TABLE(Person)
		ODB_STRING(Person, first_name)
		ODB_STRING(Person, last_name)
		ODB_STRING(Person, middle_name)
		ODB_STRING(Person, prefix)
		ODB_NUMBER(Person, birthday)
		ODB_STRING(Person, document_type)
		ODB_STRING(Person, document)
		ODB_STRING(Person, phone_mobile)
		ODB_STRING(Person, email)
		ODB_NUMBER(Person, gender)
		ODB_NUMBER(Person, tag)
		ODB_ARRAY(Person, medias)

	// Employee
	ODB_TABLE(Employee)
		ODB_NUMBER(Employee, role_number)
		ODB_STRING(Employee, name)
		ODB_OBJECT(Employee, person, employee, person)
		ODB_OBJECT(Employee, user, employee, user)
		ODB_ARRAY(Employee, medias)

	// Org
	ODB_TABLE(Org)
		ODB_STRING(Org, name)
		ODB_OBJECT(Org, manager, org, manager)
		ODB_ARRAY(Org, medias)

	// Inventory
	ODB_TABLE(Inventory)
		ODB_OBJECT(Inventory, org, inventory, org)
		ODB_NUMBER(Inventory, inventory_number)
		ODB_STRING(Inventory, name)
		ODB_ARRAY(Inventory, medias)
}
