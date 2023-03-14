
#include "web_server.h"
#include <co_routine/inc.h>
#include <net/net_work.h>
#include <iostream>

USING_NAMESPACE_TARO
USING_NAMESPACE_TARO_WS

void response(HttpClientSPtr const& conn)
{
    HttpResponse resp;
    auto content = string_packet("<html><head><title>taro</title></head><body>hello, response</body></html>");
    resp.set( "Server",  "Taro Http Server 0.1");
    resp.set( "Content-Type", "text/html; charset=UTF-8");
    resp.set( "Content-Length", content->size() );
    resp.set_time();
    conn->send_resp( resp );
    conn->send_body( content );
}

void web_svr_test()
{
    WebServer svr;

    // 静态文件路径 将test中的web目录拷贝到执行目录下
    svr.set_path( "web" ); 

    // 动态命令处理
    svr.set_routine( "/hello", []( HttpClientSPtr client, HttpRequestSPtr const& req, DynPacketSPtr const& body )
    {
        WS_WARN << "hello request arrived method:" << req->method();
        if( body ){
            WS_WARN << "body:" << std::string( ( char* )body->buffer(), body->size() );
        }
        response( client );
        return true; // true 保持连接  false 断开连接
    } );

    // 服务端接收客户端推送的chunk
    svr.set_routine( "/post_chunk", []( HttpClientSPtr client, HttpRequestSPtr const& req, DynPacketSPtr const& body )
    {
        static std::stringstream* ss = nullptr;
        if ( nullptr == ss )
            ss = new std::stringstream;
        
        if( body != nullptr )
        {
            // chunk包
            auto str = std::string( ( char* )body->buffer(), body->size() );
            *ss << str;
            std::cout << "receive:" << str << std::endl;
        }
        else
        {
            // chunk接收结束
            std::cout << ss->str() << std::endl;
            delete ss;
            ss = nullptr;
            response( client );
        }
        return true;
    } );

    // 客户端请求服务端推送chunk
    svr.set_routine( "/get_chunk", []( HttpClientSPtr client, HttpRequestSPtr const& req, DynPacketSPtr const& )
    {
        WS_WARN << "on chunk url:" << req->url() << " method:" << req->method();

        HttpResponse resp;
        resp.set( "User-Agent", "Taro Client" );
        resp.set( "Accept-Encoding", "identity" );
        resp.set( "Content-Type", "text/html" );
        resp.set( "Transfer-Encoding", "chunked" ); // 设置chunk传输格式
        client->send_resp( resp );

        // 发送chunk数据 
        std::string body = "<html><head><title>taro</title></head><body>hello, response</body></html>";
        auto count = body.length() / 10;
        for( size_t i = 0; i < count; ++i )
        {
            auto content = string_packet( body.substr( 10 * i, 10 ) );
            client->send_chunk_body( content );
        }
        auto rest = body.length() % 10;
        auto content = string_packet( body.substr( count * 10, rest ) );
        client->send_chunk_body( content );
        client->send_chunk_body(); // 发送结束标识
        return true;
    } );

    svr.set_routine( "/post_boundary", []( HttpClientSPtr client, HttpRequestSPtr const& req, DynPacketSPtr const& body )
    {
        WS_WARN << "on boundary url:" << req->url() << " method:" << req->method();

        static std::stringstream* ss = nullptr;
        if( nullptr == ss )
            ss = new std::stringstream;

        if( body != nullptr )
        {
            auto str = std::string( ( char* )body->buffer(), body->size() );
            *ss << str;
            std::cout << "receive:" << str << std::endl;
        }
        else
        {
            // 接收结束
            std::cout << ss->str() << std::endl;
            delete ss;
            ss = nullptr;
            response( client );
        }
        return true;
    } );

    svr.set_routine( "/get_boundary", []( HttpClientSPtr client, HttpRequestSPtr const& req, DynPacketSPtr const& )
    {
        WS_WARN << "on boundary url:" << req->url() << " method:" << req->method();

        const char* boundary = "myboundarytest";

        // 发送http头
        HttpResponse resp;
        resp.set( "User-Agent", "Taro Client" );
        resp.set( "Accept-Encoding", "identity" );
        resp.set_boundary( boundary ); // 设置boundary标识
        client->send_resp( resp );
        
        // 发送bounady数据
        std::string body = "<html><head><title>taro</title></head><body>hello, response</body></html>";
        auto count = body.length() / 10;
        for( size_t i = 0; i < count; ++i )
        {
            auto content = string_packet( body.substr( 10 * i, 10 ) );
            client->send_boundary_body( content, boundary );
        }

        auto rest = body.length() % 10;
        auto content = string_packet( body.substr( count * 10, rest ) );
        client->send_boundary_body( content, boundary );
        client->send_boundary_body( nullptr, boundary ); // 结束标识
        return true;
    } );

    svr.start( 20002 );
    rt::co_loop();
}

