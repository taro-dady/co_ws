
#pragma once

#include <base/utils/base64.h>
#include <base/utils/sha1.h>
#include "impl/http_proto_impl.h"
#if defined( _WIN32 ) || defined( _WIN64 )
#include<Winsock2.h>
#else
#include <arpa/inet.h>

inline uint64_t htonll(uint64_t val) 
{
    return (((uint64_t) htonl(val)) << 32) + htonl(val >> 32);
}

inline uint64_t ntohll(uint64_t val) 
{
    return (((uint64_t) ntohl(val)) << 32) + ntohl(val >> 32);
}

#endif

NAMESPACE_TARO_WS_BEGIN

/*
|  0  | 1 - 3 |  4 - 7  |  8  | 9 - 15 |
  fin    rsv    opcode    mask    len  
len == 126 2个字节为长度的扩展字节
len == 127 8个字节为长度的扩展字节
len之后的4个字节当 mask = 1时为mask字节
*/
#define WS_COMMON_HEAD_BYTES   2
#define WS_MAX_HEAD_BYTES      14   // 2 COMMON + 8 bytes(extend len) + 4 mask
#define WS_OP_CODE_BITS        0x0F
#define WS_FIN_BIT             0x80
#define WS_MASK_ENABLE_BIT     0x80
#define WS_PAYLOAD_LEN_BITS    0x7F

#define WS_MIN_PACKET_SIZE     125
#define WS_MID_PACKET_SIZE     0xFFFF
#define WS_SPLICE_PACKET_SIZE  0x20000

#define WS_OP_CODE_CONTINUE  0
#define WS_OP_CONTENT_TEXT   1
#define WS_OP_CONTENT_BINARY 2
#define WS_OP_CODE_CLOSE     8
#define WS_OP_CODE_PING      9
#define WS_OP_CODE_PONG      10

inline std::string random_str( uint32_t len )
{
    ::srand( ( uint32_t )SystemTime::current_ms() );

    std::stringstream ss;
    for( uint32_t i = 0; i < len; ++i )
    {
        auto temp = ( uint8_t )( rand() % 256 );
        if( temp == 0 )
        {
            temp = 128;
        }
        ss << temp;
    }
    return ss.str();
}

inline std::string hex_to_str( std::string const& str )
{
    std::string ret;
    std::stringstream ss;
    for( uint32_t i = 0; i < str.size(); ++i )
    {
        ss << std::hex << str[i];
        uint32_t temp;
        if( ( i & 1 ) == 1 )
        {
            ss >> temp;
            ss.clear();
            ret.push_back( char( temp ) );
        }
    }
    return ret;
}

class WsProto
{
PUBLIC: 

    static HttpRequest create_open_packet( std::string const& host, std::string const& url, std::string& random_code )
    {
        random_code = "s1T+MTWvpL8NJCu0N0mX1Q==";

        HttpRequest req( "GET", url.c_str() );
        req.set( "Upgrade",                  "websocket" );
        req.set( "Connection",               "Upgrade" );
        req.set( "Sec-WebSocket-Key",        random_code );
        req.set( "Host",                     host );
        req.set( "Pragma",                   "no-cache" );
        req.set( "Cache-Control",            "no-cache" );
        req.set( "Accept-Encoding",          "gzip, deflate, br" );
        req.set( "Accept-Language",          "zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6" );
        req.set( "Sec-WebSocket-Version",    "13" );
        //req.set( "Sec-WebSocket-Extensions", "permessage-deflate; client_max_window_bits" ); 暂时不支持压缩
        return req;
    }

    static DynPacketSPtr create_pong_packet( uint8_t* buf, uint32_t bytes, bool use_mask = true )
    {
        return create_single_packet( buf, bytes, WS_OP_CODE_PONG, true, use_mask );
    }

