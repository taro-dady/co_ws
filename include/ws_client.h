
#pragma once

#include "defs.h"
#include <net/defs.h>
#include <base/memory/dyn_packet.h>

NAMESPACE_TARO_WS_BEGIN

struct WsClientImpl;

// websocket事件类型
enum EWsEvent
{
    eWsEventOpen,
    eWsEventMsg,
    eWsEventClose,    // 对方主动断线
    eWsEventInvalid,
};

// 数据类型
enum EWsDataKind
{
    eWsDataKindText,
    eWsDataKindBinary,
    eWsDataKindInvalid,
};

// websocket接收数据
struct WsRecvData
{
    /**
     * @brief 构造函数
    */
    WsRecvData()
        : last_pack( true )
        , ret( TARO_ERR_FAILED )
        , evt( eWsEventInvalid )
        , kind( eWsDataKindInvalid )
    {
        
    }

    bool          last_pack;  // 是否为最后一包(websocket存在分包的情况)
    int32_t       ret;        // 返回值 TARO_OK 表示正常 其余表示异常
    EWsEvent      evt;        // 事件类型
    EWsDataKind   kind;       // 数据类型
    DynPacketSPtr body;       // 数据体 
};

// websocket客户端
class TARO_DLL_EXPORT WsClient
{
PUBLIC: // 公共函数

    /**
     * @brief 构造函数
    */
    WsClient();

    /**
     * @brief 析构函数
    */
    ~WsClient();

    /**
     * @brief 打开websocket连接
     * 
     * @param[in] host 服务地址
     * @param[in] port 服务端口号
     * @param[in] url  路径信息
     * @param[in] ctx  加密通信的环境配置
    */
    int32_t open( const char* host, uint16_t port = 80, const char* url = "/", net::SSLContext* ctx = nullptr);

    /**
     * @brief 发送数据
     * 
     * @param[in] buffer   数据缓冲
     * @param[in] bytes    数据大小
     * @param[in] kind     数据类型
     * @param[in] use_mask 是否使用掩码
    */
    bool send( char* buffer, int32_t bytes, EWsDataKind const& kind = eWsDataKindText, bool use_mask = false );

    /**
     * @brief 数据接收
     * 
     * @return 见WsRespRet定义
    */
    WsRecvData recv();

PRIVATE: // 私有类型
    
    friend struct WsClientImpl;

PRIVATE: // 私有函数

    TARO_NO_COPY( WsClient );

PRIVATE: // 私有变量

    WsClientImpl* impl_;
};

using WsClientSPtr = std::shared_ptr<WsClient>;

NAMESPACE_TARO_WS_END