void http_client_test()
{
    co_run []()
    {
        auto client = std::make_shared<HttpClient>();
        while( client->connect( "127.0.0.1", 20002 ) != TARO_OK )
        {
            rt::co_wait( 1000 );
        }
        printf( "connect web server ok\n" );

        while( 1 )
        {
            HttpRequest req( "GET", "/index.html" );
            req.set( "User-Agent", "Taro Client" );
            req.set( "Accept-Encoding", "identity" );
            auto result = client->request( req );
            if ( result.ret == TARO_OK )
            {
                printf("%s\n", std::string( (char*)result.body->buffer(), result.body->size()).c_str() );
            }
            else if( result.ret == TARO_ERR_DISCONNECT )
            {
                printf("disconnect\n");
                break;
            }
            rt::co_wait( 1000 );
        }
    };
    rt::co_loop();
}

void http_client_post_test()
{
    co_run[]()
    {
        auto client = std::make_shared<HttpClient>();
        while( client->connect( "127.0.0.1", 20002 ) != TARO_OK )
        {
            rt::co_wait( 1000 );
        }
        printf( "connect web server ok\n" );

        while( 1 )
        {
            auto content = string_packet( "hello http server" );
            HttpRequest req( "POST", "/hello" );
            req.set( "User-Agent", "Taro Client" );
            req.set( "Accept-Encoding", "identity" );
            req.set( "Content-Length", content->size() );
            client->send_req( req );
            client->send_body( content );

            auto result = client->recv_resp();
            if( result.ret == TARO_OK )
            {
                printf( "request ok\n" );
            }
            else if( result.ret == TARO_ERR_DISCONNECT )
            {
                printf( "disconnect\n" );
                break;
            }
            rt::co_wait( 1000 );
        }
    };
    rt::co_loop();
}

void http_client_post_chunk()
{
    co_run []()
    {
        auto client = std::make_shared<HttpClient>();
        while( client->connect( "127.0.0.1", 20002 ) != TARO_OK )
        {
            rt::co_wait( 1000 );
        }
        printf( "connect web server ok\n" );

        while( 1 )
        {
            HttpRequest req( "POST", "/post_chunk" );
            req.set( "User-Agent", "Taro Client" );
            req.set( "Accept-Encoding", "identity" );
            req.set( "Content-Type", "text/html" );
            req.set( "Transfer-Encoding", "chunked" ); // 设置chunk传输格式
            client->send_req( req );

            // 发送chunk数据 
            std::string body = "<html><head><title>taro</title></head><body>hello, response</body></html>";
            auto count = body.length() / 10;
            for( size_t i = 0; i < count; ++i )
            {
                auto content = string_packet( body.substr( 10 * i, 10 ) );
                client->send_chunk_body( content );
            }
            auto rest = body.length() % 10;
            auto content = string_packet( body.substr( count * 10, rest ) );
            client->send_chunk_body( content );
            client->send_chunk_body(); // 发送结束标识

            auto ret = client->recv_resp();
            if ( ret.ret == TARO_OK )
            {
                std::cout << "receive ok" << std::endl;
            }
            else
            {
                std::cout << "receive failed" << std::endl;
            }
 
            rt::co_wait( 1000 );
        }
    };
    rt::co_loop();
}

