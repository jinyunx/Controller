#ifndef HTTP_DISPATCH
#define HTTP_DISPATCH

#include <muduo/net/EventLoop.h>
#include <muduo/net/http/HttpServer.h>
#include <muduo/net/http/HttpRequest.h>
#include <muduo/net/http/HttpResponse.h>
#include <tr1/functional>
#include <map>
#include <string>

class HttpDispatch : private boost::noncopyable
{
public:
    typedef std::tr1::function < void(const muduo::net::HttpRequest &req,
                                      muduo::net::HttpResponse *resp) > HttpHander;

    HttpDispatch(muduo::net::EventLoop *loop,
                 const muduo::net::InetAddress &listenAddr,
                 const muduo::string &name);

    void Start();
    void AddHander(const std::string &url, const HttpHander &hander);
    void ResponseOk(muduo::net::HttpResponse *resp);
    void ResponseError(muduo::net::HttpResponse *resp);

private:
    typedef std::map<std::string, HttpHander> HanderMap;

    void OnRequest(const muduo::net::HttpRequest &req,
                   muduo::net::HttpResponse *resp);

    HanderMap m_handlers;
    muduo::net::HttpServer m_server;
};

#endif
