#ifndef WORKER_MANAGER
#define WORKER_MANAGER

#include "HttpDispatch.h"
#include <muduo/net/TcpClient.h>

class WorkerManager : private boost::noncopyable
{
public:
    WorkerManager(muduo::net::EventLoop *loop,
                  const muduo::net::InetAddress &listenAddr);
private:
    typedef boost::shared_ptr<muduo::net::TcpClient> TcpClientPtr;

    void HandleStartPush(const boost::shared_ptr<HttpRequester> &req,
                         boost::shared_ptr<HttpResponser> &resp);
    void HandleEndPush(const boost::shared_ptr<HttpRequester> &req,
                       boost::shared_ptr<HttpResponser> &resp);
    void HandleHeartbeat(const boost::shared_ptr<HttpRequester> &req,
                         boost::shared_ptr<HttpResponser> &resp);

    static const int kTimeoutSecond = 5;

    HttpDispatch m_httpDispatch;
    muduo::net::TcpClient m_tcpPolling;
};

#endif
