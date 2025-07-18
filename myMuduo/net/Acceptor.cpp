#include "myMuduo/net/Acceptor.h"
#include <unistd.h>
#include "myMuduo/net/EventLoop.h"
#include "myMuduo/net/InetAddress.h"
#include "spdlog/spdlog.h"

namespace myMuduo {
namespace net {

int createNonblockingOrDie(sa_family_t family)
{
    int sockfd = socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        spdlog::critical("createNonblockingOrDie error: {}", strerror(errno));
        abort();
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonblockingOrDie(listenAddr.family()))
    , acceptChannel_(loop, acceptSocket_.fd())
    , listening_(false)
{
    acceptSocket_.setReuseAddr(true);
    //用于多进程的端口复用
    //如果有多个进程监听同一个端口，只有一个进程能接收到新连接
    acceptSocket_.setReusePort(reuseport);
    acceptSocket_.bindAddress(listenAddr);
    acceptChannel_.setReadCallback([&](Timestamp receiveTime) { this->handleRead(receiveTime); });
}

Acceptor::~Acceptor()
{
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}

void Acceptor::listen()
{
    loop_->assertInLoopThread();
    listening_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}

//处理新连接
//epoll默认是LT模式 直到事件处理完才会再次阻塞
//LT 模式下，如果一个事件没有被一次性处理完毕，epoll_wait 会在后续的调用中持续地提醒你，直到该事件被处理完毕（即触发条件不再满足）
//handleRead调用1次只能处理1个新连接
//如果有多个新连接，handleRead会被调用多次 所以不用while一直accept
//用while一直读也可以，事件处理完后break就行了
//     if (errno == EAGAIN || errno == EWOULDBLOCK)
//     {
//         // 所有等待的连接都已处理完毕
//          break;
//     }
void Acceptor::handleRead(Timestamp receiveTime)
{
    loop_->assertInLoopThread();
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        if (newConnectionCallback_)
        {
            spdlog::debug("New connection accepted: fd={}, peer address={} Time={}", connfd,
                          peerAddr.toIpPort(), receiveTime.toString());
            newConnectionCallback_(connfd, peerAddr);
        }
        else
        {
            spdlog::warn("New connection callback is not set, closing connection fd: {}", connfd);
            close(connfd);
        }
    }
    else
    {
        spdlog::error("Accept error: {}", strerror(errno));
        if (errno == EMFILE)
        {
            spdlog::warn("socket fd limit reached, consider increasing the limit");
        }
    }
}

}  // namespace net
}  // namespace myMuduo