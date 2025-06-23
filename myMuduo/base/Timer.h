#pragma once
#include <atomic>
#include "myMuduo/base/Callback.h"
#include "myMuduo/base/Timestamp.h"
#include "myMuduo/base/noncopyable.h"

namespace myMuduo {
namespace base {
class Timer : noncopyable
{
public:
    Timer(TimerCallback cb, Timestamp when, double interval, bool repeat = true);
    ~Timer() = default;

    void run() { callback_(); };

    Timestamp expiration() const { return expiration_; }
    double interval() const { return interval_; }
    bool repeat() const { return repeat_; }
    int64_t sequence() const { return sequence_; }

    void restart(Timestamp now);
    static int64_t numCreated() { return s_numCreated_.load(); }

private:
    const TimerCallback callback_;
    Timestamp expiration_;
    const double interval_;
    const bool repeat_;
    const int64_t sequence_;

    static std::atomic<int64_t> s_numCreated_;
};

}  // namespace base
}  // namespace myMuduo