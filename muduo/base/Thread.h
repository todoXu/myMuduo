#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include "muduo/base/noncopyable.h"

namespace myMuduo {
namespace base {
class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;
    explicit Thread(ThreadFunc func, const std::string& name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const { return started_; }
    pid_t tid() { return tid_; }
    const std::string& name() const { return name_; }
    static int32_t numCreated() { return numCreated_.load(); }

private:
    void setDefaultName();

    bool started_;
    bool joined_;
    std::unique_ptr<std::thread> thread_;
    ThreadFunc func_;
    pid_t tid_;
    std::string name_;
    static std::atomic<int32_t> numCreated_;
};

}  // namespace base

}  // namespace myMuduo
