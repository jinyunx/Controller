#ifndef HTTP_SERVER
#define HTTP_SERVER

#include "HttpRequester.h"
#include "HttpResponser.h"

#include <muduo/base/Logging.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>

#include <stdio.h>

class TcpIdleDetector : public muduo::copyable
{
public:
    typedef boost::weak_ptr<muduo::net::TcpConnection> WeakTcpConnectionPtr;

    struct Entry : public muduo::copyable
    {
        explicit Entry(const WeakTcpConnectionPtr& weakConn)
            : m_weakConn(weakConn)
        { }

        ~Entry()
        {
            muduo::net::TcpConnectionPtr conn = m_weakConn.lock();
            if (conn)
            {
                conn->shutdown();
                conn->forceCloseWithDelay(kRoundTripTime);
            }
        }

        static const int kRoundTripTime = 4;

        WeakTcpConnectionPtr m_weakConn;
    };

    typedef boost::weak_ptr<Entry> WeakEntryPtr;

    TcpIdleDetector(int timeoutSecond,
                    muduo::net::EventLoop* loop)
        : m_connectionBuckets(timeoutSecond)
    {
        m_connectionBuckets.resize(timeoutSecond);
        loop->runEvery(1.0, boost::bind(&TcpIdleDetector::OnTimer, this));
    }

    WeakEntryPtr NewEntry(const muduo::net::TcpConnectionPtr &conn)
    {
        EntryPtr entry(new Entry(conn));
        m_connectionBuckets.back().insert(entry);
        return WeakEntryPtr(entry);
    }

    void FreshEntry(const WeakEntryPtr &weakEntry)
    {
        EntryPtr entry(weakEntry.lock());
        if (entry)
            m_connectionBuckets.back().insert(entry);
    }

private:
    typedef boost::shared_ptr<Entry> EntryPtr;
    typedef boost::unordered_set<EntryPtr> Bucket;
    typedef boost::circular_buffer<Bucket> WeakConnectionList;

    void OnTimer()
    {
        m_connectionBuckets.push_back(Bucket());
        //dumpConnectionBuckets();
    }

    void dumpConnectionBuckets() const
    {
        LOG_INFO << "size = " << m_connectionBuckets.size();
        int idx = 0;
        for (WeakConnectionList::const_iterator bucketI = m_connectionBuckets.begin();
             bucketI != m_connectionBuckets.end();
             ++bucketI, ++idx)
        {
            const Bucket& bucket = *bucketI;
            printf("[%d] len = %zd : ", idx, bucket.size());
            for (Bucket::const_iterator it = bucket.begin();
                 it != bucket.end();
                 ++it)
            {
                bool connectionDead = (*it)->m_weakConn.expired();
                printf("%p(%ld)%s, ", get_pointer(*it), it->use_count(),
                       connectionDead ? " DEAD" : "");
            }
            puts("");
        }
    }

    WeakConnectionList m_connectionBuckets;
};

class HttpServer
{
public:
    typedef boost::function<void(const boost::shared_ptr<HttpRequester> &,
                                 boost::shared_ptr<HttpResponser> &)> HttpCallback;

    HttpServer(int timeoutSecond,
               muduo::net::EventLoop* loop,
               const muduo::net::InetAddress &listenAddr,
               const muduo::string &name);

    void SetHttpCallback(const HttpCallback &httpCallback);
    void Start();
private:
    void OnConnection(const muduo::net::TcpConnectionPtr &conn);

    void OnMessage(const muduo::net::TcpConnectionPtr &conn,
                   muduo::net::Buffer* buf,
                   muduo::Timestamp time);

    void OnRequest(const muduo::net::TcpConnectionPtr &conn,
                   const boost::shared_ptr<HttpRequester> &httpRequester);

    HttpCallback m_httpCallback;
    TcpIdleDetector m_idleDector;
    muduo::net::TcpServer m_server;
};

#endif
