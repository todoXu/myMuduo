#include "myMuduo/base/TimerQueue.h"
#include <string.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include "TimerQueue.h"
#include "cassert"
#include "myMuduo/net/EventLoop.h"
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
    for (auto entry : timers_)
    {
        delete entry.second;
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

    for (auto it = timers_.begin(); it != timers_.end(); ++it)
    {
        if (it->second->sequence() == timerId.getSequence())
        {
            delete it->second;
            timers_.erase(it);
            return;
        }
    }

    //如果该定时器已经过期或者不存在的 放到正在取消的定时器列表中
    if (callingExpiredTimers_)
    {
        cancelingTimers_.insert(timerId.getTimer());
    }
}

void TimerQueue::handleRead()
{
    loop_->assertInLoopThread();
    Timestamp now = Timestamp::now();
    uint64_t howmany;
    ssize_t n = ::read(timerfd_, &howmany, sizeof howmany);
    if (n != sizeof howmany)
    {
        spdlog::error("TimerQueue::handleRead() reads {} bytes instead of 8", n);
    }

    std::vector<Entry> expired = getExpired(now);
    callingExpiredTimers_ = true;
    cancelingTimers_.clear();
    for (auto it = expired.begin(); it != expired.end(); ++it)
    {
        it->second->run();
    }
    callingExpiredTimers_ = false;
    reset(expired, now);
}

//获取到期该执行的定时器
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now)
{
    std::vector<Entry> expired;
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));  // 保证 upper_bound 能取到所有 <= now 的
    auto it = timers_.upper_bound(sentry);
    std::copy(timers_.begin(), it, back_inserter(expired));
    timers_.erase(timers_.begin(), it);
    return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
    for (auto it = expired.begin(); it != expired.end(); ++it)
    {
        if (it->second->repeat() && cancelingTimers_.find(it->second) == cancelingTimers_.end())
        {
            // 如果是重复定时器，重新设置它的过期时间
            it->second->restart(now);
            insert(it->second);  // 重新插入到 timers_ 中
        }
        else
        {
            delete it->second;  // 删除定时器（如果是非重复定时器或正在取消的定时器）
        }
    }

    if (!timers_.empty() && timers_.begin()->first.valid())
    {
        resetTimerfd(timers_.begin()->first);
    }
}

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

    auto res = timers_.insert(Entry(when, timer));
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
