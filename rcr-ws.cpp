#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <sstream>
#include <algorithm>
#include <functional>

#include "platform.h"

#include <sys/stat.h>
#include <microhttpd.h>

// Caution: version may be different, if microhttpd dependecy not compiled, revise version humber
#if MHD_VERSION <= 0x00096600
#define MHD_Result int
#endif
#ifndef MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_CREDENTIALS
#define MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_CREDENTIALS "Access-Control-Allow-Credentials"
#endif
#ifndef MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_METHODS
#define MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_METHODS "Access-Control-Allow-Methods"
#endif
#ifndef MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_HEADERS
#define MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_HEADERS "Access-Control-Allow-Headers"
#endif

#define	LOG_ERR								3
#define	LOG_INFO							5

#define MODULE_WS	200

#include "rcr-ws.h"

static LogIntf *logCB = nullptr;

#define PATH_COUNT 17

typedef enum {
    RT_LOGIN = 0,
    RT_GETDICTIONARIES = 1,
    RT_GETSETTINGS = 2,
    RT_SETSETTINGS = 3,
    RT_CHPROPERTYTYPE = 4,
    RT_GETCARD = 5,
    RT_CHCARD = 6,
    RT_CHBOX = 7,
    RT_CARDQUERY = 8,
    // RT_CARDPUSH,
    RT_GETBOX = 9,
    RT_LSUSER = 10,
    RT_CHUSER = 11,
    RT_CHGROUP = 12,
    RT_CHGROUPUSER = 13,
    RT_IMPORTEXCEL = 14,
    RT_EXPORTEXCEL = 15,
    RT_LJJOURNAL = 16,
    RT_UNKNOWN = 100	//< FILE params
} RequestType;

class RequestContext {
public:
    RequestType requestType;
    std::string postData;
};

static const char *paths[PATH_COUNT] = {
    "/login",
    "/getDictionaries",
    "/getSettings",
    "/setSettings",
    "/chPropertyType",
    "/getCard",
    "/chCard",
    "/chBox",
    "/cardQuery",
    // "/cardPush",
    "/getBox",
    "/lsUser",
    "/chUser",
    "/chGroup",
    "/chGroupUser",
    "/importExcel",
    "/exportExcel",
    "/lsJournal"
};

const static char *CE_GZIP = "gzip";
const static char *CT_HTML = "text/html;charset=UTF-8";
const static char *CT_JSON = "text/javascript;charset=UTF-8";
const static char *CT_KML = "application/vnd.google-earth.kml+xml";
const static char *CT_PNG = "image/png";
const static char *CT_JPEG = "image/jpeg";
const static char *CT_CSS = "text/css";
const static char *CT_TEXT = "text/plain;charset=UTF-8";
const static char *CT_TTF = "font/ttf";
const static char *CT_BIN = "application/octet";
const static char *HDR_CORS_ORIGIN = "*";
const static char *HDR_CORS_CREDENTIALS = "true";
const static char *HDR_CORS_METHODS = "GET,HEAD,OPTIONS,POST,PUT,DELETE";
const static char *HDR_CORS_HEADERS = "Authorization, Access-Control-Allow-Headers, "
    "Origin, Accept, X-Requested-With, Content-Type, Access-Control-Request-Method, Access-Control-Request-Headers";

typedef enum {
	START_FETCH_JSON_OK = 0,
	START_FETCH_FILE = 1
} START_FETCH_DB_RESULT;

const static char *MSG_HTTP_ERROR = "Error";
const static char *MSG404 = "404 not found";
const static char *MSG401 = "Unauthorized";
const static char *MSG501 = "Not immplemented";

const static char *MSG500[5] = {
	"",                                     // 0
	"Database connection not established",  // 1
	"SQL statement preparation failed",     // 2
	"Required parameter missed",            // 3
	"Binding parameter failed"              // 4
};

#ifdef _MSC_VER
#pragma warning(disable: 4996)
#endif

google::protobuf::util::JsonParseOptions jsonParseOptions;
google::protobuf::util::JsonPrintOptions jsonPrintOptions;

