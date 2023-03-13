
#include "http_proto.h"
#include "impl/http_proto_impl.h"
#include "base/utils/string_tool.h"

NAMESPACE_TARO_WS_BEGIN

inline std::string get_gmt_time()
{
    time_t raw_time = time( NULL );
    time( &raw_time );
    tm* time_info = gmtime( &raw_time );
    char temp[32] = { 0 };
    strftime( temp, sizeof( temp ), "%a, %d %b %Y %H:%M:%S GMT", time_info );
    return std::string( temp );
}

HttpRequest::HttpRequest()
    : impl_( new HttpRequestImpl )
{

}

HttpRequest::HttpRequest( const char* method, const char* url, const char* version )
    : impl_( new HttpRequestImpl )
{
    TARO_ASSERT( STRING_CHECK( method, url, version ) );

    impl_->method_  = method;
    impl_->url_     = url;
    impl_->version_ = version;
}

HttpRequest::HttpRequest( HttpRequest&& other )
    : impl_( other.impl_ )
{
    other.impl_ = nullptr;
}

HttpRequest::~HttpRequest()
{
    if ( nullptr == impl_ )
        return;
    delete impl_;
}

bool HttpRequest::valid() const
{
    return impl_ != nullptr;
}

const char* HttpRequest::version() const
{
    return impl_->version_.empty() ? nullptr : impl_->version_.c_str();
}

const char* HttpRequest::method() const
{
    return impl_->method_.empty() ? nullptr : impl_->method_.c_str();
}

const char* HttpRequest::url() const
{
    return impl_->url_.empty() ? nullptr : impl_->url_.c_str();
}

void HttpRequest::set_time()
{
    set_str( "Date", get_gmt_time().c_str() );
}

void HttpRequest::set_close()
{
    set_str( "Connection", "Close" );
}

void HttpRequest::set_boundary( const char* boundary )
{
    set_str( "Content-Type", ( std::string( "multipart/form-data; boundary=" ) + boundary ).c_str() );
}

bool HttpRequest::contains( const char* key )
{
    if ( !STRING_CHECK( key ) )
    {
        return false;
    }

    auto it = std::find_if( impl_->body_items_.begin(), impl_->body_items_.end(), [&]( BodyItem const& item )
    {
        return string_compare( item.key, key );
    } );
    return it != impl_->body_items_.end();
}

void HttpRequest::set_str( const char* key, const char* value )
{
    TARO_ASSERT( STRING_CHECK( key, value ) );

    auto it = std::find_if( impl_->body_items_.begin(), impl_->body_items_.end(), [&]( BodyItem const& item )
    {
        return string_compare( item.key, key );
    } );

    if ( it != impl_->body_items_.end() )
    {
        it->value = value;
    }
    else
    {
        impl_->body_items_.emplace_back( BodyItem{ key, value } );
    }
}

bool HttpRequest::get_str( const char* key, std::string& value )
{
    if ( !STRING_CHECK( key ) )
    {
        return false;
    }

    auto it = std::find_if( impl_->body_items_.begin(), impl_->body_items_.end(), [&]( BodyItem const& item )
    {
        return string_compare( item.key, key );
    } );

    if ( it == impl_->body_items_.end() )
    {
        return false;
    }
    value = it->value;
    return true;
}

HttpResponse::HttpResponse( int32_t code, const char* version )
    : impl_( new HttpResponseImpl )
{
    impl_->code_    = code;
    impl_->state_   = HttpRespState::message( code );
    impl_->version_ = version;
}

HttpResponse::HttpResponse( int32_t code, const char* status, const char* version )
    : impl_( new HttpResponseImpl )
{
    impl_->code_    = code;
    impl_->state_   = status;
    impl_->version_ = version;
}

HttpResponse::HttpResponse( HttpResponse&& other )
    : impl_( other.impl_ )
{
    other.impl_ = nullptr;
}

HttpResponse::~HttpResponse()
{
    if ( nullptr == impl_ )
        return;
    delete impl_;
}

bool HttpResponse::valid() const
{
    return impl_ != nullptr;
}

const char* HttpResponse::version() const
{
    return impl_->version_.empty() ? nullptr : impl_->version_.c_str();
}

const char* HttpResponse::status() const
{
    return impl_->state_.empty() ? nullptr : impl_->state_.c_str();
}

int32_t HttpResponse::code() const
{
    return impl_->code_;
}

void HttpResponse::set_boundary( const char* boundary )
{
    set_str( "Content-Type", ( std::string( "multipart/form-data; boundary=" ) + boundary ).c_str() );
}

void HttpResponse::set_time()
{
    set_str( "Date", get_gmt_time().c_str() );
}

void HttpResponse::set_close()
{
    set_str( "Connection", "Close" );
}

bool HttpResponse::contains( const char* key )
{
    if ( !STRING_CHECK( key ) )
    {
        return false;
    }

    auto it = std::find_if( impl_->body_items_.begin(), impl_->body_items_.end(), [&]( BodyItem const& item )
    {
        return string_compare( item.key, key );
    } );
    return it != impl_->body_items_.end();
}

void HttpResponse::set_str( const char* key, const char* value )
{
    TARO_ASSERT( STRING_CHECK( key, value ) );

    auto it = std::find_if( impl_->body_items_.begin(), impl_->body_items_.end(), [&]( BodyItem const& item )
    {
        return string_compare( item.key, key );
    } );

    if ( it != impl_->body_items_.end() )
    {
        it->value = value;
    }
    else
    {
        impl_->body_items_.emplace_back( BodyItem{ key, value } );
    }
}

bool HttpResponse::get_str( const char* key, std::string& value )
{
    if ( !STRING_CHECK( key ) )
    {
        return false;
    }

    auto it = std::find_if( impl_->body_items_.begin(), impl_->body_items_.end(), [&]( BodyItem const& item )
    {
        return string_compare( item.key, key );
    } );

    if ( it == impl_->body_items_.end() )
    {
        return false;
    }
    value = it->value;
    return true;
}

NAMESPACE_TARO_WS_END
