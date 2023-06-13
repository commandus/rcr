/*
 * @file rcr-ws.h
 * -DENABLE_HTTP=on
 */

#ifndef RCR_WS_H_
#define RCR_WS_H_	1

#include <map>
#include <string>

#include "log-intf.h"
#include "svc/svcImpl.h"

#define MHD_START_FLAGS 	MHD_USE_POLL | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_SUPPRESS_DATE_NO_CLOCK | MHD_USE_TCP_FASTOPEN | MHD_USE_TURBO

#define NUMBER_OF_THREADS CPU_COUNT

/**
 * Configuration to start up web service
 */
typedef struct {
	unsigned int threadCount;
	unsigned int connectionLimit;
	unsigned int flags;

	// listener port
	int port;
	// last error code
	int lasterr;
	// html root
	const char* dirRoot;
	// log verbosity
	int verbosity;
	// web server descriptor
	void *descriptor;
	
    LogIntf *onLog;
    RcrImpl *svc;
} WSConfig;

/**
 * @param threadCount threads count, e.g. 2
 * @param connectionLimit mex connection limit, e.g. 1000
 * @param flags e.g. MHD_SUPPRESS_DATE_NO_CLOCK | MHD_USE_DEBUG | MHD_USE_SELECT_INTERNALLY
 * @param config listener descriptors, port number
 */ 
bool startWS(
	WSConfig &config
);

void doneWS(
	WSConfig &config
);

#endif
