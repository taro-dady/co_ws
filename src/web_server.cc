
#include "impl/ws_proto.h"
#include "impl/ws_client_impl.h"
#include "impl/web_server_impl.h"
#include "impl/http_proto_impl.h"
#include "impl/http_client_impl.h"
#include <co_routine/inc.h>
#include <base/utils/string_tool.h>

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
        , msg_handler_( std::bind( &MsgHandler::on_http_recv, this ) )
    {

    }

    /**
     * @brief 消息处理函数
    */
    bool recv_msg()
    {
        return msg_handler_();
    }

PRIVATE: // 私有函数

    /**
     * @brief http消息接收并处理
    */
    bool on_http_recv()
    {
        auto packet = create_default_packet( 1024 );
        auto ret = client_->recv( ( char* )packet->buffer(), packet->capcity() );
        if( ret > 0 )
        {
            packet->resize( ret );
            if( !on_http_arrived( packet ) )
            {
                return false;
            }
        }
        else if( ret == TARO_ERR_CONTINUE )
        {
            return true;
        }
        else
        {
            WS_ERROR << "client disconnect";
            return false;
        }
        return true;
    }

    /**
     * @brief http消息处理函数
    */
    bool on_http_arrived( DynPacketSPtr const& packet )
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
            return on_chunk_msg();
        }
        else if( HttpProtoPaser::TYPE_BOUNDARY == type )
        {
            return on_boundary_msg();
        }
        else if( HttpProtoPaser::TYPE_WEBSOCKET == type )
        {
            return on_web_socket();
        }
        else
        {
            TARO_ASSERT( 0, "type invalid", type );
        }
        return true;
    }

    /**
     * @brief ws消息处理函数
    */
    bool on_ws_recv()
    {
        WsRecvData result;
        auto ret = WsClientImpl::recv_ws( client_, result.body, result.last_pack, result.kind, result.evt );
        if ( ret < 0 )
        {
            WS_ERROR << "receive websocket failed";
            return false;
        }

        if ( impl_->ws_handler_ )
        {
            result.ret = TARO_OK;
            impl_->ws_handler_( WsClientImpl::create( client_ ), result );
        }
        return true;
    }

    /**
     * @brief 清除状态
    */
    void clear()
    {
        header_.reset();
        handler_ = nullptr;
        parser_.reset();
    }

    /**
     * @brief 查询处理函数
    */
    bool find_handler()
    {
        auto it = impl_->matched_routine_.find( header_->url() );
        if ( it != impl_->matched_routine_.end() )
        {
            handler_ = it->second;
            return true;
        }

        std::list<std::string> shooted;
        for( auto const& one : impl_->wildcard_routine_ )
        {
            if( wildcard_match( one.first, header_->url() ) )
            {
                shooted.push_back( one.first );
            }
        }

        if( shooted.empty() )
        {
            return false;
        }

        shooted.sort( []( std::string const& a, std::string const& b )
        {
            if( a.length() > b.length() ) // choose longger string
            {
                return true;
            }

            if( a.length() == b.length() )
            {
                auto pos = a.find( "?" ); // "?" priority higher than "*"
                if( pos != std::string::npos )
                {
                    return true;
                }
            }
            return false;
        } );

        handler_ = impl_->wildcard_routine_[shooted.front()];
        return true;
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

    bool on_chunk_msg()
    {
        while( 1 )
        {
            DynPacketSPtr content;
            auto ret = parser_.get_chunk( content );
            if( ret == TARO_ERR_INVALID_ARG )
            {
                WS_ERROR << "get chunk size failed, format error";
                return false;
            }

            if( ret == TARO_OK )
            {
                if( !handler_( HttpClientImpl::create( client_ ), header_, content ) )
                    return false;

                if( content == nullptr )
                {
                    clear();
                    break;
                }
            }
            else
            {
                break;
            }
        }
        return true;
    }

    bool on_boundary_msg()
    {
        while( 1 )
        {
            DynPacketSPtr content;
            auto ret = parser_.get_boundary( content );
            if( ret == TARO_ERR_INVALID_ARG )
            {
                WS_ERROR << "get boundary failed, format error";
                return false;
            }

            if( ret == TARO_OK )
            {
                if( !handler_( HttpClientImpl::create( client_ ), header_, content ) )
                    return false;

                if( content == nullptr )
                {
                    clear();
                    break;
                }
            }
            else
            {
                break;
            }
        }
        return true;
    }

    bool on_web_socket()
    {
        auto key = header_->get<std::string>( "sec-websocket-key" );
        if ( !key.valid() )
        {
            WS_ERROR <<  "websocket key not found.";
            return false;
        }

        if ( impl_->ws_handler_ )
        {
            WsRecvData result;
            result.evt = eWsEventOpen;
            impl_->ws_handler_( WsClientImpl::create( client_ ), result );
            msg_handler_ = std::bind( &MsgHandler::on_ws_recv, this );
        }
        
        HttpResponse resp( 101, "Switching protocols" );
        resp.set( "Server",               "taro/1.1" );
        resp.set( "Upgrade",              "websocket" );
        resp.set( "Connection",           "upgrade" );
        resp.set( "Sec-WebSocket-Accept", WsProto::create_key( key ) );
        auto str = HttpResponseImpl::serialize( resp );
        client_->send( ( char* )str.c_str(), str.length() );
        return true;
    }

PRIVATE: // 私有变量

    HttpProtoPaser parser_;
    WebServerImpl* impl_;
    HttpRequestSPtr header_;
    net::TcpClientSPtr client_;
    std::function<bool()> msg_handler_;
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
        if ( !handler.recv_msg() )
        {
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

    if( is_wildcard( url ) )
    {
        impl_->wildcard_routine_[url] = handler;
        return TARO_OK;
    }

    impl_->matched_routine_[url] = handler;
    return TARO_OK;
}

int32_t WebServer::set_path( const char* dir )
{
    if ( !STRING_CHECK( dir ) || FileSystem::check_dir( dir ) != TARO_OK )
    {
        WS_ERROR << "parameter invalid";
        return TARO_ERR_INVALID_ARG;
    }

    impl_->file_reader_.reset( new FileReader( dir ) );
    impl_->wildcard_routine_["/*"] =
        std::bind( &FileReader::on_message, impl_->file_reader_.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );
    return TARO_OK;
}

int32_t WebServer::set_ws_handler( WebsocketHandler const& handler )
{
    if ( !handler )
    {
        WS_ERROR << "parameter invalid";
        return TARO_ERR_INVALID_ARG;
    }
    impl_->ws_handler_ = handler;
    return TARO_OK;
}

NAMESPACE_TARO_WS_END
