#include "muduo/net/Channel.h"

#include "spdlog/spdlog.h"

namespace myMuduo {
namespace net {

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(kNoneEvent)
    , revents_(kNoneEvent)
    , index_(-1)
    , eventHandling_(false)
    , addedToLoop_(false)
    , tied_(false)
{
    if (fd_ < 0)
    {
        spdlog::error("Channel::Channel() - fd must be >= 0, but got {}", fd_);
    }
    else
    {
        spdlog::debug("Channel::Channel() fd={}", fd_);
    }
}

Channel::~Channel() { printf("Channel::~Channel() fd=%d\n", fd_); }

void Channel::handleEvent(Timestamp receiveTime) {}

}  // namespace net
}  // namespace myMuduo
