#pragma once

#include <poll.h>

#include <functional>
#include <memory>

#include "muduo/base/Timestamp.h"
#include "muduo/base/noncopyable.h"

const int myMuduo::net::Channel::kNoneEvent = 0;
const int myMuduo::net::Channel::kReadEvent = POLLIN | POLLPRI;
const int myMuduo::net::Channel::kWriteEvent = POLLOUT;

namespace myMuduo {
namespace net {
class EventLoop;

class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    void handleEvent(Timestamp receiveTime);

    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    //绑定Channel拥有者的生命周期
    void tie(const std::shared_ptr<void> &obj)
    {
        tie_ = obj;
        tied_ = true;
    }

    int fd() const { return fd_; }
    int events() const { return events_; }
    //poller 向 Channel 通知 当前 fd 实际发生了哪些事件
    void set_revents(int revt) { revents_ = revt; }
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isReading() const { return events_ & kReadEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }

    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }

    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }

    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }

    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }

    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }

    int index() const { return index_; }
    void set_index(int idx) { index_ = idx; }

    std::string reventsToString() const;

    EventLoop *ownerLoop() { return loop_; }
    void remove();

private:
    void update();
    static std::string eventsToString(int fd, int ev);
    void handleEventWithGuard(Timestamp receiveTime);

    EventLoop *loop_;
    const int fd_;
    int events_;   // 关注的事件
    int revents_;  // poller返回的事件
    int index_;    // 用于标识Channel在Poller中的位置

    std::weak_ptr<void> tie_;  // 观察Channel拥有者的生命周期
    bool tied_;                // 是否绑定了生命周期
    bool eventHandling_;
    bool addedToLoop_;

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
}  // namespace net

}  // namespace myMuduo
