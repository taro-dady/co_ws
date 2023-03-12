
#pragma once

#include "impl/http_proto_impl.h"
#include "impl/web_server_impl.h"
#include <map>
#include <fstream>
#include <base/utils/string_tool.h>

NAMESPACE_TARO_WS_BEGIN

class FileReader
{
PUBLIC: // function

    FileReader( const char* root )
        : index_( "index.html" )
        , root_( root )
    {
        TARO_ASSERT( FileSystem::check_dir( root ) == TARO_OK );
        root_ = string_trim_back( root_, "/" );
        init();
    }

    void set_index( std::string const& index )
    {
        index_ = index;
        index_ = string_trim( index_, "/" );
    }

    bool on_message( HttpClientSPtr conn, HttpRequestSPtr const& req, DynPacketSPtr const& )
    {
        auto method = req->method();
        if ( !string_compare( method, "GET" ) )
        {
            notfound_repsonse( conn );
            return true;
        }

        std::string url = req->url();
        if ( url == "/" )
        {
            HttpResponse resp( eHttpRespCodeRedirect );
            resp.set( "Server", "Taro Http Server 0.1" );
            resp.set_time();
            resp.set( "Location", url + index_ );
            conn->send_resp( resp );
            return true;
        }
        
        std::string file_path( root_ );
        if ( url.front() != '/' )
        {
            file_path += "/";
        }
        file_path += url;

        std::ifstream is( file_path, std::ios::binary );
        if ( !is )
        {
            notfound_repsonse( conn );
            return true;
        }

        is.seekg( 0, std::ios_base::end );
        uint32_t size = ( uint32_t )is.tellg();
        if ( size == 0 )
        {
            notfound_repsonse( conn );
            return true;
        }

        auto content = create_default_packet( size );
        is.seekg( 0, std::ios_base::beg );
        is.read( ( char* )content->buffer(), size );
        is.close();
        content->resize( size );

        HttpResponse resp;
        resp.set( "Server", "Taro Http Server 0.1" );
        resp.set( "Content-Type", get_file_type( file_path ) );
        resp.set( "Content-Length", content->size() );
        resp.set_time();
        conn->send_resp( resp );
        conn->send_body( content );
        return true;
    }

    std::string get_file_type( std::string const& file_path )
    {
        for ( auto const& one : file_type_ )
        {
            auto pos = file_path.find( one.first );
            if ( pos != std::string::npos )
            {
                return one.second;
            }
        }
        return "application/octet-stream";
    }

PRIVATE: // function

    void init()
    {
        file_type_["htm"]  = "text/html";
        file_type_["html"] = "text/html";
        file_type_["css"]  = "text/css";
        file_type_["js"]   = "application/x-javascript";
        file_type_["jpe"]  = "image/jpeg";
        file_type_["jpeg"] = "image/jpeg";
        file_type_["jpg"]  = "application/x-jpg";
        file_type_["png"]  = "image/png";
    }

    void notfound_repsonse( HttpClientSPtr conn )
    {
        HttpResponse resp( eHttpRespCodeNotFound );
        auto not_found = string_packet( "URL Not Found: The resource specified is unavailable." );
        resp.set( "Server",         "Taro Http Server 0.1" );
        resp.set( "Content-Type",   "text/html" );
        resp.set( "Content-Length", not_found->size() );
        resp.set_time();
        resp.set_close();
        auto str = HttpResponseImpl::serialize( resp );
        conn->send_resp( resp );
        conn->send_body( not_found );
    }

PRIVATE: // variable

    std::string index_;
    std::string root_;
    std::map<std::string, std::string> file_type_;
};

NAMESPACE_TARO_WS_END
