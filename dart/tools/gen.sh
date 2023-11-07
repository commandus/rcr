#!/bin/sh
mkdir -p lib/src/generated
# Generate gRPC code
protoc --dart_out=grpc:lib/src/generated -I../proto ../proto/rcr.proto
# get dependencies
dart pub get
echo "Run example:\n"
echo "dart lib/src/client.dart"
exit 0
