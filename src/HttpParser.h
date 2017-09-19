#ifndef HTTP_PARSER
#define HTTP_PARSER

#include "../3rd/http-parser/http_parser.h"
#include <boost/noncopyable.hpp>
#include <string>
#include <map>

class HttpParser : private boost::noncopyable
{
public:
    typedef std::map<std::string, std::string> HeaderMap;

    HttpParser();

    bool Parse(const char *data, size_t len);
    bool IsComplete();
    void Reset();

    const std::string & GetUrl();
    const std::string & GetMethod();
    const std::string & GetBody();
    const HeaderMap & GetHeaders();
    const char * GetHeader(const std::string &name);

private:
    enum HeaderState
    {
        HeaderState_None = 0,
        HeaderState_HasName = 0x01,
        HeaderState_HasVaule = 0x10,
        HeaderState_HasAll = 0x11,
    };

    bool HeaderReady();

    static int OnUrl(http_parser* parser, const char* at, size_t length);
    static int OnHeaderName(http_parser* parser, const char* at, size_t length);
    static int OnHeaderValue(http_parser* parser, const char* at, size_t length);
    static int OnBody(http_parser* parser, const char* at, size_t length);
    static int OnComplete(http_parser* parser);

    static http_parser_settings m_settings;

    std::string m_url;
    std::string m_method;
    HeaderState m_headerState;
    std::string m_headerName;
    std::string m_headerValue;
    HeaderMap m_headers;
    std::string m_body;
    bool m_complete;

    http_parser m_httpParser;
};

#endif
