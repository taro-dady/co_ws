
#pragma once

#include <net/defs.h>
#include <base/memory/dyn_packet.h>
#include "http_client.h"

NAMESPACE_TARO_WS_BEGIN

struct WebServerImpl;

// web服务对象
class TARO_DLL_EXPORT WebServer
{
PUBLIC: // 公共类型

    /**
     * @brief HTTP处理函数
     * 
     * @param[in] http客户端
     * @param[in] http请求
     * @param[in] http数据体
     * @return true 保持连接  false 断开连接 
    */
    using HttpRoutineHandler = std::function< bool( HttpClientSPtr, HttpRequestSPtr const&, DynPacketSPtr const& ) >;

PUBLIC: // 公共函数

    /**
     * @brief 构造函数
    */
    WebServer();

    /**
     * @brief 析构函数
    */
    ~WebServer();

    /**
     * @brief 启动服务
     * 
     * @param[in] port    监听端口号
     * @param[in] ip      监听ip
     * @param[in] ssl_ctx 加密配置
    */
    int32_t start( uint16_t port = 80, const char* ip = "0.0.0.0", net::SSLContext* ssl_ctx = nullptr );

    /**
     * @brief 设置路径处理函数
     * 
     * @param[in] url 
     * @param[in] handler 处理函数
    */
    int32_t set_routine( const char* url, HttpRoutineHandler const& handler );

    /**
     * @brief 设置静态文件的路径
     * 
     * @param[in] dir 静态文件夹的路径
    */
    int32_t set_path( const char* dir );

    /**
     * @brief 设置重定向路径
     * 
     * @param[in] redirect 重定向路径
    */
    int32_t set_redirect( const char* redirect );

PRIVATE: // 私有类型 

    friend struct WebServerImpl;

PRIVATE: // 私有函数

    TARO_NO_COPY( WebServer );
    
PRIVATE: // 私有变量
    
    WebServerImpl* impl_;
};

NAMESPACE_TARO_WS_END
