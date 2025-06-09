#pragma once

#include <vector>
#include "muduo/base/Timestamp.h"
#include "muduo/base/noncopyable.h"
namespace myMuduo {
namespace net {

class Channel;
class EventLoop;

//事件分发器接口类
class IPoller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    virtual ~IPoller() = default;
    // Poller的主要功能是监听事件并返回活跃的Channel
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;
    virtual bool hasChannel(Channel* channel) const = 0;
    virtual void assertInLoopThread() const = 0;

    static IPoller* newDefaultPoller(EventLoop* loop);
};

}  // namespace net
}  // namespace myMuduo
