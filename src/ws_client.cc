
#include "impl/ws_proto.h"
#include "impl/ws_client_impl.h"
#include "impl/http_client_impl.h"

#if defined( _WIN32 ) || defined( _WIN64 )
#pragma comment(lib, "ws2_32.lib")
#endif

NAMESPACE_TARO_WS_BEGIN

WsClient::WsClient()
    : impl_( new WsClientImpl )
{

}

WsClient::~WsClient()
{
    delete impl_;
}

int32_t WsClient::open( const char* ip, uint16_t port, const char* url, net::SSLContext* ctx )
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

    auto tcp_cli = net::create_tcp_cli( ctx );
    TARO_ASSERT( tcp_cli != nullptr, "create tcp client failed" );
    if( !tcp_cli->connect( ip, port ) )
    {
        WS_ERROR << "connect to " << ip  << ":" << port << "failed";
        return TARO_ERR_FAILED;
    }
    
    // 发送open请求，并对回复进行校验
    std::stringstream ss;
    ss << ip << ":" << port;
    auto req = WsProto::create_open_packet( ss.str(), url, impl_->check_str_ );
    auto http_client = HttpClientImpl::create( tcp_cli );
    auto result = http_client->request( req );
    if ( result.ret != TARO_OK || result.resp == nullptr || result.resp->code() != 101 )
    {
        WS_ERROR << "server response invalid. ret:" << result.ret;
        return TARO_ERR_FAILED;
    }

    if ( !result.resp->equal( "Upgrade", "websocket" ) )
    {
        WS_ERROR << "protocol mismatch";
        return TARO_ERR_FAILED;
    }

    if ( !result.resp->equal( "Sec-WebSocket-Accept", WsProto::create_key( impl_->check_str_ ).c_str() ) )
    {
        WS_ERROR << "key check failed";
        return TARO_ERR_FAILED;
    }
    impl_->client_ = tcp_cli;
    return TARO_OK;
}

bool WsClient::send( char* buffer, int32_t bytes, EWsDataKind const& kind, bool use_mask )
{
    auto opcode = ( ( kind == eWsDataKindText ) ? WS_OP_CONTENT_TEXT : WS_OP_CONTENT_BINARY );
    auto packets = WsProto::split_packet( ( uint8_t* )buffer, bytes, opcode, use_mask );
    for ( auto const& one : packets )
    {
        if ( impl_->client_->send( ( char* )one->buffer(), one->size() ) < 0 )
        {
            return false;
        }
    }
    return true;
}

WsRecvData WsClient::recv()
{
    WsRecvData result;
    result.ret = WsClientImpl::recv_ws( impl_->client_, result.body, result.last_pack, result.kind, result.evt );
    if( result.ret < 0 )
    {
        WS_ERROR << "receive websocket failed";
    }
    return result;
}

NAMESPACE_TARO_WS_END
