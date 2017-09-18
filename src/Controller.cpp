#include "WorkerManager.h"
#include <muduo/net/EventLoop.h>

int main(int argc, char* argv[])
{
    muduo::net::EventLoop loop;
    WorkerManager workManager(
        &loop, muduo::net::InetAddress(8000));
    loop.loop();
    return 0;
}
