
#pragma once

#include "ws_client.h"
#include "ws_proto.h"
#include <net/tcp_client.h>

NAMESPACE_TARO_WS_BEGIN

#define RET_CHECK( ret ) if( ret < 0 ) return ret;

struct WsClientImpl
{
    WsClientImpl()
        : active_( true )
    {}

    static WsClientSPtr create( net::TcpClientSPtr const& cli )
    {
        auto client = std::make_shared<WsClient>();
        client->impl_->active_ = false;
        client->impl_->client_ = cli;
        return client;
    }

    /**
    * @brief 接收数据的逻辑，是整个库的核心逻辑之一
    */
    static int32_t recv_ws( net::TcpClientSPtr const& client, DynPacketSPtr& out, bool& last, EWsDataKind& kind, EWsEvent& evt )
    {
        uint32_t header_bytes = 0;
        uint64_t data_bytes   = 0;
        char read_buf[WS_MAX_HEAD_BYTES] = { 0 };
        while( 1 )
        {
            // 接收数据头
            auto ret = recv_msg( client, read_buf, WS_COMMON_HEAD_BYTES );
            RET_CHECK( ret );

            uint8_t* buffer = ( uint8_t* )read_buf;
            uint8_t len  = ( buffer[1] & WS_PAYLOAD_LEN_BITS ); // 获取数据长度
            data_bytes   = len;
            header_bytes = WS_COMMON_HEAD_BYTES;
            if ( len == 126 )
            {
                ret = recv_msg( client, read_buf + WS_COMMON_HEAD_BYTES, 2 );
                RET_CHECK( ret );
                uint16_t* pl = ( uint16_t* )( read_buf + WS_COMMON_HEAD_BYTES );
                data_bytes = ( uint64_t )ntohs( *pl );
                header_bytes += 2;
            }
            else if( len == 127 )
            {
                ret = recv_msg( client, read_buf + WS_COMMON_HEAD_BYTES, 8 );
                RET_CHECK( ret );
                uint64_t* pl = ( uint64_t* )( read_buf + WS_COMMON_HEAD_BYTES );
                data_bytes = ntohll( *pl );
                header_bytes += 8;
            }

            // 接收数据
            char mask[4] = { 0 };
            uint32_t mask_bytes = ( ( buffer[1] & WS_MASK_ENABLE_BIT ) ? 4 : 0 );
            if ( mask_bytes > 0 )
            {
                ret = recv_msg( client, mask, mask_bytes );
                RET_CHECK( ret );
            }

            auto packet = create_default_packet( ( uint32_t )data_bytes );
            ret = recv_msg( client, ( char* )packet->buffer(), ( uint32_t )data_bytes );
            RET_CHECK( ret );
            packet->resize( ( uint32_t )data_bytes );

            if ( mask_bytes > 0 )
            {
                uint8_t* data = ( uint8_t* )packet->buffer();
                for ( uint64_t i = 0; i < data_bytes; ++i )
                    data[i] = ( char )( data[i] ^ mask[i % 4] );
            }

            auto opcode = ( buffer[0] & WS_OP_CODE_BITS );
            evt  = eWsEventMsg;
            kind = eWsDataKindInvalid;
            if ( opcode == WS_OP_CONTENT_TEXT )
                kind = eWsDataKindText;
            else if ( opcode == WS_OP_CONTENT_BINARY )
                kind = eWsDataKindBinary;
            else if ( opcode == WS_OP_CODE_CLOSE )
                evt = eWsEventClose;
            else if( opcode == WS_OP_CODE_PING )
            {
                auto resp_pack = WsProto::create_pong_packet( ( uint8_t* )packet->buffer(), ( uint32_t )data_bytes );
                client->send( ( char* )resp_pack->buffer(), resp_pack->size() );
                continue;
            }
            else if( opcode == WS_OP_CODE_PONG )
            {
                continue;
            }
            else if( opcode == WS_OP_CODE_CONTINUE )
            {
                /*
                一个分片的消息由起始帧（FIN为0，opcode非0），
                若干（0个或多个）帧（FIN为0，opcode为0），
                结束帧（FIN为1，opcode为0）。
                */
            }
            else
            {
                return TARO_ERR_FORMAT;
            }

            out = packet;
            last = ( buffer[0] & WS_FIN_BIT );
            return TARO_OK;
        }
    }

    static int32_t recv_msg( net::TcpClientSPtr const& cli, char* buf, uint32_t bytes )
    {
        while( 1 )
        {
            auto ret = cli->recv( buf, bytes );
            if ( ret < 0 )
            {
                if ( ret == TARO_ERR_CONTINUE )
                    continue;
                return ret;
            }
            if ( ret < ( int32_t )bytes )
                continue;
            break;
        }
        return TARO_OK;
    }

    bool active_;
    std::string check_str_;
    net::TcpClientSPtr client_;
};

NAMESPACE_TARO_WS_END