static RequestType parseRequestType(const char *url)
{
	int i;
	for (i = 0; i < PATH_COUNT; i++) {
		if (strcmp(paths[i], url) == 0)
			return (RequestType) i;
	}
	return RT_UNKNOWN;
}

const char *requestTypeString(RequestType value)
{
	if (value == RT_UNKNOWN)
		return "Unknown";
	if ((value >= 0) && (value < PATH_COUNT))
		return paths[(int) value];
	else
		return "???";
}

void *uri_logger_callback(void *cls, const char *uri)
{
	/*
	if (logCB) {
		std::stringstream ss;
		ss << "URI: " << uri;
		logCB->logMessage(cls, LOG_INFO, MODULE_WS, 0, ss.str());
	}
	*/
	return nullptr;
}

const char *NULLSTR = "";

static ssize_t file_reader_callback(void *cls, uint64_t pos, char *buf, size_t max)
{
	FILE *file = (FILE *) cls;
	(void) fseek (file, (long) pos, SEEK_SET);
	return fread (buf, 1, max, file);
}

static void free_file_reader_callback(void *cls)
{
	fclose ((FILE *) cls);
}

static const char *mimeTypeByFileExtension(const std::string &filename)
{
	std::string ext = filename.substr(filename.find_last_of('.') + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	if (ext == "html")
		return CT_HTML;
	else
		if (ext == "htm")
			return CT_HTML;
	else
		if (ext == "js")
			return CT_JSON;
	else
		if (ext == "css")
			return CT_CSS;
	if (ext == "png")
		return CT_PNG;
	else
		if (ext == "jpg")
			return CT_JPEG;
	else
		if (ext == "jpeg")
			return CT_JPEG;
	else
		if (ext == "kml")
			return CT_KML;
	else
		if (ext == "txt")
			return CT_TEXT;
	else
		if (ext == "ttf")
			return CT_TTF;
	else
		return CT_BIN;
}

static MHD_Result processFile(
    struct MHD_Connection *connection,
    const std::string &filename
)
{
	struct MHD_Response *response;
	MHD_Result ret;
	FILE *file;
	struct stat buf;

	const char *localFileName = filename.c_str();
    bool gzipped = false;
	if (stat(localFileName, &buf) == 0)
		file = fopen(localFileName, "rb");
	else {
        std::string fnGzip(filename);
        fnGzip += ".gz";
        localFileName = fnGzip.c_str();
        if (stat(localFileName, &buf) == 0) {
            file = fopen(localFileName, "rb");
            gzipped = true;
        } else
            file = nullptr;
    }
	if (file == nullptr) {
		if (logCB)
			logCB->logMessage(connection, LOG_ERR, MODULE_WS, 404, "File not found " + filename);

		response = MHD_create_response_from_buffer(strlen(MSG404), (void *) MSG404, MHD_RESPMEM_PERSISTENT);
		ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
		MHD_destroy_response (response);
	} else {
		response = MHD_create_response_from_callback(buf.st_size, 32 * 1024,
			&file_reader_callback, file, &free_file_reader_callback);
		if (nullptr == response) {
			fclose (file);
			return MHD_NO;
		}

		MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, mimeTypeByFileExtension(filename));
        if (gzipped)
            MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_ENCODING, CE_GZIP);
		ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
		MHD_destroy_response(response);
	}
	return ret;
}

/**
 * Translate Url to the file name
 */
static std::string buildFileName(const char *dirRoot, const char *url)
{
	std::stringstream r;
	r << dirRoot;
	if (url)
	{
		r << url;
		size_t l = strlen(url);
		if (l && (url[l - 1] == '/'))
			r << "index.html";
	}
	return r.str();
}

