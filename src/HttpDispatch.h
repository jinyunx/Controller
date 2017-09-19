#ifndef HTTP_DISPATCH
#define HTTP_DISPATCH

#include "HttpServer.h"
#include <muduo/net/EventLoop.h>
#include <tr1/functional>
#include <map>
#include <string>

class HttpDispatch : private boost::noncopyable
{
public:
    typedef std::tr1::function < void(const boost::shared_ptr<HttpRequester> &,
                                      boost::shared_ptr<HttpResponser> &) > HttpHander;

    HttpDispatch(int timeoutSecond, 
                 muduo::net::EventLoop *loop,
                 const muduo::net::InetAddress &listenAddr,
                 const muduo::string &name);

    void Start();
    void AddHander(const std::string &url, const HttpHander &hander);
    void ResponseOk(boost::shared_ptr<HttpResponser> &resp);
    void ResponseError(boost::shared_ptr<HttpResponser> &resp);

private:
    typedef std::map<std::string, HttpHander> HanderMap;

    void OnRequest(const boost::shared_ptr<HttpRequester> &req,
                   boost::shared_ptr<HttpResponser> &resp);

    HanderMap m_handlers;
    HttpServer m_server;
};

#endif
