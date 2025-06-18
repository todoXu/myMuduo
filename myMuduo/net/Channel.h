#pragma once

#include <poll.h>
#include <functional>
#include <memory>
#include "myMuduo/base/Timestamp.h"
#include "myMuduo/base/noncopyable.h"

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
    void tie(const std::shared_ptr<void> &obj);

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

    // 设置Channel在Poller中的状态
    // -1表示未在Poller中注册
    // 1表示在Poller中注册
    // 2表示在Poller中被移除
    int index() const { return index_; }
    void set_index(int idx) { index_ = idx; }

    std::string reventsToString() const;
    std::string eventsToString() const;
    // 返回Channel所属的EventLoop
    EventLoop *ownerLoop() { return loop_; }
    void remove();

private:
    void update();
    static std::string eventsToString(int fd, int ev);
    void handleEventWithGuard(Timestamp receiveTime);

    EventLoop *loop_;
    const int fd_;
    int events_;   // 希望关注的事件类型 读/写/不关注
    int revents_;  // fd关联的真实事件 在poller中监听
    int index_;    // 用于标识Channel在Poller的实际状态
                   // -1表示未在Poller中注册
                   // 1表示在Poller中注册
                   // 2表示在Poller中被移除

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
