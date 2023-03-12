
#pragma once

#include "defs.h"
#include <base/memory/optional.h>

NAMESPACE_TARO_WS_BEGIN

struct HttpRequestImpl;
struct HttpResponseImpl;

// http请求
class TARO_DLL_EXPORT HttpRequest
{
PUBLIC: // 公共函数

    /**
     * @brief 构造函数
    */
    HttpRequest();

    /**
     * @brief 构造函数
     * 
     * @param[in] method
     * @param[in] url
    */ 
    HttpRequest( const char* method, const char* url, const char* version = HTTP_VERSION );

    /**
     * @brief 移动构造函数
     * 
     * @param[in] other
    */
    HttpRequest( HttpRequest&& other );

    /**
     * @brief 析构函数
    */
    ~HttpRequest();

    /**
     * @brief 是否有效
    */
    bool valid() const;

    /**
     * @brief 获取版本
    */
    const char* version() const;

    /**
     * @brief 获取方法
    */
    const char* method() const;

    /**
     * @brief 获取URL
    */
    const char* url() const;

    /**
     * @brief 判断关键字是否存在
     * 
     * @param[in] key 关键字
     * @return true 存在 false 不存在
    */
    bool contains( const char* key );

    /**
     * @brief 设置时间
    */
    void set_time();

    /**
     * @brief 设置关闭
    */
    void set_close();

    /**
     * @brief 设置body值
     * 
     * @param[in] key 关键字
     * @param[in] val 值
    */
    template<typename T>
    bool set( const char* key, T const& val )
    {
        if ( !STRING_CHECK( key ) )
        {
            return false;
        }

        std::stringstream ss;
        ss << val;
        set_str( key, ss.str().c_str() );
        return true;
    }

    /**
     * @brief 获取body值
     * 
     * @param[in] key 关键字
     * @return 关键字对应的值
    */
    template<typename T>
    Optional<T>& get( const char* key )
    {
        std::string str_val;
        if ( !get_str( key, str_val ) )
        {
            return Optional<T>();
        }

        std::stringstream ss;
        ss << str_val;
        T val;
        ss >> val;
        return Optional<T>( val );
    }

PRIVATE: // 私有类型 

    friend struct HttpRequestImpl;

PRIVATE: // 私有函数

    TARO_NO_COPY( HttpRequest );

    /**
     * @brief 设置body值
     * 
     * @param[in] key 关键字
     * @param[in] val 值
    */
    void set_str( const char* key, const char* value );

    /**
     * @brief 获取body值
     * 
     * @param[in]  key 关键字
     * @param[out] val 值
     * @return true 成功  false 失败 
    */
    bool get_str( const char* key, std::string& value );

PRIVATE: // 私有变量

    HttpRequestImpl* impl_;
};

// http回复
class TARO_DLL_EXPORT HttpResponse
{
PUBLIC: // 公共函数

    /**
     * @brief 构造函数
     * 
     * @param[in] code    状态码
     * @param[in] version 协议版本
    */
    HttpResponse( int32_t code = eHttpRespCodeOK, const char* version = HTTP_VERSION );

    /**
     * @brief 构造函数
     * 
     * @param[in] code   返回值
     * @param[in] status 状态信息
    */
    HttpResponse( int32_t code, const char* status, const char* version = HTTP_VERSION );

    /**
     * @brief 移动构造函数
     * 
     * @param[in] other
    */
    HttpResponse( HttpResponse&& other );

    /**
     * @brief 析构函数
    */
    ~HttpResponse();

    /**
     * @brief 是否有效
    */
    bool valid() const;

    /**
     * @brief 获取版本
    */
    const char* version() const;

    /**
     * @brief 获取状态信息
     * 
     * @return 状态信息
    */
    const char* status() const;

    /**
     * @brief 获取状态码
     * 
     * @return 状态码
    */
    int32_t code() const;

    /**
     * @brief 判断关键字是否存在
     * 
     * @param[in] key 关键字
     * @return true 存在 false 不存在
    */
    bool contains( const char* key );

    /**
     * @brief 设置时间
    */
    void set_time();

    /**
     * @brief 设置关闭
    */
    void set_close();

    /**
     * @brief 设置body值
     * 
     * @param[in] key 关键字
     * @param[in] val 值
    */
    template<typename T>
    bool set( const char* key, T const& val )
    {
        if ( !STRING_CHECK( key ) )
        {
            return false;
        }

        std::stringstream ss;
        ss << val;
        set_str( key, ss.str().c_str() );
        return true;
    }

    /**
     * @brief 获取body值
     * 
     * @param[in] key 关键字
     * @return 关键字对应的值
    */
    template<typename T>
    Optional<T>& get( const char* key )
    {
        std::string str_val;
        if ( !get_str( key, str_val ) )
        {
            return Optional<T>();
        }

        std::stringstream ss;
        ss << str_val;
        T val;
        ss >> val;
        return Optional<T>( val );
    }

PRIVATE: // 私有类型 

    friend struct HttpResponseImpl;

PRIVATE: // 私有函数

    TARO_NO_COPY( HttpResponse );

    /**
     * @brief 设置body值
     * 
     * @param[in] key 关键字
     * @param[in] val 值
    */
    void set_str( const char* key, const char* value );

    /**
     * @brief 获取body值
     * 
     * @param[in]  key 关键字
     * @param[out] val 值
     * @return true 成功  false 失败 
    */
    bool get_str( const char* key, std::string& value );

PRIVATE: // 私有变量

    HttpResponseImpl* impl_;
};

using HttpRequestSPtr  = std::shared_ptr< HttpRequest >;
using HttpResponseSPtr = std::shared_ptr< HttpResponse >;

NAMESPACE_TARO_WS_END
