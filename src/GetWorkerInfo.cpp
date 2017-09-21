//#include "GetWorkerInfo.h"
#include "HttpParser.h"
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpClient.h>
#include <muduo/base/Logging.h>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <list>

class TcpClientBase;
typedef boost::shared_ptr<TcpClientBase> TcpClientBasePtr;
typedef std::list<TcpClientBasePtr> TcpClientBaseList;

class TcpClientBase
{
public:
    TcpClientBase(int idleTimeout)
        : m_inQueue(false),
          m_idleTimeout(idleTimeout),
          m_lastActiveTime(muduo::Timestamp::now())
    { }

    virtual ~TcpClientBase(){}

    bool IsInQueue()
    {
        return m_inQueue;
    }

    void SetInQueue(bool inQueue)
    {
        m_inQueue = inQueue;
    }

    muduo::Timestamp GetLastActiveTime()
    {
        return m_lastActiveTime;
    }

    void SetLastActiveTime(const muduo::Timestamp &t)
    {
        m_lastActiveTime = t;
    }

    double GetIdleTimeout()
    {
        return m_idleTimeout;
    }
    
    void SetPosition(const TcpClientBaseList::iterator &it)
    {
        m_position = it;
    }

    const TcpClientBaseList::iterator & GetPosition()
    {
        return m_position;
    }

    virtual void DoAtTheEnd() = 0;

protected:
    bool m_inQueue;
    int m_idleTimeout;
    muduo::Timestamp m_lastActiveTime;
    TcpClientBaseList::iterator m_position;
};

class TcpClientTimeQueue : private boost::noncopyable
{
public:
    TcpClientTimeQueue(muduo::net::EventLoop *loop)
    {
        loop->runEvery(
            1.0, boost::bind(&TcpClientTimeQueue::OnTimer, this));
    }

    void OnTimer()
    {
        LOG_INFO << "Queue size = " << m_tcpClientList.size();

        muduo::Timestamp now = muduo::Timestamp::now();
        for (TcpClientBaseList::iterator it = m_tcpClientList.begin();
             it != m_tcpClientList.end();)
        {
            double age = muduo::timeDifference(now, (*it)->GetLastActiveTime());
            if (age > (*it)->GetIdleTimeout())
            {
                (*it)->DoAtTheEnd();
                (*it)->SetInQueue(false);
                it = m_tcpClientList.erase(it);
            }
            else
            {
                break;
            }
        }
    }

    void AddTcpClient(const boost::shared_ptr<TcpClientBase> &tcpClient)
    {
        m_tcpClientList.push_back(tcpClient);
        tcpClient->SetPosition(--m_tcpClientList.end());
        tcpClient->SetLastActiveTime(muduo::Timestamp::now());
        tcpClient->SetInQueue(true);
    }

    void RemoveTcpClient(const boost::shared_ptr<TcpClientBase> &tcpClient)
    {
        tcpClient->SetInQueue(false);
        m_tcpClientList.erase(tcpClient->GetPosition());
    }

    void ActiveTcpClient(const boost::shared_ptr<TcpClientBase> &tcpClient)
    {
        m_tcpClientList.splice(m_tcpClientList.end(), m_tcpClientList,
                               tcpClient->GetPosition());
        tcpClient->SetLastActiveTime(muduo::Timestamp::now());
    }

private:
    TcpClientBaseList m_tcpClientList;
};

