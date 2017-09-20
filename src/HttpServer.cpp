#include "HttpServer.h"

struct HttpContext
{
    TcpIdleDetector::WeakEntryPtr weakEntryptr;
    boost::shared_ptr<HttpRequester> httpRequester;
    HttpContext(TcpIdleDetector::WeakEntryPtr WeakPtr,
                boost::shared_ptr<HttpRequester> requester)
        : weakEntryptr(WeakPtr), httpRequester(requester)
    { }
};

HttpServer::HttpServer(int timeoutSecond,
                       muduo::net::EventLoop* loop,
                       const muduo::net::InetAddress &listenAddr,
                       const muduo::string &name)
    : m_idleDector(timeoutSecond, loop),
      m_server(loop, listenAddr, name)
{
    m_server.setConnectionCallback(
        boost::bind(&HttpServer::OnConnection, this, _1));
    m_server.setMessageCallback(
        boost::bind(&HttpServer::OnMessage, this, _1, _2, _3));
}

void HttpServer::SetHttpCallback(const HttpCallback &httpCallback)
{
    m_httpCallback = httpCallback;
}

void HttpServer::Start()
{
    m_server.start();
}

void HttpServer::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    LOG_INFO << m_server.name() << " - " << conn->peerAddress().toIpPort() << " -> "
             << conn->localAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN");

    if (conn->connected())
    {
        boost::shared_ptr<HttpRequester> httpRequester(new HttpRequester);
        httpRequester->SetPeerIp(conn->peerAddress().toIp().c_str());
        httpRequester->SetPeerPort(conn->peerAddress().toPort());
        HttpContext httpContext(m_idleDector.NewEntry(conn), httpRequester);
        conn->setContext(httpContext);
    }
}

void HttpServer::OnMessage(const muduo::net::TcpConnectionPtr &conn,
                           muduo::net::Buffer* buf,
                           muduo::Timestamp time)
{
    HttpContext httpContext = boost::any_cast<HttpContext>(conn->getContext());
    m_idleDector.FreshEntry(httpContext.weakEntryptr);
    if (httpContext.httpRequester->Parse(buf->peek(), buf->readableBytes()))
    {
        if (httpContext.httpRequester->IsComplete())
        {
            OnRequest(conn, httpContext.httpRequester);
            httpContext.httpRequester->Reset();
            buf->retrieveAll();
        }
    }
    else
    {
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }
}

void HttpServer::OnRequest(const muduo::net::TcpConnectionPtr &conn,
                           const boost::shared_ptr<HttpRequester> &httpRequester)
{
    bool close = false;
    std::string closeHead = httpRequester->GetHeader("Connection");
    if (closeHead == "close")
        close = true;
    boost::shared_ptr<HttpResponser> reponser(new HttpResponser(close));

    if (m_httpCallback)
    {
        m_httpCallback(httpRequester, reponser);
        muduo::net::Buffer buf;
        reponser->AppendToBuffer(&buf);
        conn->send(&buf);
        if (reponser->CloseConnection())
            conn->shutdown();
    }
    else
    {
        conn->send("HTTP/1.1 404 Not Found\r\n\r\n");
        conn->shutdown();
    }

}
