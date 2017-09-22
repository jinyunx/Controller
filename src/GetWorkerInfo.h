#ifndef GET_WORKER_INFO
#define GET_WORKER_INFO

#include <muduo/net/EventLoop.h>
#include <muduo/base/Thread.h>
#include <muduo/base/BlockingQueue.h>
#include <muduo/net/TcpClient.h>
#include <boost/noncopyable.hpp>
#include <boost/bind.hpp>

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

private:
    static const int kTcpTimeout = 1;

    void ThreadFunc();
    void OnTimer();
    QueueMessagePtr Take();
    void CreateTcpClientQueue();


    muduo::BlockingQueue<QueueMessagePtr> m_queue;
    boost::shared_ptr<muduo::net::EventLoop> m_loop;
    boost::shared_ptr<TcpClientTimeQueue> m_tcpClientQueue;
    muduo::Thread m_thread;
};

#endif
