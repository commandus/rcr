#
# Options:
# -DENABLE_SQLITE=on   	enable Sqlite3
# -DENABLE_PG=on 		enable PostgreSQL
#

set(Protobuf_DIR "~/src-old/third_party/protobuf")
set(gRPC_DIR "third-party/grpc-1.24.3")

#
# -------------------- Do not edit below this line --------------------
#

cmake_minimum_required(VERSION 3.9)
set(PROJECT_NAME "rcr")
project(${PROJECT_NAME} VERSION 1.0)

set(MQTTAPP_CMAKE_DIR ${CMAKE_SOURCE_DIR}/cmake)
set(CMAKE_MODULE_PATH ${MQTTAPP_CMAKE_DIR} ${CMAKE_MODULE_PATH})

#
# ODB requires 11, 14?, not compile 17
# 
set(CMAKE_CXX_STANDARD 11)

option(ENABLE_SQLITE "Build with Sqlite3" ON)
option(ENABLE_PG "Build with PostgreSQL}" OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

if(ENABLE_SQLITE)
	find_package(ODB REQUIRED OPTIONAL_COMPONENTS sqlite)
endif()

if(ENABLE_PG)
	find_package(ODB REQUIRED OPTIONAL_COMPONENTS pg)
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
	else ()
		find_package(PostgreSQL REQUIRED)
		set(BACKEND_DB_LIB ${PostgreSQL_LIBRARIES})
		set(BACKEND_DB_INC ${PostgreSQL_INCLUDE_DIRS})
	endif ()
endif ()

set(GEN ${CMAKE_CURRENT_SOURCE_DIR}/gen)
set(GEN_GRPC_SRC ${GEN}/rcr.grpc.pb.cc ${GEN}/rcr.pb.cc)
set(GEN_SRC ${GEN_GRPC_SRC} ${GEN}/rcr.pb-odb.cxx)
set(GEN_HDR ${GEN}/rcr.grpc.pb.h ${GEN}/rcr.pb.h ${GEN}/rcr.pb-odb.hxx)

include_directories("third-party" "odb" "." "gtests/lib")

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# avoid Error LNK2038 mismatch detected for 'RuntimeLibrary': value 'MT_StaticRelease' doesn't match value 'MD_DynamicRelease'
	# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
	# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
	set (OS_SPECIFIC_LIBS wsock32 ws2_32 Userenv)
endif()

#
# mkdb
#
set(MKDB_SRC 
	cli/mkdb.cpp
	${GEN_SRC}
	${ARGTABLE}
	MeasureUnit.cpp
)
add_executable(mkdb ${MKDB_SRC})
target_link_libraries(mkdb PRIVATE ${BACKEND_DB_LIB} ${OS_SPECIFIC_LIBS} ${Protobuf_LIBRARIES} 
	${ODB_LIBODB_LIBRARIES} ${ODB_SQLITE_LIBRARIES} gRPC::grpc++
)
target_compile_definitions(mkdb PRIVATE ${DEFS})
target_include_directories(mkdb PRIVATE ${BACKEND_DB_INC} ${Protobuf_INCLUDE_DIRS} ${ODB_LIBODB_INCLUDE_DIRS} )

#
# Download lexy
#
include(FetchContent)
FetchContent_Declare(lexy URL https://lexy.foonathan.net/download/lexy-src.zip DOWNLOAD_EXTRACT_TIMESTAMP true)
FetchContent_MakeAvailable(lexy)

#
# rcrq utility depends on rvrql
#
set(RCRQ_SRC 
	rcrql/rcrq.cpp
	${GEN_GRPC_SRC}
	${ARGTABLE}
)
add_executable(rcrq 
	${RCRQ_SRC}
)
target_compile_definitions(rcrq PRIVATE ${DEFS})
target_compile_features(rcrq PRIVATE cxx_std_11)
target_link_libraries(rcrq PRIVATE 
	rcrql
	${OS_SPECIFIC_LIBS}
	${Protobuf_LIBRARIES} gRPC::grpc++
)

#
# rcrql library depends on lexy
#
set(LIB_RCRQL_SRC 
	rcrql/rcrql.cpp
	MeasureUnit.cpp
)
add_library(rcrql STATIC
	${LIB_RCRQL_SRC}
)
# -fPIC
set_property(TARGET rcrql PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(rcrql PRIVATE ${DEFS})
target_compile_features(rcrql PRIVATE cxx_std_17)
target_link_libraries(rcrql PRIVATE foonathan::lexy)

#
# Just for test
#
add_executable(rcrql-example
	rcrql/rcrql-example.cpp
)
target_compile_features(rcrql-example PRIVATE cxx_std_17)
target_link_libraries(rcrql-example PRIVATE foonathan::lexy)

#
# rcr-cli
#
set(RCR_CLI_SRC
	cli/rcr-cli.cpp
	${GEN_GRPC_SRC}
	${ARGTABLE}
	cli/grpcClient.cpp
	RcrCredentials.cpp
	MeasureUnit.cpp
		RCQuery.cpp RCQuery.h)
add_executable(rcr-cli ${RCR_CLI_SRC})
target_link_libraries(rcr-cli PRIVATE  ${OS_SPECIFIC_LIBS}
	${Protobuf_LIBRARIES} gRPC::grpc++
	gtest gtest_main
)
target_compile_definitions(rcr-cli PRIVATE ${DEFS})
target_include_directories(rcr-cli PRIVATE ${Protobuf_INCLUDE_DIRS} )

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
