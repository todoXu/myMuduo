#include "myMuduo/net/TcpConnection.h"
#include "TcpConnection.h"
#include "myMuduo/net/Channel.h"
#include "myMuduo/net/EventLoop.h"
#include "myMuduo/net/Socket.h"
#include "spdlog/spdlog.h"

namespace myMuduo {
namespace net {
TcpConnection::TcpConnection(EventLoop* loop, const std::string& name, int sockfd,
                             const InetAddress& localAddr, const InetAddress& peerAddr)
    : loop_(loop)
    , name_(name)
    , state_(kConnecting)
    , reading_(false)
    , socketPtr_(new Socket(sockfd))
    , channelPtr_(new Channel(loop, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , messageCallback_()
    , writeCompleteCallback_()
    , highWaterMarkCallback_()
    , closeCallback_()
    , highWaterMark_(64 * 1024 * 1024)  // 默认水位
    , inputBuffer_()
    , outputBuffer_()
    , context_()
{
    channelPtr_->setReadCallback([this](Timestamp receiveTime) { handleRead(receiveTime); });
    channelPtr_->setWriteCallback([this]() { handleWrite(); });
    channelPtr_->setCloseCallback([this]() { handleClose(); });
    channelPtr_->setErrorCallback([this]() { handleError(); });

    spdlog::debug("TcpConnection[{}] created, local address: {}, peer address: {}", name_,
                  localAddr_.toIpPort(), peerAddr_.toIpPort());
    socketPtr_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() { assert(state_ == kDisconnected); }

EventLoop* TcpConnection::getLoop() { return loop_; }

const std::string& TcpConnection::getName() { return name_; }
const InetAddress& TcpConnection::getLocalAddress() const { return localAddr_; }
const InetAddress& TcpConnection::getPeerAddress() const { return peerAddr_; }

bool TcpConnection::connected() const { return state_ == kConnected; }
bool TcpConnection::disconnected() const { return state_ == kDisconnected; }

bool TcpConnection::getTcpInfo(struct tcp_info* info) const { return socketPtr_->getTcpInfo(info); }

const std::string TcpConnection::getTcpInfoString() const
{
    std::string info = "";
    socketPtr_->getTcpInfoString(info);
    return info;
}

void TcpConnection::send(const std::string& message)
{
    if (state_ == kConnected)
    {
        //按值传递捕获message，避免在发送过程中message被修改或销毁
        loop_->runInLoop([this, message]() { sendInLoop(message); });
    }
}

void TcpConnection::send(Buffer* message)
{
    if (state_ == kConnected)
    {
        std::string str = message->retrieveAllAsString();
        loop_->runInLoop([this, str]() { sendInLoop(str); });
    }
}

void TcpConnection::shutdown() {}
void TcpConnection::forceClose() {}
void TcpConnection::forceCloseWithDelay(double seconds) {}
void TcpConnection::setTcpNoDelay(bool on) {}

void TcpConnection::startRead() {}
void TcpConnection::stopRead() {}
bool TcpConnection::isReading() {}

void TcpConnection::setContext(const base::Any& context) {}
const base::Any& TcpConnection::getContext() const {}

void TcpConnection::setConnectionCallback(ConnectionCallback cb) {}
void TcpConnection::setMessageCallback(MessageCallback cb) {}
void TcpConnection::setWriteCompleteCallback(WriteCompleteCallback cb) {}
void TcpConnection::setHighWaterMarkCallback(HighWaterMarkCallback cb, size_t highWaterMark) {}
void TcpConnection::setCloseCallback(CloseCallback cb) {}

Buffer* TcpConnection::getInputBuffer() {}
Buffer* TcpConnection::getOutputBuffer() {}

void TcpConnection::connectEstablished() {}
void TcpConnection::connectDestroyed() {}

void TcpConnection::handleRead(Timestamp receiveTime) {}
void TcpConnection::handleWrite() {}
void TcpConnection::handleClose() {}
void TcpConnection::handleError() {}

void TcpConnection::sendInLoop(const std::string& message) {}
void TcpConnection::shutdownInLoop() {}
void TcpConnection::forceCloseInLoop() {}
void TcpConnection::setState(StateE s) {}
std::string TcpConnection::stateToString() {}
void TcpConnection::startReadInLoop() {}
void TcpConnection::stopReadInLoop() {}

}  // namespace net

}  // namespace myMuduo