
#pragma once

#include "web_server.h"
#include "impl/file_reader.h"
#include <map>
#include <net/tcp_server.h>

NAMESPACE_TARO_WS_BEGIN

using RoutineMap = std::map<std::string, WebServer::HttpRoutineHandler>;

struct WebServerImpl
{
    RoutineMap matched_routine_;
    RoutineMap wildcard_routine_;
    net::TcpServerSPtr svr_;
    std::unique_ptr<FileReader> file_reader_;
};

NAMESPACE_TARO_WS_END
