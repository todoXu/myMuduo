#include "myMuduo/net/TcpConnection.h"
#include "TcpConnection.h"
#include "myMuduo/net/Channel.h"
#include "myMuduo/net/EventLoop.h"
#include "myMuduo/net/Socket.h"
#include "spdlog/spdlog.h"
#include "myMuduo/base/WeakCallback.h"

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
    , connectionCallback_()
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
        auto self = shared_from_this();
        loop_->runInLoop([self, message]() { self->sendInLoop(message); });
    }
}

void TcpConnection::send(Buffer* message)
{
    if (state_ == kConnected)
    {
        std::string str = message->retrieveAllAsString();
        auto self = shared_from_this();
        loop_->runInLoop([self, str]() { self->sendInLoop(str); });
    }
}

void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        auto self = shared_from_this();
        loop_->runInLoop([self]() { self->shutdownInLoop(); });
    }
}
void TcpConnection::forceClose()
{
    if (state_ == kConnected || state_ == kConnecting)
    {
        setState(kDisconnecting);
        /*
        
        void onConnection(const muduo::net::TcpConnectionPtr& conn)
        {
            if (conn->connected())
            {
            LOG_INFO << "A client has connected. Preparing to demonstrate the crash...";
            
            // 这是最关键的一步：我们创建一个新线程，并通过 std::move 将 conn 的所有权
            // 完全转移给新线程的 lambda。此时，新线程是这个 TcpConnection 对象的唯一所有者。
            std::thread t([conn_moved = std::move(conn)]() {
                LOG_INFO << "Worker thread started. I am the sole owner of the connection.";
                LOG_INFO << "Connection use_count in thread: " << conn_moved.use_count();

                // 调用那个存在缺陷的、使用裸指针`this`的异步函数
                conn_moved->forceClose("This message will likely never be sent.\r\n");

                LOG_WARN << "Worker thread is now finishing. The TcpConnection object will be destroyed NOW.";
            });
            // 分离线程，让它自生自灭
            t.detach();
        }
        假设forceClose不用shared_from_this()来获取当前对象的shared_ptr，而是直接使用裸指针this，
        onConnection结束时conn_moved被释放  但forceCloseInLoop还没执行
        当forceCloseInLoop执行时，this已经被释放了
        这时如果forceCloseInLoop中有对this的引用（比如调用了handleClose()），
        就会导致访问已释放的内存，可能引发段错误或其他未定义行为。
        因此在forceClose中使用shared_from_this()
        确保在当前对象的生命周期内，引用计数不会降为0
        */
        auto self = shared_from_this();
        loop_->queueInLoop([self]() { self->forceCloseInLoop(); });
    }
}
void TcpConnection::forceCloseWithDelay(double seconds)
{
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        setState(kDisconnecting);

        loop_->runAfter(seconds, makeWeakCallback(shared_from_this(), &TcpConnection::forceClose));
    }
}
void TcpConnection::setTcpNoDelay(bool on) { socketPtr_->setTcpNoDelay(on); }

void TcpConnection::startRead()
{
    auto self = shared_from_this();
    loop_->runInLoop([self]() { self->startReadInLoop(); });
}

void TcpConnection::stopRead()
{
    auto self = shared_from_this();
    loop_->runInLoop([self]() { self->stopReadInLoop(); });
}
bool TcpConnection::isReading() { return reading_; }

void TcpConnection::setContext(const base::Any& context) { context_ = context; }
const base::Any& TcpConnection::getContext() const { return context_; }

void TcpConnection::setConnectionCallback(ConnectionCallback cb)
{
    connectionCallback_ = std::move(cb);
}

void TcpConnection::setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }

void TcpConnection::setWriteCompleteCallback(WriteCompleteCallback cb)
{
    writeCompleteCallback_ = std::move(cb);
}

void TcpConnection::setHighWaterMarkCallback(HighWaterMarkCallback cb, size_t highWaterMark)
{
    highWaterMarkCallback_ = std::move(cb);
    highWaterMark_ = highWaterMark;
}

void TcpConnection::setCloseCallback(CloseCallback cb) { closeCallback_ = std::move(cb); }

Buffer* TcpConnection::getInputBuffer() { return &inputBuffer_; }
Buffer* TcpConnection::getOutputBuffer() { return &outputBuffer_; }

void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channelPtr_->tie(shared_from_this());  // 绑定生命周期
    channelPtr_->enableReading();          // 启用读事件

    connectionCallback_(shared_from_this());  // 调用连接回调

    spdlog::debug("TcpConnection[{}] established, local address: {}, peer address: {}", name_,
                  localAddr_.toIpPort(), peerAddr_.toIpPort());
}

void TcpConnection::connectDestroyed()
{
    loop_->assertInLoopThread();
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channelPtr_->disableAll();                // 禁用所有事件
        connectionCallback_(shared_from_this());  // 调用连接回调 处理连接断开逻辑
    }
    channelPtr_->remove();
}

//handleRead会在channelPtr_的读事件触发时被调用
//如果n>0 说明是正常读取到数据
//如果n==0 说明对端关闭了连接 发了FIN，调用handleClose()处理关闭
void TcpConnection::handleRead(Timestamp receiveTime)
{
    loop_->assertInLoopThread();
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(socketPtr_->fd(), &saveErrno);
    if (n > 0)
    {
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = saveErrno;
        handleError();
    }
}

