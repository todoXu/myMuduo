#include "muduo/net/poller/EPollPoller.h"
#include "muduo/net/EventLoop.h"

namespace myMuduo {
namespace net {
EPollPoller::EPollPoller(EventLoop* loop) {}

EPollPoller::~EPollPoller() {}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels) { return Timestamp::now(); }
void EPollPoller::updateChannel(Channel* channel) {}
void EPollPoller::removeChannel(Channel* channel) {}
bool EPollPoller::hasChannel(Channel* channel) const { return false; }
void EPollPoller::assertInLoopThread() const {}

}  // namespace net
}  // namespace myMuduo