static bool fetchJson(
    std::string &retval,
	struct MHD_Connection *connection,
    const WSConfig *config,
	const RequestContext *env
)
{
    // grpc::ServerContext svcContext;
	switch (env->requestType) {
        case RT_LOGIN: {
            rcr::LoginRequest request;
            google::protobuf::util::JsonStringToMessage(env->postData, &request, jsonParseOptions);
            rcr::LoginResponse response;
            config->svc->login(nullptr, &request, &response);
            google::protobuf::util::MessageToJsonString(response, &retval, jsonPrintOptions);
        }
            break;
        case RT_GETDICTIONARIES:
        {
            rcr::DictionariesRequest request;
            google::protobuf::util::JsonStringToMessage(env->postData, &request, jsonParseOptions);
            rcr::DictionariesResponse response;
            config->svc->getDictionaries(nullptr, &request, &response);
            google::protobuf::util::MessageToJsonString(response, &retval, jsonPrintOptions);
        }
            break;
        case RT_GETSETTINGS:
        {
            rcr::Settings request;
            google::protobuf::util::JsonStringToMessage(env->postData, &request, jsonParseOptions);
            rcr::Settings response;
            config->svc->getSettings(nullptr, &request, &response);
            google::protobuf::util::MessageToJsonString(response, &retval, jsonPrintOptions);
        }
            break;
        case RT_SETSETTINGS:
        {
            rcr::Settings request;
            google::protobuf::util::JsonStringToMessage(env->postData, &request, jsonParseOptions);
            rcr::Settings response;
            config->svc->setSettings(nullptr, &request, &response);
            google::protobuf::util::MessageToJsonString(response, &retval, jsonPrintOptions);
        }
            break;
        case RT_CHPROPERTYTYPE:
        {
            rcr::ChPropertyTypeRequest request;
            google::protobuf::util::JsonStringToMessage(env->postData, &request, jsonParseOptions);
            rcr::OperationResponse response;
            config->svc->chPropertyType(nullptr, &request, &response);
            google::protobuf::util::MessageToJsonString(response, &retval, jsonPrintOptions);
        }
            break;
        case RT_GETCARD:
        {
            rcr::GetItemRequest request;
            google::protobuf::util::JsonStringToMessage(env->postData, &request, jsonParseOptions);
            rcr::CardNPropetiesPackages response;
            config->svc->getCard(nullptr, &request, &response);
            google::protobuf::util::MessageToJsonString(response, &retval, jsonPrintOptions);
        }
            break;
        case RT_CHCARD:
        {
            rcr::ChCardRequest request;
            google::protobuf::util::JsonStringToMessage(env->postData, &request, jsonParseOptions);
            rcr::OperationResponse response;
            config->svc->chCard(nullptr, &request, &response);
            google::protobuf::util::MessageToJsonString(response, &retval, jsonPrintOptions);
        }
            break;
        case RT_CHBOX:
        {
            rcr::ChBoxRequest request;
            google::protobuf::util::JsonStringToMessage(env->postData, &request, jsonParseOptions);
            rcr::OperationResponse response;
            config->svc->chBox(nullptr, &request, &response);
            google::protobuf::util::MessageToJsonString(response, &retval, jsonPrintOptions);
        }
            break;
        case RT_CARDQUERY:
        {
            rcr::CardQueryRequest request;
            google::protobuf::util::JsonStringToMessage(env->postData, &request, jsonParseOptions);
            rcr::CardQueryResponse response;
            config->svc->cardQuery(nullptr, &request, &response);
            google::protobuf::util::MessageToJsonString(response, &retval, jsonPrintOptions);
        }
            break;
        case RT_GETBOX:
        {
            rcr::BoxRequest request;
            google::protobuf::util::JsonStringToMessage(env->postData, &request, jsonParseOptions);
            rcr::BoxResponse response;
            config->svc->getBox(nullptr, &request, &response);
            google::protobuf::util::MessageToJsonString(response, &retval, jsonPrintOptions);
        }
            break;
        case RT_CHUSER:
        {
            rcr::UserRequest request;
            google::protobuf::util::JsonStringToMessage(env->postData, &request, jsonParseOptions);
            rcr::OperationResponse response;
            config->svc->chUser(nullptr, &request, &response);
            google::protobuf::util::MessageToJsonString(response, &retval, jsonPrintOptions);
        }
            break;
        case RT_CHGROUP:
        {
            rcr::GroupRequest request;
            google::protobuf::util::JsonStringToMessage(env->postData, &request, jsonParseOptions);
            rcr::OperationResponse response;
            config->svc->chGroup(nullptr, &request, &response);
            google::protobuf::util::MessageToJsonString(response, &retval, jsonPrintOptions);
        }
            break;
        case RT_CHGROUPUSER:
        {
            rcr::GroupUserRequest request;
            google::protobuf::util::JsonStringToMessage(env->postData, &request, jsonParseOptions);
            rcr::OperationResponse response;
            config->svc->chGroupUser(nullptr, &request, &response);
            google::protobuf::util::MessageToJsonString(response, &retval, jsonPrintOptions);
        }
            break;
        case RT_IMPORTEXCEL:
        {
            rcr::ImportExcelRequest request;
            google::protobuf::util::JsonStringToMessage(env->postData, &request, jsonParseOptions);
            rcr::OperationResponse response;
            config->svc->importExcel(nullptr, &request, &response);
            google::protobuf::util::MessageToJsonString(response, &retval, jsonPrintOptions);
        }
            break;
        case RT_EXPORTEXCEL:
        {
            rcr::ExportExcelRequest request;
            google::protobuf::util::JsonStringToMessage(env->postData, &request, jsonParseOptions);
            rcr::ExportExcelResponse response;
            config->svc->exportExcel(nullptr, &request, &response);
            google::protobuf::util::MessageToJsonString(response, &retval, jsonPrintOptions);
        }
            break;
        case RT_LSUSER:
        {
            rcr::UserRequest request;
            google::protobuf::util::JsonStringToMessage(env->postData, &request, jsonParseOptions);
            rcr::UserResponse response;
            config->svc->lsUser(nullptr, &request, &response);
            google::protobuf::util::MessageToJsonString(response, &retval, jsonPrintOptions);
        }
            break;
        case RT_LJJOURNAL:
        {
            rcr::JournalRequest request;
            google::protobuf::util::JsonStringToMessage(env->postData, &request, jsonParseOptions);
            rcr::JournalResponse response;
            config->svc->lsJournal(nullptr, &request, &response);
            google::protobuf::util::MessageToJsonString(response, &retval, jsonPrintOptions);
        }
            break;
        default:
            return false;
    }
	return true;
}

