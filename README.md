# Mysterious geological project

## Quick start

Install dependencies and tool first (see section Prerequisites).

Check proto/mgp.proto file.

Determine which classes are persistent in the odb/pragmas.pgsql.hxx.

List classes and members persistent in the relational database in the odb/pragmas.pgsql.hxx file
e.g.

```
	ODB_TABLE(Person)
		ODB_STRING(Person, first_name)
		ODB_STRING(Person, last_name)
      ...
```

Then call ./tools/generate-code.ps1

./tools/generate-code.ps1 do:

- generate serialziation & protocol support classes
- generate object relation mapping (ORM) classes

### Serialzation & protocol support classes

Generate in the gen/ subdirectory protobuf serialization classes  (.pb.cc, .pb.h),
gRPC protocol classes (.grpc.pb.cc, .pb.cc, .pb.h.pb.h) usinbg protoc compiler and 
protoc's grpc c++ plugin.

Object relation mapping (ORM)

Generate in the gen/ subdirectory ODB ORM mapping code(.pb-odb.cxx, .pb-odb.ixx, .pb-odb.hxx),
gRPC protocol classes (.grpc.pb.cc, .pb.cc, .pb.h.pb.h) usinbg protoc compiler and 
protoc's grpc c++ plugin.

by the proto/mgp.proto 

## Tools

- .\tools\generate-code.ps1 - Generate
- .\tools\clean-code.ps1 - Clean

## Prerequisites

### Development tools

- Visual Studio 2022 17.4.5 (Visual C++)
- vcpkg 2021-12-09 (package manager)

### Preinstalled dependencies

Libraries:

- odb (ORM)
- grpc (client-server protocol)
- protobuf (serialization) 

Compilers:

- odb (2.4.0)
- protoc (libprotoc 3.18.0)

must be in the folder in the %PATH%

Copy protoc.exe, grpc_cpp_plugin.exe and *.dll from C:\git\vcpkg\installed\x64-windows-static\tools\protobuf, C:\git\vcpkg\buildtrees\grpc\x64-windows-rel
to C:\bin

Download https://www.codesynthesis.com/download/odb/2.4/odb-2.4.0-i686-windows.zip

Unzip odb-2.4.0-i686-windows.zip to c:\p\odb (for example)

Add C:\p\odb\bin to %PATH%


#### Install odb

```
vcpkg install libodb:x64-windows-static
```

https://www.codesynthesis.com/download/odb/2.4/libodb-2.4.0.zip

PostgreSQL backend

- libodb-pgsql (with libpq)

Install:

```
vcpkg install libodb-pgsql:x64-windows-static
```

SQLite backend

- libodb-sqlite

```
vcpkg install libodb-sqlite:x64-windows-static
```

or download from https://www.codesynthesis.com/download/odb/2.4/libodb-pgsql-2.4.0.zip
upgrade solution from 10 to 14 and build release

#### Install grpc & protobuf

```
vcpkg install grpc:x64-windows-static protobuf:x64-windows-static
```

#### Install all

```
vcpkg install libodb:x64-windows-static libodb-pgsql:x64-windows-static libodb-sqlite:x64-windows-static sqlite3:x64-windows-static grpc:x64-windows-static
```

find_package(unofficial-sqlite3 CONFIG REQUIRED)
target_link_libraries(main PRIVATE unofficial::sqlite3::sqlite3)

find_package(odb CONFIG REQUIRED)
target_link_libraries(main PRIVATE odb::libodb)

find_package(odb CONFIG REQUIRED)
target_link_libraries(main PRIVATE odb::libodb-pgsql)

find_package(odb CONFIG REQUIRED)
target_link_libraries(main PRIVATE odb::libodb-sqlite)

find_package(gRPC CONFIG REQUIRED)
target_link_libraries(main PRIVATE gRPC::gpr gRPC::upb gRPC::grpc gRPC::grpc++)

find_package(modules CONFIG REQUIRED)
target_link_libraries(main PRIVATE re2::re2 c-ares::cares)

find_package(protobuf CONFIG REQUIRED)
target_link_libraries(main PRIVATE protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite)


C:\git\vcpkg\installed\x64-windows-static\tools\protobuf\protoc
C:\git\vcpkg\installed\x64-windows-static\lib\ C:\git\vcpkg\installed\x64-windows-static\lib\ 

## Generate

Generate files:

- mgp.grpc.pb.cc
- mgp.grpc.pb.h
- mgp.pb-odb.cxx
- mgp.pb-odb.hxx
- mgp.pb-odb.ixx
- mgp.pb.cc
- mgp.pb.h
- mgp.pb.sql

In the PowerShell execute:
```
.\tools\generate-code.ps1
```

If errors occured see Bugs section.

## Clean files

In the PowerShell execute:
```
.\tools\clean-code.ps1
```


## Bugs

### gcc

odb exit with gcc error :

```
C:\git\vcpkg\packages\protobuf_x64-windows-static\include/google/protobuf/stubs/mutex.h:124:29: error: temporary of non-literal type 'google::protobuf::internal::CallOnceInitializedMutex<std::mutex>' in a constant expression
   constexpr WrappedMutex() {}
```

(StackOverflow explanation)[https://stackoverflow.com/questions/69232278/c-protocol-buffer-temporary-of-non-literal-type-googleprotobufinternal]


Solution

Remove (comment) line 124 in the C:\git\vcpkg\packages\protobuf_x64-windows-static\include/google/protobuf/stubs/mutex.h


### odb

To see error description, change console code page:
```
chcp 1251
```

