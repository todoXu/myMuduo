#pragma once

#include <sys/epoll.h>
#include <unordered_map>
#include "myMuduo/base/Timestamp.h"
#include "myMuduo/net/Channel.h"
#include "myMuduo/net/poller/Poller.h"

namespace myMuduo {
namespace net {

class EPollPoller : public IPoller
{
public:

    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;
    bool hasChannel(Channel* channel) const override;
    void assertInLoopThread() const override;

private:
    static const int kInitEventListSize = 16;

    static const char* operationToString(int op);

    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    void update(int operation, Channel* channel);

    using EventList = std::vector<struct epoll_event>;
    EventList events_;

    int epollfd_;

    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;

    EventLoop* ownerLoop_;
};

}  // namespace net
}  // namespace myMuduo