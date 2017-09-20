#ifndef HTTP_REQUESTER
#define HTTP_REQUESTER

#include "HttpParser.h"

class HttpRequester : private boost::noncopyable
{
public:
    HttpRequester()
    { }

    ~HttpRequester()
    { }

    bool Parse(const char *data, size_t len)
    {
        return m_httpParser.Parse(data, len);
    }

    bool IsComplete()
    {
        return m_httpParser.IsComplete();
    }

    void Reset()
    {
        m_httpParser.Reset();
    }

    const std::string & GetUrl()
    {
        return m_httpParser.GetUrl();
    }

    const std::string & GetMethod()
    {
        return m_httpParser.GetMethod();
    }

    const std::string & GetBody()
    {
        return m_httpParser.GetBody();
    }

    const HttpParser::HeaderMap & GetHeaders()
    {
        return m_httpParser.GetHeaders();
    }

    const char * GetHeader(const std::string &name)
    {
        return m_httpParser.GetHeader(name);
    }

    void SetPeerIp(const std::string &peerIp)
    {
        m_peerIp = peerIp;
    }

    void SetPeerPort(unsigned short peerPort)
    {
        m_peerPort = peerPort;
    }

    const std::string & GetPeerIp()
    {
        return m_peerIp;
    }

    unsigned short GetPeerPort()
    {
        return m_peerPort;
    }

private:
    HttpParser m_httpParser;
    std::string m_peerIp;
    unsigned short m_peerPort;
};

#endif
