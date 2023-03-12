
#include "impl/web_server_impl.h"
#include "impl/http_proto_impl.h"
#include "impl/http_client_impl.h"
#include <co_routine/inc.h>

NAMESPACE_TARO_WS_BEGIN

// 消息处理
struct MsgHandler
{
PUBLIC: // 公共函数

    /**
     * @brief 构造函数
    */
    MsgHandler( net::TcpClientSPtr const& client, WebServerImpl* impl )
        : impl_( impl )
        , client_( client )
    {

    }

    /**
     * @brief 消息处理函数
    */
    bool on_msg_arrived( DynPacketSPtr const& packet )
    {
        parser_.push( packet );
        if ( HttpProtoPaser::TYPE_INVALID == parser_.type() )
        {
            auto ret = parser_.parse_header();
            if ( ret == TARO_ERR_INVALID_ARG )
            {
                WS_ERROR << "parse http request header failed";
                return false; 
            }
            else if ( ret == TARO_ERR_CONTINUE )
            {
                return true;
            }
        }

        if ( nullptr == header_ )
        {
            header_ = std::make_shared<HttpRequest>();
            if ( !HttpRequestImpl::deserialize( *header_, parser_.get_header() ) )
            {
                WS_ERROR << "deserialize http request failed";
                return false; 
            }

            if( !find_handler() )
            {
                notfound_repsonse();
                clear();
                return false;
            }
        }

        auto type = parser_.type();
        if ( HttpProtoPaser::TYPE_NORMAL == type )
        {
            DynPacketSPtr content;
            if ( TARO_ERR_CONTINUE == parser_.get_content( content ) )
                return true;
            if( !handler_( HttpClientImpl::create( client_ ), header_, content ) )
                return false;
            clear();
        }
        else if( HttpProtoPaser::TYPE_CHUNK == type )
        {
            DynPacketSPtr content;
            auto ret = parser_.get_chunk( content );
            if ( ret == TARO_ERR_INVALID_ARG )
            {
                WS_ERROR << "get chunk size failed, format error";
                return false;
            }

            if ( ret == TARO_OK )
            {
                if( !handler_( HttpClientImpl::create( client_ ), header_, content ) )
                    return false;

                if( content == nullptr ) 
                    clear();
            }
        }
        else if( HttpProtoPaser::TYPE_BOUNDARY == type )
        {
            DynPacketSPtr content;
            auto ret = parser_.get_boundary( content );
            if ( ret == TARO_ERR_INVALID_ARG )
            {
                WS_ERROR << "get boundary failed, format error";
                return false;
            }

            if ( ret == TARO_OK )
            {
                if( !handler_( HttpClientImpl::create( client_ ), header_, content ) )
                    return false;

                if( content == nullptr )
                    clear();
            }
        }
        else if( HttpProtoPaser::TYPE_WEBSOCKET == type )
        {

        }
        else
        {
            TARO_ASSERT( 0, "type invalid", type );
        }
        return true;
    }

PRIVATE: // 私有函数

    void clear()
    {
        header_.reset();
        handler_ = nullptr;
        parser_.reset();
    }

    bool find_handler()
    {
        auto it = impl_->matched_routine_.find( header_->url() );
        if ( it != impl_->matched_routine_.end() )
        {
            handler_ = it->second;
            return true;
        }
        return false;
    }

    void notfound_repsonse()
    {
        HttpResponse resp( eHttpRespCodeNotFound );
        const char* not_found = "URL Not Found: The resource specified is unavailable.";
        resp.set( "Server",         "Taro Http Server 0.1" );
        resp.set( "Content-Type",   "text/html" );
        resp.set( "Content-Length", strlen( not_found ) );
        resp.set_time();
        resp.set_close();
        auto str = HttpResponseImpl::serialize( resp );
        client_->send( ( char* )str.c_str(), str.length() );
        client_->send( ( char* )not_found, strlen( not_found ) );
    }

PRIVATE: // 私有变量

    HttpProtoPaser parser_;
    WebServerImpl* impl_;
    HttpRequestSPtr header_;
    net::TcpClientSPtr client_;
    WebServer::HttpRoutineHandler handler_;
};

/**
* @brief 服务端连接处理协程函数
*/
void client_handle( net::TcpClientSPtr const& client, WebServerImpl* impl )
{
    MsgHandler handler( client, impl );
    while( 1 )
    {
        auto packet = create_default_packet( 1024 );
        auto ret = client->recv( ( char* )packet->buffer(), packet->capcity() );
        if( ret > 0 )
        {
            packet->resize( ret );
            if( !handler.on_msg_arrived( packet ) )
            {
                break;
            }
        }
        else if( ret == TARO_ERR_CONTINUE )
        {
            continue;
        }
        else
        {
            WS_ERROR << "client disconnect";
            break;
        }
    }
}

WebServer::WebServer()
    : impl_( new WebServerImpl )
{

}

WebServer::~WebServer()
{
    delete impl_;
}

int32_t WebServer::start( uint16_t port, const char* ip, net::SSLContext* ssl_ctx )
{
    if ( !STRING_CHECK( ip ) )
    {
        WS_ERROR << "ip invalid";
        return TARO_ERR_INVALID_ARG;
    }

    if ( nullptr != impl_->svr_ )
    {
        WS_ERROR << "mutiple start";
        return TARO_ERR_MULTI_OP;
    }

    impl_->svr_ = net::create_tcp_svr( ssl_ctx );
    if ( impl_->svr_ == nullptr )
    {
        WS_ERROR << "create tcp server failed";
        return TARO_ERR_FAILED;
    }

    if ( !impl_->svr_->listen( ip, port ) )
    {
        impl_->svr_.reset();
        WS_ERROR << "listen failed. ip:" << ip << ":" << port;
        return TARO_ERR_FAILED;
    }

    co_run [&]()
    {
        while( 1 )
        {
            auto client = impl_->svr_->accept();
            co_run std::bind( client_handle, client, impl_ ), opt_name( "web_client" );
        }
    }, opt_name( "webserver" );
    return TARO_OK;
}

int32_t WebServer::set_routine( const char* url, HttpRoutineHandler const& handler )
{
    if ( !STRING_CHECK( url ) || !handler )
    {
        WS_ERROR << "parameter invalid";
        return TARO_ERR_INVALID_ARG;
    }

    impl_->matched_routine_[url] = handler;
    return TARO_OK;
}

int32_t WebServer::set_path( const char* dir )
{
    return TARO_OK;
}

int32_t WebServer::set_redirect( const char* redirect )
{
    return TARO_OK;
}

NAMESPACE_TARO_WS_END
