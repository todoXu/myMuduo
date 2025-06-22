#pragma once
#include <functional>
#include <memory>
#include "myMuduo/base/Timestamp.h"

namespace myMuduo {
namespace net {

class Buffer;
class TcpConnection;
class EventLoop;
class InetAddress;

using ThreadInitCallback = std::function<void(EventLoop*)>;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*, Timestamp)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &peeraddr)>;
using EventCallback = std::function<void()>;
using ReadEventCallback = std::function<void(Timestamp)>;

}  // namespace net
}  // namespace muduo
