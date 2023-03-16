
#pragma once

#include "http_proto.h"
#include <base/utils/string_tool.h>
#include <base/memory/pakcet_list.h>
#include <map>
#include <vector>

#define HTTP_END_FLAG         "\r\n\r\n"
#define HTTP_SEP              "\r\n"
#define HTTP_SEP_LEN          2
#define HTTP_CONTENT_LEN      "Content-Length:"
#define HTTP_CONTENT_TYPE     "Content-Type:"
#define HTTP_CONTENT_CHUNK    "Transfer-Encoding: chunked"
#define HTTP_CONTENT_BOUNDARY "multipart/form-data; boundary="
#define HTTP_UPGRADE          "Upgrade"

NAMESPACE_TARO_WS_BEGIN

struct BodyItem
{
    std::string key;
    std::string value;
};

struct HttpRespState
{
PUBLIC: // function

    static std::string message( int32_t code )
    {
        auto iter = instance().state_.find( code );
        TARO_ASSERT( iter !=  instance().state_.end() );
        return iter->second;
    }

PRIVATE: // function

    static HttpRespState& instance()
    {
        static HttpRespState inst;
        return inst;
    }

    HttpRespState()
    {
        state_[eHttpRespCodeOK]         = HTTP_MSG_OK;
        state_[eHttpRespCodeRedirect]   = HTTP_MSG_REDIR;
        state_[eHttpRespCodeBadReq]     = HTTP_MSG_BADREQ;
        state_[eHttpRespCodeUnauth]     = HTTP_MSG_UNAUTH;
        state_[eHttpRespCodeForbidden]  = HTTP_MSG_FORBINDDEN;
        state_[eHttpRespCodeNotFound]   = HTTP_MSG_NOTFOUND;
        state_[eHttpRespCodeInterSvr]   = HTTP_MSG_INTER_SVR;
        state_[eHttpRespCodeSvrUnavail] = HTTP_MSG_SVR_UNAVAIL;
    }

PRIVATE: // variable

    std::map<int32_t, std::string> state_;
};

inline void package_header( std::stringstream& ss, std::vector<BodyItem> const& items )
{
    for ( auto& one : items )
    {
        ss << one.key << ": " << one.value << HTTP_SEP;
    }
    ss << HTTP_SEP;
}

inline bool parse_header( std::string header, std::vector<BodyItem>& header_items )
{
    std::string::size_type pos;
    while ( ( pos = header.find( HTTP_SEP ) ) != std::string::npos )
    {
        auto line = header.substr( 0, pos );
        auto pos1 = line.find_first_of(":");
        if ( pos1 == std::string::npos )
        {
            WS_ERROR << "header format error:" << line;
        }
        else
        {
            header_items.emplace_back( BodyItem{string_trim( line.substr( 0, pos1 ) ), string_trim( line.substr( pos1 + 1 ) )} );
        }
        header = header.substr( pos + HTTP_SEP_LEN );
    }
    return true;
}

struct HttpRequestImpl
{
    static std::string serialize( HttpRequest const& req )
    {
        auto impl = req.impl_;
        std::stringstream ss;
        ss << impl->method_ << " " << impl->url_ << " " << impl->version_ << HTTP_SEP;
        package_header( ss, impl->body_items_ );
        return ss.str();
    }

    static bool deserialize( HttpRequest& req, DynPacketSPtr const& packet )
    {
        std::string http_str( ( char* )packet->buffer(), packet->size() );

        auto pos = http_str.find( HTTP_SEP );
        if ( pos == std::string::npos )
        {
            WS_ERROR << "can not find request line.";
            return false;
        }

        std::string line = http_str.substr( 0, pos );
        auto splited = split_string( line );
        if ( splited.size() != 3 )
        {
            WS_ERROR << "parse request line failed. header:" << line;
            return false;
        }
        req.impl_->method_  = string_trim( splited[0] );
        req.impl_->url_     = string_trim( splited[1] );
        req.impl_->version_ = string_trim( splited[2] );

        std::string header  = http_str.substr( pos + HTTP_SEP_LEN );
        return parse_header( header, req.impl_->body_items_ );
    }

    std::string version_;
    std::string method_;
    std::string url_;
    std::vector<BodyItem> body_items_;
};

struct HttpResponseImpl
{
    static std::string serialize( HttpResponse const& resp )
    {
        auto impl = resp.impl_;
        std::stringstream ss;
        ss << impl->version_ << " " << impl->code_ << " " << impl->state_ << HTTP_SEP;
        package_header( ss, impl->body_items_ );
        return ss.str();
    }

