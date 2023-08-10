#ifdef _MSC_VER
#define	SYSLOG(msg)
#define OPEN_SYSLOG(NAME)
#define CLOSE_SYSLOG()
#else
#include <syslog.h>
#include <sstream>
#define	SYSLOG(level, msg) { syslog (level, "%s", msg); }
#define OPEN_SYSLOG(NAME) { setlogmask (LOG_UPTO(LOG_NOTICE)); openlog(NAME, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1); }
#define CLOSE_SYSLOG() closelog();
#endif