class GetWorkerInfoTcpClient : public TcpClientBase,
                               public boost::enable_shared_from_this<GetWorkerInfoTcpClient>
{
public:
    GetWorkerInfoTcpClient(muduo::net::EventLoop *loop,
                           const muduo::net::InetAddress &serverAddr,
                           TcpClientTimeQueue *timeQueue,
                           int idleTimeout)
        : TcpClientBase(idleTimeout),
          m_tcpClient(loop, serverAddr, "GetWorkerInfoTcpClient"),
          m_tcpTimeQueue(timeQueue)
    {
        m_tcpClient.setConnectionCallback(boost::bind(
            &GetWorkerInfoTcpClient::OnConnect, shared_from_this(), _1));
        m_tcpClient.setMessageCallback(boost::bind(
            &GetWorkerInfoTcpClient::OnMessage, shared_from_this(), _1, _2, _3));
        m_tcpClient.connect();
        m_tcpTimeQueue->AddTcpClient(shared_from_this());
    }

    ~GetWorkerInfoTcpClient()
    { }

    void OnConnect(const muduo::net::TcpConnectionPtr &conn)
    {
        LOG_INFO << conn->localAddress().toIpPort() << " -> "
                 << conn->peerAddress().toIpPort() << " is "
                 << (conn->connected() ? "UP" : "DOWN");

        if (conn->connected())
        {
            conn->send("GET / HTTP/1.1");
            m_tcpTimeQueue->ActiveTcpClient(shared_from_this());
        }
        else if (!conn->connected() && m_inQueue)
            m_tcpTimeQueue->RemoveTcpClient(shared_from_this());
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
        {
            conn->shutdown();
            conn->forceClose();
            m_tcpTimeQueue->RemoveTcpClient(shared_from_this());
        }
    }

    virtual void DoAtTheEnd()
    { }

private:
    HttpParser m_httpParser;
    muduo::net::TcpClient m_tcpClient;
    TcpClientTimeQueue *m_tcpTimeQueue;
};

class GetWorkerInfoTcpClient2 : public boost::enable_shared_from_this<GetWorkerInfoTcpClient2>
{
public:
    GetWorkerInfoTcpClient2(muduo::net::EventLoop *loop,
                           const muduo::net::InetAddress &serverAddr,
                           int idleTimeout)
    {

    }

    ~GetWorkerInfoTcpClient2()
    {
        LOG_INFO << "~GetWorkerInfoTcpClient2";
    }

    void Start(muduo::net::EventLoop *loop,
               const muduo::net::InetAddress &serverAddr,
               int idleTimeout)
    {
        m_tcpClient.reset(new muduo::net::TcpClient(loop, serverAddr, "GetWorkerInfoTcpClient2"));

        m_tcpClient->setConnectionCallback(boost::bind(
            &GetWorkerInfoTcpClient2::OnConnect, shared_from_this(), _1));
        m_tcpClient->setMessageCallback(boost::bind(
            &GetWorkerInfoTcpClient2::OnMessage, shared_from_this(), _1, _2, _3));
        m_tcpClient->connect();

        m_tcpClient->getLoop()->runEvery(1.0, boost::bind(&GetWorkerInfoTcpClient2::OnTimer,
            shared_from_this()));
    }

    void OnConnect(const muduo::net::TcpConnectionPtr &conn)
    {
        LOG_INFO << conn->localAddress().toIpPort() << " -> "
            << conn->peerAddress().toIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");

        if (conn->connected())
        {
            conn->setContext(new HttpParser);
            conn->send("GET / HTTP/1.1\r\n\r\n");
        }
    }

    void OnMessage(const muduo::net::TcpConnectionPtr &conn,
                   muduo::net::Buffer *buff, muduo::Timestamp t)
    {
        HttpParser *m_httpParser = boost::any_cast<HttpParser *>(conn->getContext());
        if (m_httpParser->Parse(buff->peek(), buff->readableBytes()))
        {
            if (m_httpParser->IsComplete())
            {
                LOG_INFO << m_httpParser->GetBody();
                buff->retrieveAll();
            }
        }
        else
        {
            conn->shutdown();
            conn->forceClose();
        }
    }

    void OnTimer()
    {
        LOG_INFO << "Timer running";
    }

    virtual void DoAtTheEnd()
    { }

private:
    boost::shared_ptr<muduo::net::TcpClient> m_tcpClient;
};

int main()
{
    muduo::net::EventLoop loop;
    muduo::net::InetAddress serverAddr(80);

    {
        boost::shared_ptr<GetWorkerInfoTcpClient2> tcpClient(
            new GetWorkerInfoTcpClient2(&loop, serverAddr, 1));
        tcpClient->Start(&loop, serverAddr, 1);
        tcpClient->Start(&loop, serverAddr, 1);
        tcpClient->Start(&loop, serverAddr, 1);
        tcpClient->Start(&loop, serverAddr, 1);
    }

    //TcpClientTimeQueue timequeue(&loop);

    //{
    //    boost::shared_ptr<GetWorkerInfoTcpClient> tcpClient(
    //        new GetWorkerInfoTcpClient(&loop, serverAddr, &timequeue, 1));
    //}

    loop.loop();
    return 0;
}