    static bool deserialize( HttpResponse& resp, DynPacketSPtr const& packet )
    {
        std::string http_str( ( char* )packet->buffer(), packet->size() );
        auto pos = http_str.find( HTTP_SEP );
        if ( pos == std::string::npos )
        {
            WS_ERROR << "can not find request line.";
            return false;
        }

        std::string line = http_str.substr( 0, pos );
        auto splited = split_string( line );
        if ( splited.size() < 3 )
        {
            WS_ERROR << "parse request line failed.";
            return false;
        }

        resp.impl_->version_ = string_trim( splited[0] );
        resp.impl_->code_    = atoi( string_trim( splited[1] ).c_str() );
        for ( size_t i = 2; i < splited.size(); ++i )
        {
            resp.impl_->state_ += string_trim( splited[i] );
            if ( i + 1 < splited.size() )
            {
                resp.impl_->state_ += " ";
            }
        }
        std::string header = http_str.substr( pos + HTTP_SEP_LEN );
        return parse_header( header, resp.impl_->body_items_ );
    }

    int32_t code_;
    std::string state_;
    std::string version_;
    std::vector<BodyItem> body_items_;
};

inline bool str_cmp( std::string const& left, std::string const& right )
{
    return string_compare( left, right, to_lower );
}

// 协议解析器 
class HttpProtoPaser
{
PUBLIC:  // type

    enum HttpProtoType
    {
        TYPE_NORMAL,
        TYPE_CHUNK,
        TYPE_BOUNDARY,
        TYPE_WEBSOCKET,
        TYPE_INVALID, 
    };

PUBLIC:  // function

    HttpProtoPaser()
        : type_( TYPE_INVALID )
        , chunk_bytes_( -1 )
    {

    }

    HttpProtoType type() const
    {
        return type_;
    }

    void push( DynPacketSPtr const& packet )
    {
        pktlist_.append( packet );
    }

    int32_t rest_bytes()
    {
        return pktlist_.size();
    }

    int32_t parse_header()
    {
        uint32_t pos;
        if ( !pktlist_.search( HTTP_END_FLAG, 0, pos ) )
        {
            return TARO_ERR_CONTINUE;
        }

        uint32_t len_begin = 0;
        uint32_t total_len = pos + 4;
        if ( pktlist_.search( HTTP_UPGRADE, 0, len_begin, str_cmp ) )
        {
            if ( pktlist_.search( "websocket", 0, len_begin, str_cmp ) )
            {
                type_   = TYPE_WEBSOCKET;
                header_ = pktlist_.read( total_len - 2 ); // reverse \r\n for header parse loop
                pktlist_.consume( 2 );
                return TARO_OK;
            }
        }

        if ( pktlist_.search( HTTP_CONTENT_CHUNK, 0, len_begin, str_cmp ) )
        {
            type_ = TYPE_CHUNK;
        }
        else if ( pktlist_.search( HTTP_CONTENT_BOUNDARY, 0, len_begin, str_cmp ) )
        {
            std::string tmp;
            if ( read_value( len_begin + ( uint32_t )strlen( HTTP_CONTENT_BOUNDARY ), tmp ) != TARO_OK )
            {
                WS_ERROR << "boundary not found.";
                pktlist_.consume( total_len ); // drop the packet
                return TARO_ERR_INVALID_ARG;
            }
            boundary_ = string_trim( tmp );
            type_     = TYPE_BOUNDARY;
        }
        else if ( pktlist_.search( HTTP_CONTENT_LEN, 0, len_begin, str_cmp ) )
        {
            int32_t tmp = -1;
            if ( ( read_value( len_begin + ( uint32_t )strlen( HTTP_CONTENT_LEN ), tmp ) != TARO_OK ) || tmp < 0 )
            {
                WS_ERROR << "content length not found.";
                pktlist_.consume( total_len ); // drop the packet
                return TARO_ERR_INVALID_ARG;
            }
            body_bytes_ = tmp;
            type_       = TYPE_NORMAL;
        }
        else
        {
            body_bytes_ = 0;
            type_       = TYPE_NORMAL;
        }

        header_ = pktlist_.read( total_len - 2 ); // reverse \r\n for header parse loop
        pktlist_.consume( 2 );
        return TARO_OK;
    }

    DynPacketSPtr get_header() const
    {
        return header_;
    }

