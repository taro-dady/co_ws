
#include "web_server.h"
#include <co_routine/inc.h>
#include <net/net_work.h>

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
    svr.set_routine( "/hello", []( HttpClientSPtr client, HttpRequestSPtr const& req, DynPacketSPtr const& )
    {
        WS_WARN << "hello request arrived";
        response( client );

        return true; // true 保持连接 false 断开连接
    } );

    svr.start( 20002 );
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
        //udp_conn_receiver();
        break;
    }
    net::stop_network();
    return 0;
}
