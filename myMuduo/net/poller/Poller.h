#pragma once

#include <vector>
#include "myMuduo/base/Timestamp.h"
#include "myMuduo/base/noncopyable.h"
namespace myMuduo {
namespace net {

class Channel;
class EventLoop;

enum
{
    kNew = -1,    // Channel未在Poll中注册
    kAdded = 1,   // Channel已在Poll中注册
    kDeleted = 2  // Channel已从Poll中删除
};

//事件分发器接口类
class IPoller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    IPoller() = default;
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
