
#pragma once

#include "http_client.h"
#include "impl/http_proto_impl.h"
#include <net/tcp_client.h>

NAMESPACE_TARO_WS_BEGIN

// HTTP客户端内部实现
struct HttpClientImpl
{
    /**
     * @brief 构造函数
    */
    HttpClientImpl( bool active = true )
        : active_( active )
    {

    }

    /**
     * @brief 创建http客户端
    */
    static HttpClientSPtr create( net::TcpClientSPtr client )
    {
        auto http_client = std::make_shared<HttpClient>();
        http_client->impl_->active_ = false;
        http_client->impl_->client_ = client;
        return http_client;
    }

    bool active_;
    HttpProtoPaser parser_;
    HttpResponseSPtr resp_;
    net::TcpClientSPtr client_;
    Optional<net::SSLContext> ctx_;
};

NAMESPACE_TARO_WS_END
