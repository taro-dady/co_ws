
#pragma once

#include "log/inc.h"
#include "base/error_no.h"
#include "base/utils/assert.h"
#include <string>
#include <memory>
#include <sstream>

#define NAMESPACE_TARO_WS_BEGIN namespace taro { namespace ws{
#define NAMESPACE_TARO_WS_END } }
#define USING_NAMESPACE_TARO_WS using namespace taro::ws;

#define WS_FATAL taro::log::LogWriter( "co_ws", taro::log::eLogLevelFatal, __FILE__, __FUNCTION__, __LINE__ ).stream<std::stringstream>()
#define WS_ERROR taro::log::LogWriter( "co_ws", taro::log::eLogLevelError, __FILE__, __FUNCTION__, __LINE__ ).stream<std::stringstream>()
#define WS_WARN  taro::log::LogWriter( "co_ws", taro::log::eLogLevelWarn,  __FILE__, __FUNCTION__, __LINE__ ).stream<std::stringstream>()
#define WS_TRACE taro::log::LogWriter( "co_ws", taro::log::eLogLevelTrace, __FILE__, __FUNCTION__, __LINE__ ).stream<std::stringstream>()
#define WS_DEBUG taro::log::LogWriter( "co_ws", taro::log::eLogLevelDebug, __FILE__, __FUNCTION__, __LINE__ ).stream<std::stringstream>()

NAMESPACE_TARO_BEGIN

#define HTTP_VERSION "HTTP/1.1"

enum EHttpRespCode
{
    eHttpRespCodeOK          = 200,
    eHttpRespCodeRedirect    = 302,
    eHttpRespCodeBadReq      = 400,
    eHttpRespCodeUnauth      = 401,
    eHttpRespCodeForbidden   = 403,
    eHttpRespCodeNotFound    = 404,
    eHttpRespCodeInterSvr    = 500,
    eHttpRespCodeSvrUnavail  = 503,
};

#define HTTP_MSG_OK           "OK"
#define HTTP_MSG_REDIR        "Moved Temporarily"
#define HTTP_MSG_BADREQ       "Bad Request"
#define HTTP_MSG_UNAUTH       "Unauthorized"
#define HTTP_MSG_FORBINDDEN   "Forbidden"
#define HTTP_MSG_NOTFOUND     "Not Found"
#define HTTP_MSG_INTER_SVR    "Internal Server Error"
#define HTTP_MSG_SVR_UNAVAIL  "Server Unavailable"

NAMESPACE_TARO_END
