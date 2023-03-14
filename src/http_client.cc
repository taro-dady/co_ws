
#include "impl/http_client_impl.h"
#include "impl/http_proto_impl.h"

NAMESPACE_TARO_WS_BEGIN

HttpClient::HttpClient( net::SSLContext* ctx )
    : impl_( new HttpClientImpl )
{
    if ( nullptr != ctx )
    {
        impl_->ctx_ = *ctx;
    }
}

HttpClient::~HttpClient()
{
    delete impl_;
}

int32_t HttpClient::connect( const char* ip, uint16_t port )
{
    if ( !impl_->active_ )
    {
        WS_ERROR << "not support this method";
        return TARO_ERR_NOT_SUPPORT;
    }

    if ( impl_->client_ != nullptr )
    {
        WS_ERROR << "already connected";
        return TARO_ERR_MULTI_OP;
    }

    if( !STRING_CHECK( ip ) )
    {
        WS_ERROR << "ip invalid";
        return TARO_ERR_MULTI_OP;
    }

    auto tcp_cli = net::create_tcp_cli( impl_->ctx_.valid() ? impl_->ctx_.pointer() : nullptr );
    TARO_ASSERT( tcp_cli != nullptr, "create tcp client failed" );
    if( tcp_cli->connect( ip, port ) )
    {
        impl_->client_ = tcp_cli;
        return TARO_OK;
    }
    WS_ERROR << "connect to " << ip  << ":" << port << "failed";
    return TARO_ERR_FAILED;
}

int32_t HttpClient::send_req( HttpRequest const& req )
{
    if( impl_->client_ == nullptr )
    {
        WS_ERROR << "connect is invalid";
        return TARO_ERR_INVALID_RES;
    }

    if( !req.valid() )
    {
        WS_ERROR << "http request is invalid";
        return TARO_ERR_INVALID_ARG;
    }

    auto str = HttpRequestImpl::serialize( req );
    if ( str.empty() )
    {
        WS_ERROR << "serialize failed";
        return TARO_ERR_INVALID_ARG;
    }
    return impl_->client_->send( ( char* )str.c_str(), str.length() );
}

int32_t HttpClient::send_resp( HttpResponse const& resp )
{
    if( impl_->client_ == nullptr )
    {
        WS_ERROR << "connect is invalid";
        return TARO_ERR_INVALID_RES;
    }

    if( !resp.valid() )
    {
        WS_ERROR << "http response is invalid";
        return TARO_ERR_INVALID_ARG;
    }

    auto str = HttpResponseImpl::serialize( resp );
    if ( str.empty() )
    {
        WS_ERROR << "serialize failed";
        return TARO_ERR_INVALID_ARG;
    }
    return impl_->client_->send( ( char* )str.c_str(), str.length() );
}

int32_t HttpClient::send_body( DynPacketSPtr const& body )
{
    if( nullptr == body 
     || nullptr == body->buffer() 
     || 0 == body->size() )
    {
        WS_ERROR << "body invalid";
        return TARO_ERR_INVALID_ARG;
    }

    if( impl_->client_ == nullptr )
    {
        WS_ERROR << "connect is invalid";
        return TARO_ERR_INVALID_RES;
    }
    return impl_->client_->send( ( char* )body->buffer(), body->size() );
}

int32_t HttpClient::send_chunk_body( DynPacketSPtr const& body )
{
    if ( body == nullptr || body->size() == 0 )
    {
        const char* last_body = "0\r\n\r\n";
        return impl_->client_->send( ( char* )last_body, strlen( last_body ) );
    }

    std::stringstream ss;
    ss << std::hex << body->size() << HTTP_SEP;
    if ( impl_->client_->send( ( char* )ss.str().c_str(), ss.str().length() ) < 0 )
    {
        WS_ERROR << "disconnect";
        return TARO_ERR_DISCONNECT;
    }

    body->append( HTTP_SEP );
    if ( impl_->client_->send( ( char* )body->buffer(), body->size() ) < 0 )
    {
        WS_ERROR << "disconnect";
        return TARO_ERR_DISCONNECT;
    }
    return TARO_OK;
}

