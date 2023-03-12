
#pragma once

#include "http_proto.h"
#include <net/tcp_client.h>
#include <base/memory/dyn_packet.h>

NAMESPACE_TARO_WS_BEGIN

struct HttpRespRet
{
    int32_t ret;
    DynPacketSPtr body;
    HttpResponseSPtr resp;
};

struct HttpClientImpl;

// http客户端
class TARO_DLL_EXPORT HttpClient
{
PUBLIC: // 公共函数

    /**
     * @brief 构造函数
     * 
     * @param[in] ctx 加密文件
    */
    HttpClient( net::SSLContext* ctx = nullptr );

    /**
     * @brief 析构函数
    */
    ~HttpClient();

    /**
     * @brief 连接服务
     * 
     * @param[in] ip
     * @param[in] port
    */
    int32_t connect( const char* ip, uint16_t port );

    /**
     * @brief 发送请求
     * 
     * @param[in] request http请求
     * @param[in] body    http数据体
    */
    int32_t send_req( HttpRequest const& request );

    /**
     * @brief 发送回复
     * 
     * @param[in] request http回复
     * @param[in] body    http数据体
    */
    int32_t send_resp( HttpResponse const& resp );

    /**
     * @brief 发送数据体
     * 
     * @param[in] body 数据体
    */
    int32_t send_body( DynPacketSPtr const& body );

    /**
     * @brief 发送chunk数据体
     * 
     * @param[in] body 数据体 nullptr 表示最后一包
    */
    int32_t send_chunk_body( DynPacketSPtr const& body = nullptr );

    /**
     * @brief 发送boundary数据体
     * 
     * @param[in] body     数据体 nullptr 表示最后一包
     * @param[in] boundary 标识
    */
    int32_t send_boundary_body( DynPacketSPtr const& body = nullptr, const char* boundary = nullptr );

    /**
     * @brief 接收http回复
     * 
     * @param[in] ms 超时时间 0 表示一直阻塞
     * @return 回复信息
    */
    HttpRespRet recv_resp( uint32_t ms = 0 );

PRIVATE: // 私有类型
    
    friend struct HttpClientImpl;

PRIVATE: // 私有函数

    TARO_NO_COPY( HttpClient );

PRIVATE: // 私有变量

    HttpClientImpl* impl_;
};

using HttpClientSPtr = std::shared_ptr<HttpClient>;

NAMESPACE_TARO_WS_END
