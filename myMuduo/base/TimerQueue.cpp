#include "myMuduo/base/TimerQueue.h"
#include <string.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include "TimerQueue.h"
#include "cassert"
#include "spdlog/spdlog.h"

namespace myMuduo {
namespace base {

//将timerfd注册到epoll中
//timerfd_settime会在指定时间到达时触发读事件 epoll会通知EventLoop回调handleRead
//handleRead处理注册的定时器 执行回调
TimerQueue::TimerQueue(net::EventLoop* loop)
    : loop_(loop)
    , timerfd_(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC))
    , timerfdChannel_(loop_, timerfd_)
    , timers_()
    , callingExpiredTimers_(false)
{
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue()
{
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    close(timerfd_);
    for (auto timer : timers_)
    {
        delete timer.second;
    }
}

TimerId TimerQueue::addTimer(TimerCallback cb, Timestamp when, double interval, bool repeat)
{
    Timer* timer = new Timer(cb, when, interval, repeat);
    TimerId timerId(timer, timer->sequence());
    loop_->runInLoop([this, timer]() { this->addTimerInLoop(timer); });
    return timerId;
}
void TimerQueue::cancel(TimerId timerId)
{
    loop_->runInLoop([this, timerId]() { this->cancelInLoop(timerId); });
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
    loop_->assertInLoopThread();
    //新加入的定时器是否比当前最早的定时器要早
    bool earliestChanged = insert(timer);
    if (earliestChanged)
    {
        resetTimerfd(timer->expiration());
    }
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
    loop_->assertInLoopThread();
    auto it = timers_.find(timerId.getTimer()->expiration());
    if (it != timers_.end())
    {
        timers_.erase(it);
        delete it->second;
    }
    //定时器不在timers_中 说明它已经到期销毁了 或者 正在执行
    //如果正在执行定时器回调 则将其加入cancelingTimers_中  如果是正在执行的 需要执行完成后delete
    else if (callingExpiredTimers_)
    {
        cancelingTimers_.insert({timerId.getTimer()->expiration(), timerId.getTimer()});
    }
}

// void TimerQueue::handleRead() {}

//获取到期该执行的定时器
std::vector<Timer*> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Timer*> expired;
    auto it = timers_.upper_bound(now);
    std::copy(timers_.begin(), it, std::back_inserter(expired));
    timers_.erase(timers_.begin(), it);

    return expired;
}

// void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now) {}

bool TimerQueue::insert(Timer* timer)
{
    loop_->assertInLoopThread();
    bool earliestChanged = false;
    Timestamp when = timer->expiration();

    auto it = timers_.begin();
    if (it == timers_.end() || when < it->first)
    {
        earliestChanged = true;
    }

    auto res = timers_.insert({when, timer});
    assert(res.second);

    return earliestChanged;
}

void TimerQueue::resetTimerfd(Timestamp expiration)
{
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof(newValue));
    memset(&oldValue, 0, sizeof(oldValue));

    // 计算距离当前时间的微秒数
    // 如果小于100微秒，则设置为100微秒，避免精度过小
    int64_t microseconds = expiration.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    if (microseconds < 100)
    {
        microseconds = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);

    newValue.it_value = ts;
    int ret = timerfd_settime(timerfd_, 0, &newValue, &oldValue);
    if (ret != 0)
    {
        spdlog::critical("TimerQueue::resetTimerfd timerfd_settime error: {}", strerror(errno));
        abort();
    }
}

}  // namespace base
}  // namespace myMuduo
