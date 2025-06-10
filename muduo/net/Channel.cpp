#include "muduo/net/Channel.h"
#include <cassert>
#include <sstream>
#include "muduo/net/EventLoop.h"
#include "muduo/net/poller/Poller.h"
#include "spdlog/spdlog.h"

const int myMuduo::net::Channel::kNoneEvent = 0;
const int myMuduo::net::Channel::kReadEvent = POLLIN | POLLPRI;
const int myMuduo::net::Channel::kWriteEvent = POLLOUT;

namespace myMuduo {
namespace net {

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(kNoneEvent)
    , revents_(kNoneEvent)
    , index_(kNew)
    , tied_(false)
    , eventHandling_(false)
    , addedToLoop_(false)
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

/*
在一个正常的服务器退出流程中：
先关闭所有连接，销毁相关 Channel
然后退出事件循环
最后销毁 EventLoop
*/
Channel::~Channel()
{
    // assert(!eventHandling_);
    // assert(!addedToLoop_);
    // if (loop_->isInLoopThread())
    // {
    //     assert(!loop_->hasChannel(this));
    // }
}

void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

//通过eventloop在poller里面更改fd的事件
void Channel::update()
{
    // addedToLoop_ = true;
    // loop_->updateChannel(this);
}

void Channel::remove()
{
    // assert(isNoneEvent());
    // addedToLoop_ = false;
    // loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    std::shared_ptr<void> guard;
    if (tied_)
    {
        guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
        else
        {
            // 如果绑定的对象已经过期，则不处理事件
            spdlog::warn("Channel::handleEvent() - tie_ has expired, fd={}", fd_);
        }
    }
    //没有绑定时直接处理 比如eventloop拥有channel eventloop的生命周期比channel长 不需要绑定确认eventloop有没有过期
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    eventHandling_ = true;
    spdlog::info("Channel::handleEventWithGuard() - fd={}, events={}, revents={}", fd_,
                 eventsToString(fd_, events_), eventsToString(fd_, revents_));

    // 连接被挂起 且 没有可读事件
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN))
    {
        if (closeCallback_)
        {
            spdlog::debug("Channel::handleEventWithGuard() - close event, fd={}", fd_);
            closeCallback_();
        }
        else
        {
            spdlog::warn("Channel::handleEventWithGuard() - POLLHUP without closeCallback, fd={}", fd_);
        }
    }

    //监听事件无效
    if (revents_ & POLLNVAL)
    {
        spdlog::error("Channel::handleEventWithGuard() - POLLNVAL, fd={}", fd_);
    }

    if (revents_ & (POLLERR | POLLNVAL))
    {
        if (errorCallback_)
        {
            spdlog::debug("Channel::handleEventWithGuard() - error event, fd={}", fd_);
            errorCallback_();
        }
        else
        {
            spdlog::warn("Channel::handleEventWithGuard() - POLLERR without errorCallback, fd={}", fd_);
        }
    }

    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP))
    {
        if (readCallback_)
        {
            spdlog::debug("Channel::handleEventWithGuard() - read event, fd={}", fd_);
            readCallback_(receiveTime);
        }
        else
        {
            spdlog::warn("Channel::handleEventWithGuard() - POLLIN without readCallback, fd={}", fd_);
        }
    }

    if (revents_ & POLLOUT)
    {
        if (writeCallback_)
        {
            spdlog::debug("Channel::handleEventWithGuard() - write event, fd={}", fd_);
            writeCallback_();
        }
        else
        {
            spdlog::warn("Channel::handleEventWithGuard() - POLLOUT without writeCallback, fd={}", fd_);
        }
    }

    eventHandling_ = false;
}

std::string Channel::reventsToString() const { return eventsToString(fd_, revents_); }

std::string Channel::eventsToString() const { return eventsToString(fd_, events_); }

std::string Channel::eventsToString(int fd, int ev)
{
    std::ostringstream oss;
    oss << fd << ": ";
    if (ev & POLLIN) oss << "IN ";
    if (ev & POLLPRI) oss << "PRI ";
    if (ev & POLLOUT) oss << "OUT ";
    if (ev & POLLHUP) oss << "HUP ";
    if (ev & POLLRDHUP) oss << "RDHUP ";
    if (ev & POLLERR) oss << "ERR ";
    if (ev & POLLNVAL) oss << "NVAL ";

    return oss.str();
}

}  // namespace net
}  // namespace myMuduo
