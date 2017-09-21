#ifndef GET_WORKER_INFO
#define GET_WORKER_INFO

#include <muduo/net/EventLoop.h>
#include <muduo/base/Thread.h>
#include <muduo/base/BlockingQueue.h>
#include <muduo/net/TcpClient.h>
#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>

class GetWorkerInfo : private boost::noncopyable
{
public:
    GetWorkerInfo(const muduo::net::InetAddress& serverAddr);
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

private:
    void ThreadFunc();
    QueueMessagePtr Take();

    muduo::BlockingQueue<QueueMessagePtr> m_queue;
    muduo::net::EventLoop loop;
    muduo::net::TcpClient m_tcpClient;
    muduo::Thread m_thread;
};

GetWorkerInfo::GetWorkerInfo(const muduo::net::InetAddress& serverAddr)
    : m_thread(boost::bind(&GetWorkerInfo::ThreadFunc, this)),
    m_tcpClient(&loop, serverAddr, "GetWorkerInfo")
{ }

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

GetWorkerInfo::QueueMessagePtr GetWorkerInfo::Take()
{
    return m_queue.take();
}

void GetWorkerInfo::ThreadFunc()
{
    bool stop = false;
    while (!stop)
    {
        GetWorkerInfo::QueueMessagePtr messagePtr = Take();
        switch (messagePtr->type)
        {
        case MessageType_Command:
            stop = true;
            break;

        case MessageType_Data:
            m_tcpClient.connect();
        default:
            break;
        }
    }
}

#endif
