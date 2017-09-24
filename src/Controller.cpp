#include "WorkerManager.h"
#include <muduo/net/EventLoop.h>
#include <unistd.h>
#include <signal.h>

static void SignalHandler(int signal)
{
    switch (signal)
    {
    case SIGQUIT:
    case SIGTERM:
    case SIGINT:
    {
        LOG_ERROR << "Stop now";
        exit(0);
        break;
    }
    default:
        break;
    }
}

void InstallSigHandler()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = SignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGQUIT, &sa, NULL);

    sa.sa_handler = SignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_handler = SignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
}

int main(int argc, char* argv[])
{
    daemon(1, 1);

    InstallSigHandler();
    muduo::net::EventLoop loop;
    WorkerManager workManager(
        &loop, muduo::net::InetAddress(8085));
    loop.loop();
    return 0;
}
