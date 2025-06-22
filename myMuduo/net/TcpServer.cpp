#include "myMuduo/net/TcpServer.h"
#include "TcpServer.h"
#include "spdlog/spdlog.h"

namespace myMuduo {
namespace net {

struct sockaddr_in getLocalAddr(int sockfd)
{
    struct sockaddr_in localaddr;
    memset(&localaddr, 0, sizeof(localaddr));
    socklen_t addrlen = sizeof(localaddr);
    if (getsockname(sockfd, reinterpret_cast<struct sockaddr*>(&localaddr), &addrlen) < 0)
    {
        spdlog::critical("TcpServer::getLocalAddr - getsockname error: {}", strerror(errno));
        abort();
    }
    return localaddr;
}

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name, Option option)
    : baseLoop_(loop)
    , name_(name)
    , ipPort_(listenAddr.toIpPort())
    , acceptorPtr_(new Acceptor(loop, listenAddr, option))
    , threadPoolPtr_(new EventLoopThreadPool(loop, name))
    , connectionCallback_()
    , messageCallback_()
    , writeCompleteCallback_()
    , threadInitCallback_()
    , started_(false)
    , nextConnId_(1)
{
    if (baseLoop_ == nullptr)
    {
        spdlog::critical("TcpServer::TcpServer - loop is nullptr");
        abort();
    }

    acceptorPtr_->setNewConnectionCallback(
        [this](int sockfd, const InetAddress& peeraddr) { this->newConnection(sockfd, peeraddr); });
}

TcpServer::~TcpServer()
{
    //待实现
}

void TcpServer::setThreadNum(int numThreads)
{
    assert(numThreads > 0);
    baseLoop_->assertInLoopThread();
    threadPoolPtr_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    baseLoop_->assertInLoopThread();
    if (!started_.exchange(true))
    {
        threadPoolPtr_->start(threadInitCallback_);

        //对于poller的修改必须在对应Loop_的线程中进行
        //实际上start()方法是在baseLoop_的线程中调用的
        //acceptorPtr_->listen();直接这样也可以
        baseLoop_->runInLoop([this]() { acceptorPtr_->listen(); });
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    baseLoop_->assertInLoopThread();
    assert(sockfd >= 0);
    std::string connName = name_ + "-" + peerAddr.toIpPort() + "-" + std::to_string(nextConnId_);
    nextConnId_++;
    spdlog::info("TcpServer::newConnection - new connection [{}] from {}", connName, peerAddr.toIpPort());

    EventLoop* ioLoop = threadPoolPtr_->getNextLoop();
    InetAddress localAddr(getLocalAddr(sockfd));

    TcpConnectionPtr connPtr =
        std::shared_ptr<TcpConnection>(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));

    connections_[connName] = connPtr;

}  // namespace net
}  // namespace net
}  // namespace myMuduo