#include "HttpDispatch.h"
#include <muduo/base/Logging.h>

#define RSP_OK "{\"code\": 0, \"message\": \"\"}\n"
#define RSP_ERROR "{\"code\": 1, \"message\": \"Bad Method\"}\n"
#define RSP_NOTFOUND "{\"code\": 1, \"message\": \"Not Found\"}\n"

HttpDispatch::HttpDispatch(int timeoutSecond,
                           muduo::net::EventLoop *loop,
                           const muduo::net::InetAddress &listenAddr,
                           const muduo::string &name)
    : m_server(timeoutSecond, loop, listenAddr, name)
{ }

void HttpDispatch::Start()
{
    m_server.SetHttpCallback(std::tr1::bind(&HttpDispatch::OnRequest, this,
                             std::tr1::placeholders::_1,
                             std::tr1::placeholders::_2));
    m_server.Start();
}

void HttpDispatch::AddHander(const std::string &url,
                             const HttpHander &hander)
{
    m_handlers[url] = hander;
}

void HttpDispatch::ResponseOk(boost::shared_ptr<HttpResponser> &resp)
{
    resp->SetStatusCode(HttpResponser::StatusCode_200Ok);
    resp->SetBody(RSP_OK);
}

void HttpDispatch::ResponseError(boost::shared_ptr<HttpResponser> &resp)
{
    resp->SetStatusCode(HttpResponser::StatusCode_400BadRequest);
    resp->SetBody(RSP_ERROR);
    resp->SetCloseConnection(true);
}

void HttpDispatch::OnRequest(const boost::shared_ptr<HttpRequester> &req,
                             boost::shared_ptr<HttpResponser> &resp)
{
    LOG_INFO << req->GetMethod() << " " << req->GetUrl();
    HanderMap::iterator it = m_handlers.find(req->GetUrl().c_str());
    if (it == m_handlers.end())
    {
        resp->SetStatusCode(HttpResponser::StatusCode_404NotFound);
        resp->SetBody(RSP_NOTFOUND);
        resp->SetCloseConnection(true);
    }
    else
    {
        it->second(req, resp);
    }
}