    static std::list<DynPacketSPtr> split_packet( uint8_t* buf, uint64_t bytes, uint8_t type, bool use_mask = false )
    {
        std::list<DynPacketSPtr> packetlist;
        if ( nullptr == buf || 0 == bytes )
        {
            WS_ERROR << "packet is empty.";
            return packetlist;
        }

        if ( bytes <= WS_SPLICE_PACKET_SIZE )
        {
            packetlist.emplace_back( create_single_packet( buf, bytes, type, true, use_mask ) );
            return packetlist;
        }

        bool first = true;
        uint32_t chunks = ( uint32_t )bytes / WS_SPLICE_PACKET_SIZE;
        uint32_t rest   = ( uint32_t )bytes % WS_SPLICE_PACKET_SIZE;
        for ( uint32_t i = 0; i < chunks; ++i )
        {
            bool finish = ( ( i == chunks - 1 ) && ( rest == 0 ) );
            if ( first )
            {
                first = false;
                packetlist.emplace_back( create_single_packet( buf + i * WS_SPLICE_PACKET_SIZE, WS_SPLICE_PACKET_SIZE, type, finish, use_mask ) );
            }
            else
            {
                packetlist.emplace_back( create_single_packet( buf + i * WS_SPLICE_PACKET_SIZE, WS_SPLICE_PACKET_SIZE, WS_OP_CODE_CONTINUE, finish, use_mask ) );
            }
        }

        if ( rest > 0 )
        {
            packetlist.emplace_back( create_single_packet( buf + chunks * WS_SPLICE_PACKET_SIZE, rest, WS_OP_CODE_CONTINUE, true, use_mask ) );
        }
        return packetlist;
    }

    static std::string create_key( std::string const& k )
    {
        static const char* codes = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        auto key = k + codes;
        std::string sha_tmp = SHA1::encode( key.c_str(), ( uint32_t )key.length() );
        auto tmp = hex_to_str( sha_tmp );
        std::string res = Base64::encode( tmp );
        return res;
    }

    static DynPacketSPtr create_single_packet( uint8_t* buf, uint64_t bytes, uint8_t type, bool fin = true, bool use_mask = false )
    {
        uint32_t header_ext_len = 0;
        if( bytes > WS_MIN_PACKET_SIZE && bytes <= WS_MID_PACKET_SIZE )
            header_ext_len = 2;
        else if( bytes > WS_MID_PACKET_SIZE )
            header_ext_len = 8;

        uint32_t totalbytes = ( uint32_t )bytes + WS_COMMON_HEAD_BYTES + ( use_mask ? 4 : 0 ) + header_ext_len;
        auto temp = create_default_packet( totalbytes );
        uint8_t* header = ( uint8_t* )temp->buffer();
        ( *header ) = fin ? ( WS_FIN_BIT | type ) : type;
        ++header;

        if ( bytes > WS_MIN_PACKET_SIZE && bytes <= WS_MID_PACKET_SIZE )
        {
            ( *header ) = WS_MIN_PACKET_SIZE + 1;
            if ( use_mask )
            {
                ( *header ) |= WS_MASK_ENABLE_BIT;
            }
            ++header;
            uint16_t lp = ntohs( ( uint16_t )bytes );
            memcpy( header, ( char* )&lp, sizeof( lp ) );
            header += sizeof( lp );
        }
        else if ( bytes > WS_MID_PACKET_SIZE )
        {
            ( *header ) = WS_MIN_PACKET_SIZE + 2;
            if ( use_mask )
            {
                ( *header ) |= WS_MASK_ENABLE_BIT;
            }
            ++header;
            uint64_t lp = ntohll( bytes );
            memcpy( header, ( char* )&lp, sizeof( lp ) );
            header += sizeof( lp );
        }
        else
        {
            ( *header ) = ( uint8_t )bytes;
            if ( use_mask )
            {
                ( *header ) |= WS_MASK_ENABLE_BIT;
            }
            ++header;
        }

        if( buf != nullptr && bytes > 0 )
        {
            if( use_mask )
            {
                char mask[4] ={ 0 };
                auto random = ( uint32_t )rand();
                memcpy( mask, ( char* )&random, 4 );
                memcpy( header, ( char* )&random, 4 );
                header += 4;
                int32_t i = 0;
                while( i < ( int32_t )bytes )
                {
                    ( *header++ ) = ( *( buf + i ) ) ^ mask[i % 4];
                    ++i;
                }
            }
            else
            {
                memcpy( header, buf, bytes );
            }
        }
        temp->resize( totalbytes );
        return temp;
    }
};

NAMESPACE_TARO_WS_END
