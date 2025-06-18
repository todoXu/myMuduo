#include "myMuduo/net/poller/EPollPoller.h"
#include <sys/epoll.h>
#include <cstdlib>
#include "myMuduo/net/EventLoop.h"
#include "spdlog/spdlog.h"

namespace myMuduo {
namespace net {

EPollPoller::EPollPoller(EventLoop* loop)
    : events_(kInitEventListSize)
    , ownerLoop_(loop)
{
    // 创建一个epoll实例，返回一个文件描述符
    // EPOLL_CLOEXEC用于设置文件描述符的关闭执行标志，防止子进程继承该文件描述符
    // 这样可以避免在fork后子进程意外使用父进程的epoll实例
    // 如果不设置，可能会导致资源泄漏或意外行为
    epollfd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epollfd_ < 0)
    {
        spdlog::critical("EPollPoller::EPollPoller() - epoll_create1 failed: {}", strerror(errno));
        abort();
    }
}

EPollPoller::~EPollPoller()
{
    spdlog::debug("EPollPoller::~EPollPoller() - epollfd: {}", epollfd_);
    close(epollfd_);
}

Timestamp EPollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    int numEvents = epoll_wait(epollfd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);
    int savedErrno = errno;
    spdlog::debug("EPollPoller::poll() - numEvents: {}, timeoutMs: {}", numEvents, timeoutMs);
    Timestamp now = Timestamp::now();
    if (numEvents > 0)
    {
        fillActiveChannels(numEvents, activeChannels);
    }

    if (events_.size() == static_cast<size_t>(numEvents))
    {
        // 如果事件列表的大小等于返回的事件数量，说明需要扩容
        events_.resize(events_.size() * 2);
    }

    if (numEvents == 0)
    {
        spdlog::debug("EPollPoller::poll() - no events occurred, timeout: {} ms", timeoutMs);
    }
    else if (numEvents < 0)
    {
        if (savedErrno != EINTR)
        {
            spdlog::critical("EPollPoller::poll() - epoll_wait failed: {}", strerror(errno));
            abort();
        }
        else
        {
            spdlog::debug("EPollPoller::poll() - epoll_wait interrupted by signal");
        }
    }

    return now;
}

// 更新Channel的事件 channel都会添加到channels_中
// 如果channel是不关注任何事件，则从epoll中删除 channels_中仍然保留该fd的记录
void EPollPoller::updateChannel(Channel* channel)
{
    // 确保在EventLoop的拥有者线程中调用
    assertInLoopThread();
    const int index = channel->index();
    const int fd = channel->fd();
    spdlog::debug("EPollPoller::updateChannel() - fd: {}, index: {}, enent: {}", fd, index,
                  channel->eventsToString());
    // 新增Channel
    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            //确保Channel未在Poller中注册
            assert(channels_.find(fd) == channels_.end());

            channel->set_index(kAdded);
            channels_[fd] = channel;
            update(EPOLL_CTL_ADD, channel);
        }
        else
        {
            //channel以前在Poller中注册过，但在poller中被删除
            //channels_中仍然有该fd的记录
            assert(channels_.find(fd) != channels_.end());
            assert(channels_[fd] == channel);

            channel->set_index(kAdded);
            update(EPOLL_CTL_ADD, channel);
        }
    }
    else
    {
        // 更新已存在的Channel
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel);
        assert(index == kAdded);

        if (channel->isNoneEvent())
        {
            // 如果Channel不关注任何事件，则从epoll中删除
            // 但是channels_中仍然保留该fd的记录
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            // 更新Channel的事件
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

//只有chennel不关心任何事件时，才能在channels_和epoll中删除
void EPollPoller::removeChannel(Channel* channel)
{
    assertInLoopThread();
    int fd = channel->fd();
    int index = channel->index();
    spdlog::debug("EPollPoller::removeChannel() - fd: {}, index: {}", fd, channel->index());

    // 确保Channel在channels_中注册了
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(index == kAdded || index == kDeleted);
    assert(channel->isNoneEvent());

    //从channels_中删除Channel
    channels_.erase(fd);
    if (index == kAdded)
    {
        //从epoll中删除Channel
        update(EPOLL_CTL_DEL, channel);
    }
    // 设置为未注册状态
    channel->set_index(kNew);
}

bool EPollPoller::hasChannel(Channel* channel) const
{
    assertInLoopThread();
    int fd = channel->fd();
    auto it = channels_.find(fd);
    //防止fd重用导致channel被误判
    return it != channels_.end() && it->second == channel;
}

void EPollPoller::assertInLoopThread() const
{
    // 确保当前线程是EventLoop的拥有者线程
    ownerLoop_->assertInLoopThread();
}

void EPollPoller::update(int operation, Channel* channel)
{
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->events();
    event.data.ptr = channel;  // 将Channel指针存储在epoll_event的data中
    int fd = channel->fd();
    spdlog::debug("EPollPoller::update() - fd: {}, operation: {}, events: {}", fd,
                  operationToString(operation), channel->eventsToString());

    if (epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        spdlog::critical("EPollPoller::update() - epoll_ctl failed: {}, fd: {}, operation: {}",
                         strerror(errno), fd, operationToString(operation));
        abort();
    }
}

const char* EPollPoller::operationToString(int op)
{
    switch (op)
    {
        case EPOLL_CTL_ADD:
            return "ADD";
        case EPOLL_CTL_DEL:
            return "DEL";
        case EPOLL_CTL_MOD:
            return "MOD";
        default:
            assert(false && "ERROR op");
            return "Unknown Operation";
    }
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    assert(activeChannels != nullptr);
    assert(numEvents <= static_cast<int>(events_.size()));

    for (int i = 0; i < numEvents; ++i)
    {
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);

        int fd = channel->fd();
        ChannelMap::const_iterator it = channels_.find(fd);
        assert(it != channels_.end());
        assert(it->second == channel);

        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel);
    }
}

}  // namespace net
}  // namespace myMuduo