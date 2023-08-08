#
# Dependencies: Gettext Intl ICU Xlnt Protobuf gRPC
#               unofficial-sqlite3/SQLite3(Windows) or unofficial-postgresql/PostgreSQL(Windows)
# Options:
# -DENABLE_SQLITE=on   	enable Sqlite3
# -DENABLE_PG=on 		enable PostgreSQL
# -DENABLE_HTTP=on 		enable embedded HTTP server
#

set(VCPKG_INC "/git/vcpkg/installed/x64-windows/include")

#
# -------------------- Do not edit below this line --------------------
#

cmake_minimum_required(VERSION 3.9)
set(PROJECT_NAME "rcr")
project(${PROJECT_NAME} VERSION 1.0)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

#
# Setting up i18l & l10n
# see https://stackoverflow.com/questions/10521635/using-intltool-with-cmake
#
find_package(Gettext REQUIRED)
find_package(Intl REQUIRED)

if (CMAKE_VERSION VERSION_GREATER "3.20.0")
set(LIBINTL Intl::Intl)
else()
set(LIBINTL Intl::Intl)
endif()

include_directories(${Intl_INCLUDE_DIRS})
link_directories(${Intl_LIBRARY_DIRS})
find_program(GETTEXT_MSGFMT_EXECUTABLE msgfmt)
IF(NOT GETTEXT_MSGFMT_EXECUTABLE)
	MESSAGE("NOTE: msgfmt not found. Translations will *not* be installed")
