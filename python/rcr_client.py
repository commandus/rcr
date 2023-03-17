from __future__ import print_function

import logging

import grpc
import rcr_pb2
import rcr_pb2_grpc


def run():
    # NOTE(gRPC Python Team): .close() is possible on a channel and should be
    # used in circumstances in which the with statement does not fit the needs
    # of the code.
    print("Will try to rcr ...")
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = rcr_pb2_grpc.RcrStub(channel)
        response = stub.version(rcr_pb2.VersionRequest(value=1))
    print("Version received: " + response.name)


if __name__ == '__main__':
    logging.basicConfig()
    run()
