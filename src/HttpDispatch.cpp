#include "HttpDispatch.h"
#include <muduo/base/Logging.h>

#define RSP_OK "{\"code\": 0, \"message\": \"\"}\n"
#define RSP_ERROR "{\"code\": 1, \"message\": \"Bad Method\"}\n"
#define RSP_NOTFOUND "{\"code\": 1, \"message\": \"Not Found\"}\n"

HttpDispatch::HttpDispatch(muduo::net::EventLoop *loop,
                           const muduo::net::InetAddress &listenAddr,
                           const muduo::string &name)
    : m_server(loop, listenAddr, name)
{

}

void HttpDispatch::Start()
{
    m_server.setHttpCallback(std::tr1::bind(&HttpDispatch::OnRequest, this,
                             std::tr1::placeholders::_1,
                             std::tr1::placeholders::_2));
    m_server.start();
}

void HttpDispatch::AddHander(const std::string &url,
                             const HttpHander &hander)
{
    m_handlers[url] = hander;
}

void HttpDispatch::ResponseOk(muduo::net::HttpResponse *resp)
{
    resp->setStatusCode(muduo::net::HttpResponse::k200Ok);
    resp->setStatusMessage(RSP_OK);
}

void HttpDispatch::ResponseError(muduo::net::HttpResponse *resp)
{
    resp->setStatusCode(muduo::net::HttpResponse::k400BadRequest);
    resp->setStatusMessage(RSP_ERROR);
    resp->setCloseConnection(true);
}

void HttpDispatch::OnRequest(const muduo::net::HttpRequest &req,
                             muduo::net::HttpResponse *resp)
{
    LOG_INFO << req.methodString() << " " << req.path();
    HanderMap::iterator it = m_handlers.find(req.path().c_str());
    if (it == m_handlers.end())
    {
        resp->setStatusCode(muduo::net::HttpResponse::k404NotFound);
        resp->setStatusMessage(RSP_NOTFOUND);
        resp->setCloseConnection(true);
    }
    else
    {
        it->second(req, resp);
    }
}

