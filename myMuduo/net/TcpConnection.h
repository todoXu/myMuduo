#pragma once
#include <netinet/tcp.h>
#include <memory>
#include "myMuduo/base/Any.h"
#include "myMuduo/base/noncopyable.h"
#include "myMuduo/net/Buffer.h"
#include "myMuduo/net/Callback.h"
#include "myMuduo/net/InetAddress.h"

namespace myMuduo {
namespace net {
class EventLoop;
class Socket;
class Channel;

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, const std::string& name, int sockfd,
                  const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection();
    EventLoop* getLoop();
    const std::string& getName();
    const InetAddress& getLocalAddress() const;
    const InetAddress& getPeerAddress() const;
    bool connected() const;
    bool disconnected() const;
    bool getTcpInfo(struct tcp_info*) const;
    const std::string getTcpInfoString() const;

    void send(const std::string& message);
    void send(Buffer* message);
    void shutdown();
    void forceClose();
    void forceCloseWithDelay(double seconds);
    void setTcpNoDelay(bool on);

    void startRead();
    void stopRead();
    bool isReading();

    void setContext(const base::Any& context);
    const base::Any& getContext() const;

    void setConnectionCallback(ConnectionCallback cb);
    void setMessageCallback(MessageCallback cb);
    void setWriteCompleteCallback(WriteCompleteCallback cb);
    void setHighWaterMarkCallback(HighWaterMarkCallback cb, size_t highWaterMark);
    void setCloseCallback(CloseCallback cb);

    Buffer* getInputBuffer();
    Buffer* getOutputBuffer();

    void connectEstablished();
    void connectDestroyed();

private:
    enum StateE
    {
        kConnecting,
        kConnected,
        kDisconnecting,
        kDisconnected
    };

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const std::string& message);
    void shutdownInLoop();
    void forceCloseInLoop();
    void setState(StateE s);
    std::string stateToString();
    void startReadInLoop();
    void stopReadInLoop();

    EventLoop* loop_;
    const std::string name_;
    StateE state_;
    bool reading_;
    std::unique_ptr<Socket> socketPtr_;
    std::unique_ptr<Channel> channelPtr_;
    const InetAddress localAddr_;
    const InetAddress peerAddr_;
    ConnectionCallback connectionCallback_;
    MessageCallback messageCallback_;
    WriteCompleteCallback writeCompleteCallback_;
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_;
    Buffer inputBuffer_;
    Buffer outputBuffer_;
    base::Any context_;
};

}  // namespace net
}  // namespace myMuduo