#include "GetWorkerInfo.h"
#include "HttpParser.h"
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>
#include <muduo/base/Logging.h>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <list>

class GetWorkerInfoTcpClient : private boost::noncopyable
{
public:
    GetWorkerInfoTcpClient(muduo::net::EventLoop *loop,
                           const muduo::net::InetAddress &serverAddr)
        : m_createTime(muduo::Timestamp::now()),
          m_tcpClient(loop, serverAddr, "GetWorkerInfoTcpClient")
    {
        m_tcpClient.setConnectionCallback(boost::bind(
            &GetWorkerInfoTcpClient::OnConnect, this, _1));
        m_tcpClient.setMessageCallback(boost::bind(
            &GetWorkerInfoTcpClient::OnMessage, this, _1, _2, _3));
        m_tcpClient.connect();
    }

    ~GetWorkerInfoTcpClient()
    {
        LOG_INFO << "~GetWorkerInfoTcpClient";
    }

    muduo::Timestamp GetCreateTime()
    {
        return m_createTime;
    }

    void Stop()
    {
        m_tcpClient.stop();
    }

    void OnConnect(const muduo::net::TcpConnectionPtr &conn)
    {
        LOG_INFO << conn->localAddress().toIpPort() << " -> "
                 << conn->peerAddress().toIpPort() << " is "
                 << (conn->connected() ? "UP" : "DOWN");

        if (conn->connected())
        {
            conn->send("GET /api/v1/streams/ HTTP/1.1\r\n\r\n");
            conn->shutdown();
        }
    }

    void OnMessage(const muduo::net::TcpConnectionPtr &conn,
                   muduo::net::Buffer *buff, muduo::Timestamp t)
    {
        if (m_httpParser.Parse(buff->peek(), buff->readableBytes()))
        {
            if (m_httpParser.IsComplete())
            {
                LOG_INFO << m_httpParser.GetBody();
                buff->retrieveAll();
            }
        }
        else
            conn->forceClose();
    }

private:
    muduo::Timestamp m_createTime;
    HttpParser m_httpParser;
    muduo::net::TcpClient m_tcpClient;
};

typedef boost::shared_ptr<GetWorkerInfoTcpClient> GetWorkerInfoTcpClientPtr;
typedef std::list<GetWorkerInfoTcpClientPtr> GetWorkerInfoTcpClientList;

class TcpClientTimeQueue : private boost::noncopyable
{
public:
    TcpClientTimeQueue(muduo::net::EventLoop *loop, int timeout)
        : m_timeout(timeout)
    {
        loop->runEvery(
            1.0, boost::bind(&TcpClientTimeQueue::OnTimer, this));
    }

    ~TcpClientTimeQueue()
    {
        for (GetWorkerInfoTcpClientList::iterator it = m_tcpClientList.begin();
             it != m_tcpClientList.end();)
        {
            (*it)->Stop();
            it = m_tcpClientList.erase(it);
        }
    }

    void AddTcpClient(const GetWorkerInfoTcpClientPtr &tcpClient)
    {
        m_tcpClientList.push_back(tcpClient);
    }

private:

    void OnTimer()
    {
        LOG_INFO << "Queue size = " << m_tcpClientList.size();

        muduo::Timestamp now = muduo::Timestamp::now();
        for (GetWorkerInfoTcpClientList::iterator it = m_tcpClientList.begin();
             it != m_tcpClientList.end();)
        {
            double age = muduo::timeDifference(now, (*it)->GetCreateTime());
            if (age > m_timeout)
            {
                (*it)->Stop();
                it = m_tcpClientList.erase(it);
            }
            else
            {
                break;
            }
        }
    }

    int m_timeout;
    GetWorkerInfoTcpClientList m_tcpClientList;
};

GetWorkerInfo::GetWorkerInfo()
    : m_thread(boost::bind(&GetWorkerInfo::ThreadFunc, this))
{
    m_thread.start();
}

GetWorkerInfo::~GetWorkerInfo()
{
    QueueMessagePtr stop(new(QueueMessage));
    stop->type = MessageType_Command;
    stop->command = Command_Stop;
    Put(stop);
    m_thread.join();
}

void GetWorkerInfo::Put(const QueueMessagePtr &message)
{
    m_queue.put(message);
}

void GetWorkerInfo::OnTimer()
{
    LOG_INFO << "OnTimer";
    if (m_queue.size() > 0)
    {
        GetWorkerInfo::QueueMessagePtr messagePtr = m_queue.take();
        switch (messagePtr->type)
        {
        case MessageType_Command:
        {
            m_loop->quit();
            break;
        }

        case MessageType_Data:
        {
            CreateTcpClientQueue();
            muduo::net::InetAddress serverAddr(
                messagePtr->message.ip, messagePtr->message.port);
            GetWorkerInfoTcpClientPtr tcpClient(
                new GetWorkerInfoTcpClient(m_loop.get(), serverAddr));
            m_tcpClientQueue->AddTcpClient(tcpClient);
            break;
        }

        default:
            break;
        }
    }
}

GetWorkerInfo::QueueMessagePtr GetWorkerInfo::Take()
{
    return m_queue.take();
}

void GetWorkerInfo::ThreadFunc()
{
    m_loop.reset(new(muduo::net::EventLoop));
    m_loop->runEvery(1.0, boost::bind(&GetWorkerInfo::OnTimer, this));
    m_loop->loop();
}

void GetWorkerInfo::CreateTcpClientQueue()
{
    if (!m_tcpClientQueue)
        m_tcpClientQueue.reset(new TcpClientTimeQueue(m_loop.get(), kTcpTimeout));
}