void http_client_get_chunk()
{
    co_run []()
    {
        auto client = std::make_shared<HttpClient>();
        while( client->connect( "127.0.0.1", 20002 ) != TARO_OK )
        {
            rt::co_wait( 1000 );
        }
        printf( "connect web server ok\n" );

        while( 1 )
        {
            HttpRequest req( "GET", "/get_chunk" );
            req.set( "User-Agent", "Taro Client" );
            req.set( "Accept-Encoding", "identity" );
            req.set( "Content-Type", "text/html" );
            client->send_req( req );

            std::stringstream ss;
            while( 1 )
            {
                auto ret = client->recv_resp();
                if ( ret.body )
                {
                    std::string tmp = std::string( (char*)ret.body->buffer(), ret.body->size() );
                    ss << tmp;
                }
                else
                {
                    std::cout << ss.str() << std::endl;
                    break;
                }
            }
            rt::co_wait( 200 );
        }
    };
    rt::co_loop();
}

void http_client_post_boundary()
{
    co_run[]()
    {
        auto client = std::make_shared<HttpClient>();
        while( client->connect( "127.0.0.1", 20002 ) != TARO_OK )
        {
            rt::co_wait( 1000 );
        }
        printf( "connect web server ok\n" );

        const char* boundary = "myboundarytest";
        while( 1 )
        {
            HttpRequest req( "POST", "/post_boundary" );
            req.set( "User-Agent", "Taro Client" );
            req.set( "Accept-Encoding", "identity" );
            req.set( "Content-Type", "text/html" );
            req.set_boundary( boundary ); // 设置boundary标识
            client->send_req( req );

            // 发送chunk数据 
            std::string body = "<html><head><title>taro</title></head><body>hello, response</body></html>";
            auto count = body.length() / 10;
            for( size_t i = 0; i < count; ++i )
            {
                auto content = string_packet( body.substr( 10 * i, 10 ) );
                client->send_boundary_body( content, boundary );
            }

            auto rest = body.length() % 10;
            auto content = string_packet( body.substr( count * 10, rest ) );
            client->send_boundary_body( content, boundary );
            client->send_boundary_body( nullptr, boundary ); // 结束标识

            auto ret = client->recv_resp();
            if( ret.ret == TARO_OK )
            {
                std::cout << "receive ok" << std::endl;
            }
            else
            {
                std::cout << "receive failed" << std::endl;
            }

            rt::co_wait( 1000 );
        }
    };
    rt::co_loop();
}

void http_client_get_boundary()
{
    co_run[]()
    {
        auto client = std::make_shared<HttpClient>();
        while( client->connect( "127.0.0.1", 20002 ) != TARO_OK )
        {
            rt::co_wait( 1000 );
        }
        printf( "connect web server ok\n" );

        while( 1 )
        {
            HttpRequest req( "GET", "/get_boundary" );
            req.set( "User-Agent", "Taro Client" );
            req.set( "Accept-Encoding", "identity" );
            req.set( "Content-Type", "text/html" );
            client->send_req( req );

            std::stringstream ss;
            while( 1 )
            {
                auto ret = client->recv_resp();
                if( ret.body )
                {
                    ss << std::string( ( char* )ret.body->buffer(), ret.body->size() );
                }
                else
                {
                    std::cout << ss.str() << std::endl;
                    break;
                }
            }
            rt::co_wait( 1000 );
        }
    };
    rt::co_loop();
}

int main( int argc, char** argv )
{
    if ( argc < 2 )
    {
        perror( "parameter error\n" ); 
        exit( 0 );
    }

    rt::CoRoutine::init();
    net::start_network();
    
    switch ( atoi( argv[1] ) )
    {
    case 0:
        web_svr_test();
        break;
    case 1:
        http_client_test();
        break;
    case 2:
        http_client_post_chunk();
        break;
    case 3:
        http_client_get_chunk();
        break;
    case 4:
        http_client_post_boundary();
        break;
    case 5:
        http_client_get_boundary();
        break;
    case 6:
        http_client_post_test();
        break;
    }
    net::stop_network();
    return 0;
}
