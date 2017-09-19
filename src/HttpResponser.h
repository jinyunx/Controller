#ifndef HTTP_RESPONSER
#define HTTP_RESPONSER

#include <muduo/net/Buffer.h>
#include <boost/noncopyable.hpp>
#include <stdio.h>
#include <map>

class HttpResponser : private boost::noncopyable
{
public:
    enum StatusCode
    {
        StatusCode_Unknown,
        StatusCode_200Ok = 200,
        StatusCode_301MovedPermanently = 301,
        StatusCode_400BadRequest = 400,
        StatusCode_404NotFound = 404,
    };

    explicit HttpResponser(bool close)
        : m_statusCode(StatusCode_Unknown),
          m_closeConnection(close)
    { }

    void SetStatusCode(StatusCode code)
    {
        m_statusCode = code;
    }

    void SetStatusMessage(const muduo::string &message)
    {
        m_statusMessage = message;
    }

    void SetCloseConnection(bool on)
    {
        m_closeConnection = on;
    }

    bool CloseConnection() const
    {
        return m_closeConnection;
    }

    void SetContentType(const muduo::string &contentType)
    {
        AddHeader("Content-Type", contentType);
    }

    void AddHeader(const muduo::string &key, const muduo::string &value)
    {
        m_headers[key] = value;
    }

    void SetBody(const muduo::string &body)
    {
        m_body = body;
    }

    void AppendToBuffer(muduo::net::Buffer* output) const
    {
        char buf[32];
        snprintf(buf, sizeof buf, "HTTP/1.1 %d ", m_statusCode);
        output->append(buf);
        output->append(m_statusMessage);
        output->append("\r\n");

        if (m_closeConnection)
        {
            output->append("Connection: close\r\n");
        }
        else
        {
            snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", m_body.size());
            output->append(buf);
            output->append("Connection: Keep-Alive\r\n");
        }

        for (std::map<muduo::string, muduo::string>::const_iterator it = m_headers.begin();
             it != m_headers.end();
             ++it)
        {
            output->append(it->first);
            output->append(": ");
            output->append(it->second);
            output->append("\r\n");
        }

        output->append("\r\n");
        output->append(m_body);
    }

private:
    std::map<muduo::string, muduo::string> m_headers;
    StatusCode m_statusCode;
    muduo::string m_statusMessage;
    bool m_closeConnection;
    muduo::string m_body;
};

#endif