int32_t HttpClient::send_boundary_body( DynPacketSPtr const& body, const char* boundary )
{
    if ( !STRING_CHECK( boundary ) )
    {
        WS_ERROR << "boundary invalid";
        return TARO_ERR_INVALID_ARG;
    }

    if ( body == nullptr || body->size() == 0 )
    {
        std::stringstream ss;
        ss << "--" << boundary << "--\r\n";
        return impl_->client_->send( ( char* )ss.str().c_str(), ss.str().length() );
    }

    std::stringstream ss;
    ss << "--" << boundary << HTTP_SEP;
    if ( impl_->client_->send( ( char* )ss.str().c_str(), ss.str().length() ) < 0 )
    {
        WS_ERROR << "disconnect";
        return TARO_ERR_DISCONNECT;
    }

    body->append( HTTP_SEP );
    if ( impl_->client_->send( ( char* )body->buffer(), body->size() ) < 0 )
    {
        WS_ERROR << "disconnect";
        return TARO_ERR_DISCONNECT;
    }
    return TARO_OK;
}

HttpRespRet HttpClient::recv_resp( uint32_t ms )
{
    constexpr uint32_t default_pack_size = 1024;
    auto recv_func = [&]()
    {
        auto packet = create_default_packet( default_pack_size );
        TARO_ASSERT( impl_->client_, "connection is nullptr" );

        while( 1 )
        {
            auto ret = impl_->client_->recv( ( char* )packet->buffer(), packet->capcity(), ms );
            if( ret < 0 )
            {
                if( ret == TARO_ERR_CONTINUE )
                {
                    continue;
                }
                set_errno( ret );
                return false;
            }
            packet->resize( ret );
            impl_->parser_.push( packet );
            break;
        }
        return true;
    };

    HttpRespRet result;
    result.ret = TARO_ERR_FAILED;
    while( 1 )
    {
        if ( ( HttpProtoPaser::TYPE_INVALID == impl_->parser_.type() )
        &&   ( TARO_OK != impl_->parser_.parse_header() ) )
        {
            if ( !recv_func() )
            {
                result.ret = TARO_ERR_DISCONNECT;
                return result;
            }
            continue;
        }

        auto type = impl_->parser_.type();
        if ( HttpProtoPaser::TYPE_NORMAL == type )
        {
            DynPacketSPtr body;
            if ( TARO_ERR_CONTINUE == impl_->parser_.get_content( body ) )
            {
                if ( !recv_func() )
                {
                    result.ret = TARO_ERR_DISCONNECT;
                    return result;
                }
                continue;
            }

            auto resp = std::make_shared<HttpResponse>();
            if ( !HttpResponseImpl::deserialize( *resp, impl_->parser_.get_header() ) )
            {
                result.ret = TARO_ERR_FORMAT;
                return result;
            }

            result.body = body;
            result.resp = resp;
            result.ret  = TARO_OK;
            impl_->parser_.reset();
            impl_->resp_.reset();
            return result;
        }

        if ( impl_->resp_ == nullptr )
        {
            auto resp = std::make_shared<HttpResponse>();
            if ( !HttpResponseImpl::deserialize( *resp, impl_->parser_.get_header() ) )
            {
                result.ret = TARO_ERR_FORMAT;
                return result;
            }
            impl_->resp_ = resp;
        }

        if ( HttpProtoPaser::TYPE_CHUNK == type )
        {
            DynPacketSPtr body;
            auto ret = impl_->parser_.get_chunk( body );
            if ( TARO_ERR_CONTINUE == ret )
            {
                if ( !recv_func() )
                {
                    result.ret = TARO_ERR_DISCONNECT;
                    return result;
                }
                continue;
            }
            else if( ret != TARO_OK )
            {
                WS_ERROR << "get chunk failed";
                return result;
            }

            result.body = body;
            result.resp = impl_->resp_;
            result.ret  = TARO_OK;
            if( body == nullptr )
            {
                impl_->parser_.reset();
                impl_->resp_.reset();
            }
            return result;
        }

        if ( HttpProtoPaser::TYPE_BOUNDARY == type )
        {
            DynPacketSPtr body;
            if ( TARO_ERR_CONTINUE == impl_->parser_.get_boundary( body ) )
            {
                if ( !recv_func() )
                {
                    result.ret = TARO_ERR_DISCONNECT;
                    return result;
                }
                continue;
            }
            result.body = body;
            result.resp = impl_->resp_;
            result.ret  = TARO_OK;
            if( body == nullptr )
            {
                impl_->parser_.reset();
                impl_->resp_.reset();
            }
        }
        return result;
    }
    return HttpRespRet();
}

HttpRespRet HttpClient::request( HttpRequest const& request, uint32_t ms )
{
    auto ret = send_req( request );
    if ( ret <= 0 )
    {
        WS_ERROR << "send request failed";
        HttpRespRet result;
        result.ret = ret;
        return result;
    }
    return recv_resp( ms );
}

NAMESPACE_TARO_WS_END
