#pragma once

#include "myMuduo/base/Timer.h"

namespace myMuduo {
namespace base {
class TimerId
{
public:
    TimerId()
        : timer_(nullptr)
        , sequence_(0)
    {
    }
    TimerId(Timer* timer, int64_t sequence)
        : timer_(timer)
        , sequence_(sequence)
    {
    }
    ~TimerId() = default;

    Timer* getTimer() const { return timer_; }
    int64_t getSequence() const { return sequence_; }

private:
    Timer* timer_;
    int64_t sequence_;
};
}  // namespace base
}  // namespace myMuduo