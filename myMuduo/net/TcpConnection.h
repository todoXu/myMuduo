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
};

}  // namespace net
}  // namespace myMuduo