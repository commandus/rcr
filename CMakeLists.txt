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

set(CMAKE_CXX_STANDARD 11)

option(ENABLE_SQLITE "Build with Sqlite3" ON)
option(ENABLE_PG "Build with PostgreSQL}" OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

set (ARGTABLE "third-party/argtable3/argtable3.c")

if(ENABLE_SQLITE)
	set(DEFS ENABLE_SQLITE)
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
	set(DEFS ENABLE_PG)
	if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
		find_package(unofficial-postgresql CONFIG REQUIRED)
		set(BACKEND_DB_LIB unofficial::PostgreSQL::PostgreSQL)
	else ()
		find_package(PostgreSQL REQUIRED)
		set(BACKEND_DB_LIB ${PostgreSQL_LIBRARIES})
		set(BACKEND_DB_INC ${PostgreSQL_INCLUDE_DIRS})
	endif ()
endif ()

set(GEN_SRC gen/rcr.grpc.pb.cc gen/rcr.pb.cc gen/rcr.pb-odb.cxx)

include_directories("third-party" "odb" ".")

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	# avoid Error LNK2038 mismatch detected for 'RuntimeLibrary': value 'MT_StaticRelease' doesn't match value 'MD_DynamicRelease'
	# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
	set (OS_SPECIFIC_LIBS wsock32 ws2_32 Userenv)
endif()

#
# mkdb
#
set(MKDB_SRC 
	cli/mkdb.cpp
	${GEN_SRC}
	${ARGTABLE}
)

add_executable(mkdb ${MKDB_SRC})
target_link_libraries(mkdb PRIVATE ${BACKEND_DB_LIB} ${OS_SPECIFIC_LIBS})
target_compile_definitions(mkdb PRIVATE ${DEFS} ${BACKEND_DB_INC})

#
# Generate Protobuf and GRPC code
#
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_custom_command(TARGET mkdb PRE_BUILD COMMAND tools/generate-code.ps1)
else()
	add_custom_command(TARGET mkdb PRE_BUILD COMMAND tools/generate-code)
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
