#pragma once
#include <memory>
#include "myMuduo/base/noncopyable.h"

namespace myMuduo {
namespace net {
class EventLoop;
class InetAddress;

class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, const std::string& name, int sockfd,
                  const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection();

private:
    enum StateE
    {
        kConnecting,
        kConnected,
        kDisconnecting,
        kDisconnected
    };

    void handleRead();
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
    
};

}  // namespace net
}  // namespace myMuduo