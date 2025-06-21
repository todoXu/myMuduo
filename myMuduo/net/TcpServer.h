#pragma once

#include <functional>
#include <unordered_map>
#include "myMuduo/base/noncopyable.h"
#include "myMuduo/net/Acceptor.h"
#include "myMuduo/net/Buffer.h"
#include "myMuduo/net/EventLoop.h"
#include "myMuduo/net/EventLoopThreadPool.h"
#include "myMuduo/net/InetAddress.h"
#include "myMuduo/net/TcpConnection.h"

namespace myMuduo {
namespace net {

class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
    using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
    using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)>;
    using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    enum Option
    {
        kNoReusePort = false,  // 不使用端口复用
        kReusePort = true
    };

    TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name, Option option = kNoReusePort);
    ~TcpServer();

    const std::string& name() const { return name_; }
    const std::string& ipPort() const { return ipPort_; }
    EventLoop* getLoop() const { return baseLoop_; }
    std::shared_ptr<EventLoopThreadPool> threadPool() const { return threadPoolPtr_; }

    void setThreadNum(int numThreads);
    void setConnectionCallback(ConnectionCallback cb) { connectionCallback_ = std::move(cb); }
    void setMessageCallback(MessageCallback cb) { messageCallback_ = std::move(cb); }
    void setWriteCompleteCallback(WriteCompleteCallback cb)
    {
        writeCompleteCallback_ = std::move(cb);
    }
    void setThreadInitCallback(ThreadInitCallback cb) { threadInitCallback_ = std::move(cb); }

    void start();

private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    EventLoop* baseLoop_;
    std::string name_;
    std::string ipPort_;
    std::unique_ptr<Acceptor> acceptorPtr_;
    std::shared_ptr<EventLoopThreadPool> threadPoolPtr_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_;
    std::atomic<bool> started_;
    int nextConnId_;
    ConnectionMap connections_;
};

}  // namespace net
}  // namespace myMuduo