    int32_t get_content( DynPacketSPtr& packet )
    {
        TARO_ASSERT( type_ == TYPE_NORMAL );

        if ( 0 == body_bytes_ )
        {
            packet = DynPacketSPtr();
            return TARO_OK;
        }

        if ( pktlist_.size() < body_bytes_ )
        {
            return TARO_ERR_CONTINUE;
        }

        packet = pktlist_.read( body_bytes_ );
        return TARO_OK;
    }

    int32_t get_chunk( DynPacketSPtr& packet )
    {
        TARO_ASSERT( type_ == TYPE_CHUNK );
        
        // chunk format: [chunk size][\r\n][chunk data][\r\n][chunk size][\r\n][chunk data][\r\n][chunk size = 0][\r\n][\r\n]
        if ( chunk_bytes_ < 0 )
        {
            auto ret = read_value( 0, chunk_bytes_, true, true );
            if ( ret == TARO_ERR_FORMAT )
            {
                WS_ERROR << "read chunk size failed";
                return TARO_ERR_INVALID_ARG;
            }

            if ( ret == TARO_ERR_CONTINUE )
            {
                return TARO_ERR_CONTINUE;
            }

            if ( chunk_bytes_ < 0 )
            {
                WS_ERROR << "chunk value invalid.";
                return TARO_ERR_INVALID_ARG;
            }
        }

        if ( 0 == chunk_bytes_ )
        {
            pktlist_.consume( HTTP_SEP_LEN );
            chunk_bytes_ = -1;
            return TARO_OK;
        }

        if ( chunk_bytes_ + HTTP_SEP_LEN > pktlist_.size() )
        {
            return TARO_ERR_CONTINUE;
        }
        packet = pktlist_.read( chunk_bytes_ );
        pktlist_.consume( HTTP_SEP_LEN );
        chunk_bytes_ = -1;
        return TARO_OK;
    }

    int32_t get_boundary( DynPacketSPtr& packet )
    {
        TARO_ASSERT( type_ == TYPE_BOUNDARY );

        std::string flag = "--";
        flag += boundary_;
        uint32_t len_begin = 0, len_end = 0;
        if ( !pktlist_.search( flag.c_str(), 0, len_begin ) )
        {
            return TARO_ERR_CONTINUE;
        }

        if ( !pktlist_.search( flag.c_str(), len_begin + ( uint32_t )flag.length() + HTTP_SEP_LEN, len_end ) )
        {
            auto end = flag + "--";
            if ( pktlist_.search( end.c_str(), 0, len_begin ) )
            {
                pktlist_.consume( ( uint32_t )end.length() + HTTP_SEP_LEN );
                return TARO_OK;
            }
            return TARO_ERR_CONTINUE; // receive not complete
        }

        auto readbytes = len_end - len_begin - flag.length() - 2 * HTTP_SEP_LEN;
        pktlist_.consume( ( uint32_t )flag.length() + HTTP_SEP_LEN );
        packet = pktlist_.read( ( uint32_t )readbytes );
        pktlist_.consume( HTTP_SEP_LEN );
        return TARO_OK;
    }

    void reset()
    {
        type_ = TYPE_INVALID;
        header_.reset();
        body_bytes_ = -1;
        boundary_ = "";
    }

PRIVATE: // function

    template<typename T>
    int32_t read_value( uint32_t len_begin, T& v, bool consume = false, bool hex = false )
    {
        uint32_t len_end = 0;
        if ( !pktlist_.search( HTTP_SEP, len_begin, len_end ) )
        {
            WS_DEBUG << "finish flag not found size:" << pktlist_.size();
            return TARO_ERR_CONTINUE;
        }

        uint32_t value_len = len_end - len_begin;
        if( 0 == value_len )
        {
            WS_ERROR << "value len is zero";
            return TARO_ERR_FORMAT;
        }

        std::string value( value_len, 0 );
        auto readbytes = pktlist_.try_read( ( uint8_t* )value.c_str(), value_len, len_begin );
        if( readbytes < ( int32_t )value_len )
        {
            WS_ERROR << "read length error";
            return TARO_ERR_CONTINUE;
        }

        if ( consume )
        {
            pktlist_.consume( value_len + HTTP_SEP_LEN );
        }
        std::stringstream ss;
        if ( hex )
            ss << std::hex;
        ss << value;
        ss >> v;
        return TARO_OK;
    }

PRIVATE: // variable

    HttpProtoType type_;
    DynPacketSPtr header_;
    int32_t       body_bytes_;
    std::string   boundary_;
    PacketList    pktlist_;
    int32_t       chunk_bytes_;
};

NAMESPACE_TARO_WS_END