static void addCORS(MHD_Response *response) {
    MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, HDR_CORS_ORIGIN);
    MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_CREDENTIALS, HDR_CORS_CREDENTIALS);
    MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_METHODS, HDR_CORS_METHODS);
    MHD_add_response_header(response, MHD_HTTP_HEADER_ACCESS_CONTROL_ALLOW_HEADERS, HDR_CORS_HEADERS);
}

static MHD_Result putStringVector(
    void *retVal,
    enum MHD_ValueKind kind,
    const char *key,
    const char *value
)
{
    std::map<std::string, std::string> *r = (std::map<std::string, std::string> *) retVal;
    r->insert(std::pair<std::string, std::string>(key, value));
    return MHD_YES;
}

static MHD_Result httpError(
    struct MHD_Connection *connection,
    int code
)
{
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(MSG_HTTP_ERROR), (void *) MSG_HTTP_ERROR, MHD_RESPMEM_PERSISTENT);
    addCORS(response);
    MHD_Result r = MHD_queue_response(connection, code, response);
    MHD_destroy_response(response);
    return r;
}

static MHD_Result httpError401(
    struct MHD_Connection *connection
)
{
    int hc = MHD_HTTP_UNAUTHORIZED;
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(MSG401), (void *) MSG401, MHD_RESPMEM_PERSISTENT);
    std::string hwa = "Bearer error=\"invalid_token\"";
    MHD_add_response_header(response, MHD_HTTP_HEADER_WWW_AUTHENTICATE, hwa.c_str());
    addCORS(response);
    MHD_Result r = MHD_queue_response(connection, hc, response);
    MHD_destroy_response(response);
    return r;
}

