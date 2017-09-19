#include "WorkerManager.h"
#include <muduo/base/Logging.h>

WorkerManager::WorkerManager(muduo::net::EventLoop *loop,
                             const muduo::net::InetAddress &listenAddr)
    : m_httpDispatch(kTimeoutSecond, loop, listenAddr, "WorkerManager")
{
    m_httpDispatch.AddHander("/start",
                             std::tr1::bind(&WorkerManager::HandleStartPush,
                                            this, std::tr1::placeholders::_1,
                                            std::tr1::placeholders::_2));

    m_httpDispatch.AddHander("/end",
                             std::tr1::bind(&WorkerManager::HandleEndPush,
                                            this, std::tr1::placeholders::_1,
                                            std::tr1::placeholders::_2));

    m_httpDispatch.AddHander("/heartbeat",
                             std::tr1::bind(&WorkerManager::HandleHeartbeat,
                                            this, std::tr1::placeholders::_1,
                                            std::tr1::placeholders::_2));
    m_httpDispatch.Start();
}

void WorkerManager::HandleStartPush(const boost::shared_ptr<HttpRequester> &req,
                                    boost::shared_ptr<HttpResponser> &resp)
{
    LOG_INFO << "HandleStartPush";
    m_httpDispatch.ResponseOk(resp);
}

void WorkerManager::HandleEndPush(const boost::shared_ptr<HttpRequester> &req,
                                  boost::shared_ptr<HttpResponser> &resp)
{
    LOG_INFO << "HandleEndPush";
    m_httpDispatch.ResponseOk(resp);
}

void WorkerManager::HandleHeartbeat(const boost::shared_ptr<HttpRequester> &req,
                                    boost::shared_ptr<HttpResponser> &resp)
{
    LOG_INFO << "HandleHeartbeat";
    LOG_INFO << req->GetBody();
    m_httpDispatch.ResponseOk(resp);
}
