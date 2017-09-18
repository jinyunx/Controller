#ifndef WORKER_MANAGER
#define WORKER_MANAGER

#include "HttpDispatch.h"

class WorkerManager : private boost::noncopyable
{
public:
    WorkerManager(muduo::net::EventLoop *loop,
                  const muduo::net::InetAddress &listenAddr);
private:

    void HandleStartPush(const muduo::net::HttpRequest &req,
                         muduo::net::HttpResponse *resp);
    void HandleEndPush(const muduo::net::HttpRequest &req,
                       muduo::net::HttpResponse *resp);
    void HandleHeartbeat(const muduo::net::HttpRequest &req,
                         muduo::net::HttpResponse *resp);

    HttpDispatch m_httpDispatch;
};

#endif