ELSE()
	# Build the .po files
	SET(PO_PATH ${CMAKE_CURRENT_SOURCE_DIR}/po)
	# Input files
	FILE(GLOB PO_FILES ${PO_PATH}/*.po)
	# Generated file list
	SET(MO_FILES)
	SET(PRG rcr-cli)
	SET(LOCALE_INSTALL_DIR /usr/share/locale)
	FOREACH(_poFile ${PO_FILES})
		GET_FILENAME_COMPONENT(_poFileName ${_poFile} NAME)
		STRING(REGEX REPLACE "^${PRG}\\." "" _langCode ${_poFileName} )
		STRING(REGEX REPLACE "\\.UTF-8\\.po$" "" _langCode ${_langCode} )
		STRING(REGEX REPLACE "_..$" "" _langCode ${_langCode} )
		IF( _langCode )
			MESSAGE("** PO file ${_poFileName} LANG ${_langCode} LIBDIR ${INTL_LIBRARY_DIRS} INCDIR ${INTL_INCLUDE_DIRS}")
			GET_FILENAME_COMPONENT(_lang ${_poFile} NAME_WE)

			SET(_moPath ${CMAKE_CURRENT_SOURCE_DIR}/locale/${_langCode}/LC_MESSAGES)
			SET(_moFile ${_moPath}/${PRG}.mo)
			MESSAGE("** ** ${_moFile} install to ${LOCALE_INSTALL_DIR}/${_langCode}/LC_MESSAGES/${PRG}.mo")
#	mkdir -p ${_moPath} ; ${GETTEXT_MSGFMT_EXECUTABLE} -o ${_moFile} ${_poFile}
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	MESSAGE("** powershell ${CMAKE_CURRENT_SOURCE_DIR}/tools/mk_mo.ps1 ${GETTEXT_MSGFMT_EXECUTABLE} ${_moPath} ${_moFile} ${_poFile}")
	ADD_CUSTOM_COMMAND(OUTPUT ${_moFile}
		COMMAND powershell ${CMAKE_CURRENT_SOURCE_DIR}/tools/mk_mo.ps1 ${GETTEXT_MSGFMT_EXECUTABLE} ${_moPath} ${_moFile} ${_poFile}
		DEPENDS ${_poFile})
else()
	MESSAGE("** ${CMAKE_CURRENT_SOURCE_DIR}/tools/mk_mo.sh ${_moPath} ${_moFile} ${_poFile}")
	ADD_CUSTOM_COMMAND(OUTPUT ${_moFile}
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/mk_mo.sh ${_moPath} ${_moFile} ${_poFile}
		DEPENDS ${_poFile})
endif()
#			INSTALL(FILES ${_moFile} DESTINATION ${LOCALE_INSTALL_DIR}/${_langCode}/LC_MESSAGES/ RENAME ${PRG}.mo)
			LIST(APPEND MO_FILES ${_moFile})
		ENDIF()
	ENDFOREACH(_poFile ${PO_FILES})
	ADD_CUSTOM_TARGET(translations ALL DEPENDS ${MO_FILES})
ENDIF()

#
# Unicode toUpper()
#
find_package(ICU 61.0 COMPONENTS uc i18n REQUIRED)
set(ICU_LIBS ICU::uc ICU::i18n)

#
# xlsx load
#
find_package(Xlnt CONFIG REQUIRED)
set(XLSX_LIBS xlnt::xlnt)

#
# ODB requires 11, 14?, not compile 17 (on Windows)
# gRPC 1.24 requires 11, gRPC 1.53 requires 17
# 
set(CMAKE_CXX_STANDARD 14)

option(ENABLE_SQLITE "Build with Sqlite3" ON)
option(ENABLE_PG "Build with PostgreSQL}" OFF)
option(ENABLE_HTTP "Build with HTTP JSON server" ON)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

set(GRPC_LIBS gRPC::grpc++)

if(ENABLE_SQLITE)
	find_package(ODB REQUIRED OPTIONAL_COMPONENTS sqlite)
endif()

if(ENABLE_PG)
	find_package(ODB REQUIRED OPTIONAL_COMPONENTS pg)
endif()

if(ENABLE_HTTP)
	find_library(LIBMICROHTTPD NAMES microhttpd libmicrohttpd-dll.lib HINTS /usr/lib/x86_64-linux-gnu/)
	set(SVC_EXTRA_SRC rcr-ws.cpp)
	set(SVC_EXTRA_LIB ${LIBMICROHTTPD})
	set(SVC_EXTRA_DEFS ENABLE_HTTP)
endif()

set (ARGTABLE "third-party/argtable3/argtable3.c")

if(ENABLE_SQLITE)
	set(ODB_DATABASE "sqlite")
	set(DEFS "ENABLE_SQLITE")
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
		find_package(unofficial-sqlite3 CONFIG REQUIRED)
		set(BACKEND_DB_LIB unofficial::sqlite3::sqlite3)
	else ()
		find_package(SQLite3 REQUIRED)
		set(BACKEND_DB_LIB ${SQLite3_LIBRARIES})
		set(BACKEND_DB_INC ${SQLite3_INCLUDE_DIRS})
	endif ()
endif ()

if(ENABLE_PG)
	set(ODB_DATABASE "pgsql")
	set(DEFS "ENABLE_PG")
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
		find_package(unofficial-postgresql CONFIG REQUIRED)
		set(BACKEND_DB_LIB unofficial::PostgreSQL::PostgreSQL)
	else()
		find_package(PostgreSQL REQUIRED)
		set(BACKEND_DB_LIB ${PostgreSQL_LIBRARIES})
		set(BACKEND_DB_INC ${PostgreSQL_INCLUDE_DIRS})
	endif()
endif()

# set(DEFS ${DEFS} gRPC_SSL_PROVIDER:STRING=package)
	
set(GEN ${CMAKE_CURRENT_SOURCE_DIR}/gen)
set(GEN_GRPC_SRC ${GEN}/rcr.grpc.pb.cc ${GEN}/rcr.pb.cc)
set(GEN_SRC ${GEN_GRPC_SRC} ${GEN}/rcr.pb-odb.cxx)
set(GEN_HDR ${GEN}/rcr.grpc.pb.h ${GEN}/rcr.pb.h ${GEN}/rcr.pb-odb.hxx)

include_directories("third-party" "odb" "." "gtests/lib" ${VCPKG_INC})

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# avoid Error LNK2038 mismatch detected for 'RuntimeLibrary': value 'MT_StaticRelease' doesn't match value 'MD_DynamicRelease'
	# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
	# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
	set (OS_SPECIFIC_LIBS wsock32 ws2_32 Userenv)
endif()

#
# librcr
#
set(RCR_LIB_SRC
	${GEN_GRPC_SRC}
	third-party/utilfile.cpp third-party/utilstring.cpp
	string-helper.cpp RcrCredentials.cpp RCQuery.cpp RCQueryProcessor.cpp MeasureUnit.cpp StockOperation.cpp QueryProperties.cpp
	AppSettings.cpp SpreadSheetHelper.cpp BoxName.cpp
)
add_library(rcr STATIC ${RCR_LIB_SRC})
target_link_libraries(rcr PRIVATE ${OS_SPECIFIC_LIBS} gRPC::grpc++ ${Protobuf_LIBRARIES} ${ICU_LIBS} ${XLSX_LIBS}
	${SSL_LIBS} ${LIBINTL}
)
target_compile_definitions(rcr PRIVATE ${DEFS})
target_include_directories(rcr PRIVATE ${BACKEND_DB_INC} ${Protobuf_INCLUDE_DIRS} ${ODB_LIBODB_INCLUDE_DIRS} )

#
# mkdb
#
set(MKDB_SRC 
	cli/mkdb.cpp
	${GEN_SRC}
	${ARGTABLE}
)
add_executable(mkdb ${MKDB_SRC})
target_link_libraries(mkdb PRIVATE ${BACKEND_DB_LIB} ${OS_SPECIFIC_LIBS} ${Protobuf_LIBRARIES}
	${ICU_LIBS} ${ODB_LIBODB_LIBRARIES} ${ODB_SQLITE_LIBRARIES} ${GRPC_LIBS} rcr
)
target_compile_definitions(mkdb PRIVATE ${DEFS})
target_include_directories(mkdb PRIVATE ${BACKEND_DB_INC} ${Protobuf_INCLUDE_DIRS} ${ODB_LIBODB_INCLUDE_DIRS} )

#
# box
#
add_executable(box cli/box.cpp	${ARGTABLE}	StockOperation.cpp)
target_link_libraries(box PRIVATE ${ICU_LIBS} ${LIBINTL})

#
# rcr-svc
#
add_executable(rcr-svc svc/daemonize.cpp svc/passphrase.cpp svc/SSLValidator.cpp svc/svcconfig.cpp svc/svc.cpp svc/svcImpl.cpp
	${GEN_SRC} ${ARGTABLE} ${SVC_EXTRA_SRC} gen/odb-views-odb.cxx
)
target_link_libraries(rcr-svc PRIVATE
	rcr ${BACKEND_DB_LIB} ${OS_SPECIFIC_LIBS} ${ICU_LIBS} ${ODB_LIBODB_LIBRARIES} ${ODB_SQLITE_LIBRARIES}
	gRPC::grpc++ ${Protobuf_LIBRARIES} ${XLSX_LIBS}	${SSL_LIBS} ${SVC_EXTRA_LIB}
)
target_compile_definitions(rcr-svc PRIVATE ${DEFS} ${SVC_EXTRA_DEFS})
target_include_directories(rcr-svc PRIVATE ${BACKEND_DB_INC} ${Protobuf_INCLUDE_DIRS} ${ODB_LIBODB_INCLUDE_DIRS} )

#
# rcr-cli
#
set(RCR_CLI_SRC
	cli/grpcClient.cpp cli/rcr-cli.cpp
	third-party/config-filename.cpp
	${GEN_GRPC_SRC} ${ARGTABLE}
)
add_executable(rcr-cli ${RCR_CLI_SRC})
target_link_libraries(rcr-cli PRIVATE ${OS_SPECIFIC_LIBS} rcr gRPC::grpc++ ${Protobuf_LIBRARIES} ${ICU_LIBS} ${XLSX_LIBS}
	${SSL_LIBS} ${LIBINTL}
)
target_compile_definitions(rcr-cli PRIVATE ${DEFS})
target_include_directories(rcr-cli PRIVATE ${BACKEND_DB_INC} ${Protobuf_INCLUDE_DIRS} ${ODB_LIBODB_INCLUDE_DIRS} rcr )

#if(WIN32)
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# avoid Error LNK2038 mismatch detected for 'RuntimeLibrary': value 'MT_StaticRelease' doesn't match value 'MD_DynamicRelease'
	# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

	if(ENABLE_MQTT)
		set (OS_SPECIFIC_LIBS wsock32 ws2_32 Userenv iphlpapi)
	else()
		set (OS_SPECIFIC_LIBS wsock32 ws2_32 Userenv)
	endif()
endif()

set (LNS_DEF ${LNS_DEF} )
if (ENABLE_LISTENER_UDP)
	set (LNS_DEF ${LNS_DEF} ENABLE_LISTENER_UDP )
endif()
if (ENABLE_LISTENER_EMBEDDED)
	set (LNS_DEF ${LNS_DEF} ENABLE_LISTENER_EMBEDDED )
endif()

#
# gtest gtest_main
#

#
# Generate Protobuf and GRPC code
#
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_custom_command(
		OUTPUT ${GEN_SRC} ${GEN_HDR}
		COMMAND powershell ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate-code.ps1 ${CMAKE_CURRENT_SOURCE_DIR} ${ODB_DATABASE}
	)
else()
	add_custom_command(
		OUTPUT ${GEN_SRC} ${GEN_HDR}
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate-code ${CMAKE_CURRENT_SOURCE_DIR}
	)
endif()

#
# Strip binary for release builds
#
add_custom_command(TARGET mkdb POST_BUILD
	COMMAND "$<$<CONFIG:release>:${CMAKE_STRIP}>" mkdb
)

INSTALL(FILES README.md LICENSE HISTORY COPYING TODO CODE_OF_CONDUCT.md CONTRIBUTING.md DESTINATION doc)

include(InstallRequiredSystemLibraries)

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
include(CPack)

#
# Tests
#
if (NOT CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_subdirectory(gtests)
endif()

if(ENABLE_SQLITE)
	message("Sqlite enabled")
endif ()

if(ENABLE_PG)
	message("PostgreSQL enabled")
endif ()

if(ENABLE_HTTP)
	message("Embedded HTTP JSON server enabled")
endif ()
