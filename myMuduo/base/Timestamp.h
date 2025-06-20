#pragma once
#include <inttypes.h>
#include <cstdint>
#include <ctime>
#include <string>

namespace myMuduo {
class Timestamp
{
public:
    Timestamp()
        : microSecondsSinceEpoch_(0)
    {
    }

    Timestamp(const Timestamp &rhs)
        : microSecondsSinceEpoch_(rhs.microSecondsSinceEpoch_)
    {
    }

    explicit Timestamp(int64_t microSecondsSinceEpochArg)
        : microSecondsSinceEpoch_(microSecondsSinceEpochArg)
    {
    }

    ~Timestamp() = default;

    std::string toString() const
    {
        char buf[32];
        int64_t seconds = microSecondsSinceEpoch_ / kMicroSecondsPerSecond;
        int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
        snprintf(buf, sizeof(buf), "%" PRId64 ".%06" PRId64, seconds, microseconds);
        return std::string(buf);
    }

    std::string toFormattedString(bool showMicroseconds = true) const
    {
        char buf[64];
        time_t seconds = static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
        struct tm tm_time;
        localtime_r(&seconds, &tm_time);  // now得到的是UTC时间，localtime_r将其转换为本地时间

        if (showMicroseconds)
        {
            int64_t microseconds = microSecondsSinceEpoch_ % kMicroSecondsPerSecond;
            snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d.%06" PRId64,
                     tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_hour,
                     tm_time.tm_min, tm_time.tm_sec, microseconds);
        }
        else
        {
            snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", tm_time.tm_year + 1900,
                     tm_time.tm_mon + 1, tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        }
        return std::string(buf);
    }

    bool valid() const { return microSecondsSinceEpoch_ > 0; }

    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

    time_t secondsSinceEpoch() const
    {
        return static_cast<time_t>(microSecondsSinceEpoch_ / kMicroSecondsPerSecond);
    }

    // 得到UTC时间
    static Timestamp now()
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        int64_t microSecondsSinceEpoch =
            static_cast<int64_t>(ts.tv_sec) * kMicroSecondsPerSecond + ts.tv_nsec / 1000;
        return Timestamp(microSecondsSinceEpoch);
    }

    static Timestamp invalid() { return Timestamp(); }

    
    Timestamp &operator=(const Timestamp &rhs)
    {
        microSecondsSinceEpoch_ = rhs.microSecondsSinceEpoch_;
        return *this;
    }
    
    // 操作符重载
    bool operator<(const Timestamp &rhs) const
    {
        return microSecondsSinceEpoch_ < rhs.microSecondsSinceEpoch_;
    }

    bool operator==(const Timestamp &rhs) const
    {
        return microSecondsSinceEpoch_ == rhs.microSecondsSinceEpoch_;
    }

    bool operator!=(const Timestamp &rhs) const
    {
        return microSecondsSinceEpoch_ != rhs.microSecondsSinceEpoch_;
    }

    bool operator<=(const Timestamp &rhs) const
    {
        return microSecondsSinceEpoch_ <= rhs.microSecondsSinceEpoch_;
    }

    bool operator>(const Timestamp &rhs) const
    {
        return microSecondsSinceEpoch_ > rhs.microSecondsSinceEpoch_;
    }

    bool operator>=(const Timestamp &rhs) const
    {
        return microSecondsSinceEpoch_ >= rhs.microSecondsSinceEpoch_;
    }

    // 运算符重载
    Timestamp operator+(int64_t seconds) const
    {
        return Timestamp(microSecondsSinceEpoch_ + seconds * kMicroSecondsPerSecond);
    }

    Timestamp operator+(const Timestamp &rhs) const
    {
        return Timestamp(microSecondsSinceEpoch_ + rhs.microSecondsSinceEpoch_);
    }

    Timestamp operator-(int64_t seconds) const
    {
        return Timestamp(microSecondsSinceEpoch_ - seconds * kMicroSecondsPerSecond);
    }

    Timestamp operator-(const Timestamp &rhs) const
    {
        return Timestamp(microSecondsSinceEpoch_ - rhs.microSecondsSinceEpoch_);
    }

private:
    int64_t microSecondsSinceEpoch_;  // 微妙数 (1970-01-01 00:00:00 UTC)
    static const int kMicroSecondsPerSecond = 1e6;
};

}  // namespace myMuduo
