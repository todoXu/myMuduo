#include "myMuduo/base/Timer.h"
#include <cassert>

namespace myMuduo {
namespace base {

std::atomic<int64_t> Timer::s_numCreated_{0};

Timer::Timer(TimerCallback cb, Timestamp when, double interval, bool repeat)
    : callback_(std::move(cb))
    , expiration_(when)
    , interval_(interval)
    , repeat_(repeat)
    , sequence_(s_numCreated_.fetch_add(1))
{
    if (repeat)
    {
        assert(interval_ > 0.0);
    }
}

void Timer::restart(Timestamp now)
{
    if (repeat_)
    {
        //下一个触发时间
        expiration_ = now + interval_;
    }
    else
    {
        expiration_ = Timestamp::invalid();
    }
}

}  // namespace base
}  // namespace myMuduo