static MHD_Result request_callback(
	void *cls,			// struct WSConfig*
	struct MHD_Connection *connection,
	const char *url,
	const char *method,
	const char *version,
	const char *upload_data,
	size_t *upload_data_size,
	void **ptr
)
{
	struct MHD_Response *response;
	MHD_Result ret;

    if (!*ptr) {
		// do never respond on first call
		*ptr = new RequestContext;
		return MHD_YES;
	}

    if (strcmp(method, "OPTIONS") == 0) {
        response = MHD_create_response_from_buffer(strlen(MSG500[0]), (void *) MSG500[0], MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, CT_JSON);
        addCORS(response);
        MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return MHD_YES;
    }

    RequestContext *requestCtx = (RequestContext *) *ptr;
    if (*upload_data_size != 0) {
        requestCtx->postData += std::string(upload_data, *upload_data_size);
        *upload_data_size = 0;
        return MHD_YES;
    }

    requestCtx->requestType = parseRequestType(url);

    // if JSON service not found, try load from the file
    if (requestCtx->requestType == RT_UNKNOWN) {
        MHD_Result r = processFile(connection, buildFileName(((WSConfig*) cls)->dirRoot, url));
        delete requestCtx;
        *ptr = nullptr;
        return r;
	}
    int hc;
    if (strcmp(method, "DELETE") == 0) {
        hc = MHD_HTTP_NOT_IMPLEMENTED;
        response = MHD_create_response_from_buffer(strlen(MSG501), (void *) MSG501, MHD_RESPMEM_PERSISTENT);
    } else {
        // Service
        std::string json;
        bool r = fetchJson(json, connection, (WSConfig*) cls, requestCtx);
        // bool r = true; json = "{}";
        if (!r) {
            hc = MHD_HTTP_INTERNAL_SERVER_ERROR;
            response = MHD_create_response_from_buffer(strlen(MSG500[r]), (void *) MSG500[r], MHD_RESPMEM_PERSISTENT);
        } else {
            hc = MHD_HTTP_OK;
            response = MHD_create_response_from_buffer(json.size(), (void *) json.c_str(), MHD_RESPMEM_MUST_COPY);
        }
    }
    MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, CT_JSON);
    addCORS(response);
	ret = MHD_queue_response(connection, hc, response);
	MHD_destroy_response(response);
    delete requestCtx;
    *ptr = nullptr;
	return ret;
}

bool startWS(
	WSConfig &config
) {
	if (config.flags == 0)
		config.flags = MHD_START_FLAGS;

    jsonParseOptions.ignore_unknown_fields = true;
    jsonPrintOptions.add_whitespace = true;
    jsonPrintOptions.always_print_primitive_fields = true;
    jsonPrintOptions.preserve_proto_field_names = true;

    struct MHD_Daemon *d = MHD_start_daemon(
		config.flags, config.port, nullptr, nullptr,
		&request_callback, &config,
		MHD_OPTION_CONNECTION_TIMEOUT, (unsigned int) 30,  // 30s timeout
		MHD_OPTION_THREAD_POOL_SIZE, config.threadCount,
		MHD_OPTION_URI_LOG_CALLBACK, &uri_logger_callback, nullptr,
		MHD_OPTION_CONNECTION_LIMIT, config.connectionLimit,
		MHD_OPTION_END
	);
	config.descriptor = (void *) d;
	logCB = config.onLog;
	if (logCB) {
		if (!config.descriptor) {
			std::stringstream ss;
			ss << "Start web service error " << errno
				<< ": " << strerror(errno);
			logCB->logMessage(&config, LOG_ERR, MODULE_WS, errno, ss.str());
		}
	}
	return config.descriptor != nullptr;
}

void doneWS(
	WSConfig &config
) {
	logCB = nullptr;
	if (config.descriptor)
		MHD_stop_daemon((struct MHD_Daemon *) config.descriptor);
	if (logCB) {
		logCB->logMessage(&config, LOG_INFO, MODULE_WS, 0, "web service stopped");
	}
	config.descriptor = nullptr;
}
