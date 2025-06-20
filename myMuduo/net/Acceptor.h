#pragma once

#include <functional>
#include "myMuduo/base/Timestamp.h"
#include "myMuduo/base/noncopyable.h"
#include "myMuduo/net/Channel.h"
#include "myMuduo/net/Socket.h"
#include "myMuduo/base/Timestamp.h"

namespace myMuduo {
namespace net {

class InetAddress;

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &peeraddr)>;
    Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(NewConnectionCallback cb)
    {
        newConnectionCallback_ = std::move(cb);
    }

    void listen();
    bool listening() const { return listening_; }

private:
    void handleRead(Timestamp receiveTime);
    EventLoop *loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listening_;
};

}  // namespace net
}  // namespace myMuduo