//只会在enableWriting后才会触发回调
//只要内核发送缓冲区有空闲 就会触发可写事件
//所以优先是直接write 不通过epoll可写事件来触发 只有当内核缓冲区满了才将数据写入outbuffer里, 再enableWriting
//关注内核缓冲区的可写事件 等到有空间后就会触发可写事件
//再write发送数据
void TcpConnection::handleWrite()
{
    loop_->assertInLoopThread();
    //检查channel是否开启了写事件
    if (channelPtr_->isWriting())
    {
        ssize_t n = write(channelPtr_->fd(), outputBuffer_.peek(), outputBuffer_.readableBytesLength());
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            //判断数据是否发送完成
            if (outputBuffer_.readableBytesLength() == 0)
            {
                //关闭可写事件关注 防止一直触发回调
                channelPtr_->disableWriting();
                if (writeCompleteCallback_)
                {
                    auto self = shared_from_this();
                    loop_->queueInLoop([self]() { self->writeCompleteCallback_(self); });
                }

                //判断当前状态 如果是kDisconnecting状态
                //则调用shutdownInLoop来停止写数据
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            spdlog::critical("TcpConnection::handleWrite() - write error: {}", strerror(errno));
            abort();
        }
    }
    else
    {
        spdlog::warn("TcpConnection::handleWrite() - channel is not writing, fd: {}", channelPtr_->fd());
    }
}
void TcpConnection::handleClose()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnected || state_ == kDisconnecting);
    setState(kDisconnected);
    channelPtr_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this());
    connectionCallback_(guardThis);
    closeCallback_(guardThis);
}

void TcpConnection::handleError()
{
    int err = Socket::getSocketError(socketPtr_->fd());
    spdlog::error("TcpConnection::handleError() - socket error: {}, fd: {}", strerror(err),
                  socketPtr_->fd());
}

//优先直接write到内核发送缓冲区
//如果内核发送缓冲区满了 再将数据写入outputBuffer_
//用handleWrite来处理可写事件
void TcpConnection::sendInLoop(const std::string& message)
{
    loop_->assertInLoopThread();
    size_t remaining = message.size();
    bool faultError = false;
    ssize_t n = 0;
    if (state_ == kDisconnected)
    {
        spdlog::warn("TcpConnection::sendInLoop() - connection is disconnected, fd: {}", socketPtr_->fd());
        return;
    }

    //!channelPtr_->isWriting() 表示没有开启写事件 内核发送缓冲区有空间
    //outputBuffer_.readableBytesLength() == 0 应用层发送缓冲区是空的，没有积压任何待发送的数据
    if (!channelPtr_->isWriting() && outputBuffer_.readableBytesLength() == 0)
    {
        n = write(channelPtr_->fd(), message.data(), message.size());
        if (n >= 0)
        {
            remaining -= n;
            if (remaining == 0 && writeCompleteCallback_)
            {
                //如果发送完成了 则调用写完成回调
                auto self = shared_from_this();
                loop_->queueInLoop([self]() { self->writeCompleteCallback_(self); });
            }
        }
        else
        {
            n = 0;  // 如果写入失败，重置n为0
            //非缓冲区满的错误
            if (errno != EWOULDBLOCK)
            {
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    assert(remaining <= message.size());
    if (!faultError && remaining > 0)
    {
        size_t oldLen = outputBuffer_.readableBytesLength();
        if (oldLen < highWaterMark_ && oldLen + remaining >= highWaterMark_ && highWaterMarkCallback_)
        {
            auto self = shared_from_this();
            loop_->queueInLoop([self, oldLen, remaining]() {
                self->highWaterMarkCallback_(self, oldLen + remaining);
            });
        }
        outputBuffer_.append(message.data() + n, remaining);
        if (!channelPtr_->isWriting())
        {
            //如果没有开启写事件 则开启写事件
            channelPtr_->enableWriting();
            spdlog::debug("TcpConnection[{}] - enable writing, fd: {}", name_, channelPtr_->fd());
        }
    }
}

//停止写数据 如果当前还在写数据，则不执行
//在handleWrite中会根据当前连接状态和数据是否写完再次调用shutdownInLoop
void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if (!channelPtr_->isWriting())
    {
        socketPtr_->shutdownWrite();
    }
}
void TcpConnection::forceCloseInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == kConnected || state_ == kConnecting)
    {
        handleClose();
    }
}

void TcpConnection::setState(StateE s)
{
    state_ = s;
    spdlog::debug("TcpConnection[{}] - state changed to {}", name_, stateToString());
}

std::string TcpConnection::stateToString()
{
    switch (state_)
    {
        case kDisconnected:
            return "kDisconnected";
        case kConnecting:
            return "kConnecting";
        case kConnected:
            return "kConnected";
        case kDisconnecting:
            return "kDisconnecting";
        default:
            return "unknown state";
    }
}

void TcpConnection::startReadInLoop()
{
    loop_->assertInLoopThread();
    if (!reading_ || !channelPtr_->isReading())
    {
        reading_ = true;
        channelPtr_->enableReading();  // 启用读事件
        spdlog::debug("TcpConnection[{}] - start reading, fd: {}", name_, channelPtr_->fd());
    }
}
void TcpConnection::stopReadInLoop()
{
    loop_->assertInLoopThread();
    if (reading_ || channelPtr_->isReading())
    {
        channelPtr_->disableReading();
        reading_ = false;
    }
}

}  // namespace net

}  // namespace myMuduo