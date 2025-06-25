#pragma once

#include <memory>
#include <set>
#include <vector>
#include "myMuduo/base/Callback.h"
#include "myMuduo/base/Timer.h"
#include "myMuduo/base/TimerId.h"
#include "myMuduo/base/Timestamp.h"
#include "myMuduo/base/noncopyable.h"
#include "myMuduo/net/Channel.h"
namespace myMuduo {
namespace net {
class EventLoop;
}
}  // namespace myMuduo

namespace myMuduo {
namespace base {

class TimerQueue : noncopyable
{
public:
    TimerQueue(net::EventLoop* loop);
    ~TimerQueue();

    TimerId addTimer(TimerCallback cb, Timestamp when, double interval, bool repeat);
    void cancel(TimerId timerId);

private:
    using Entry = std::pair<Timestamp, Timer*>;
    using TimerList = std::set<Entry>;

    void addTimerInLoop(Timer* timer);
    void cancelInLoop(TimerId timerId);

    void resetTimerfd(Timestamp expiration);
    void handleRead();
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired, Timestamp now);

    bool insert(Timer* timer);

    net::EventLoop* loop_;
    const int timerfd_;
    net::Channel timerfdChannel_;

    //还没到期的定时器 到期的定时器会被移除
    TimerList timers_;

    //正在取消的定时器
    std::set<Timer*> cancelingTimers_;

    bool callingExpiredTimers_;
};
}  // namespace base
}  // namespace myMuduo