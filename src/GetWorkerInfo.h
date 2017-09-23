#ifndef GET_WORKER_INFO
#define GET_WORKER_INFO

#include <muduo/net/EventLoop.h>
#include <muduo/base/Thread.h>
#include <muduo/base/BlockingQueue.h>
#include <muduo/net/TcpClient.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Logging.h>
#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>
#include <map>
#include <string>

class WorkerInfo
{
public:
    struct StreamInfo
    {
        bool active;
        int clientsNum;
        std::string name;
        std::string app;
        std::string ip;

        StreamInfo()
            : active(false), clientsNum(0)
        { }
    };

    struct IpInfo
    {
        muduo::Timestamp t;
        int activeStreamNum;

        IpInfo()
            : t(muduo::Timestamp::now()), activeStreamNum(0)
        { }
    };

    bool TakeToPlay(const std::string &app,
                    const std::string &name,
                    StreamInfo &info) const
    {
        // FIXME: when the worker was dead, and the streams in that worker would not
        // refresh, so client may get the dead stream url to play, but it can try to play
        muduo::MutexLockGuard lock(m_mutex);

        StreamInfoMap::const_iterator it = m_streamInfo.find(app + "_" + name);
        if (it != m_streamInfo.end())
        {
            info = it->second;
            return true;
        }
        return false;
    }

    bool TakeToPublish(std::string &ip) const
    {
        muduo::MutexLockGuard lock(m_mutex);
        std::string bestIp;
        std::string worseIp;
        int bestStreamNum = kMaxStreamInOneIp + 1;
        int worseStreamNum = bestStreamNum;

        IpInfoMap::const_iterator it = m_ipInfo.begin();
        for (; it != m_ipInfo.end(); ++it)
        {
            double diff = muduo::timeDifference(muduo::Timestamp::now(), it->second.t);
            // Best
            if (diff <= kActiveTime && it->second.activeStreamNum < bestStreamNum)
            {
                bestIp = it->first;
                bestStreamNum = it->second.activeStreamNum;
            }
            // Worse but not dead
            else if (diff > kActiveTime && diff <= kInactiveTime &&
                it->second.activeStreamNum < worseStreamNum)
            {
                worseIp = it->first;
                worseStreamNum = it->second.activeStreamNum;
            }
        }

        if (!bestIp.empty())
        {
            ip = bestIp;
            return true;
        }
        else if (!worseIp.empty())
        {
            ip = worseIp;
            return true;
        }
        return false;
    }

    void Put(const StreamInfo &info, bool first)
    {
        muduo::MutexLockGuard lock(m_mutex);

        if (first)
            LOG_INFO << "Fresh worker " << info.ip << " info";

        // Only save the active streams
        if (info.active)
            m_streamInfo[info.app + "_" + info.name] = info;

        IpInfo &ipInfo = m_ipInfo[info.ip];
        ipInfo.t = muduo::Timestamp::now();
        if (first)
            ipInfo.activeStreamNum = info.active ? 1 : 0;
        else
            ipInfo.activeStreamNum += info.active ? 1 : 0;
    }

private:
    typedef std::map<std::string, StreamInfo> StreamInfoMap;
    typedef std::map<std::string, IpInfo> IpInfoMap;

    // 0~20 is active, 20~60 is worse, 60~- is dead
    const static int kActiveTime = 20;
    const static int kInactiveTime = 60;

    const static int kMaxStreamInOneIp = 500;

    mutable muduo::MutexLock m_mutex;
    StreamInfoMap m_streamInfo;
    IpInfoMap m_ipInfo;
};

class TcpClientTimeQueue;
class GetWorkerInfo : private boost::noncopyable
{
public:
    GetWorkerInfo();
    ~GetWorkerInfo();

    enum MessageType
    {
        MessageType_Data,
        MessageType_Command,
    };

    struct Message
    {
        std::string ip;
        unsigned short port;
        Message()
            : port(0)
        { }
    };

    enum Command
    {
        Command_None,
        Command_Stop,
    };

    struct QueueMessage
    {
        MessageType type;
        Message message;
        Command command;
        QueueMessage()
            : type(MessageType_Data), command(Command_None)
        { }
    };

    typedef boost::shared_ptr<QueueMessage> QueueMessagePtr;

    void Put(const QueueMessagePtr &message);
    bool GetPublishIp(std::string &ip) const;
    bool GetSteamToPlay(const std::string &app,
                        const std::string &name,
                        WorkerInfo::StreamInfo &info) const;

private:
    static const int kTcpTimeout = 1;

    void ThreadFunc();
    void OnTimer();
    QueueMessagePtr Take();
    void CreateTcpClientQueue();

    WorkerInfo m_workerInfo;
    muduo::BlockingQueue<QueueMessagePtr> m_queue;
    boost::shared_ptr<muduo::net::EventLoop> m_loop;
    boost::shared_ptr<TcpClientTimeQueue> m_tcpClientQueue;
    muduo::Thread m_thread;
};

#endif
