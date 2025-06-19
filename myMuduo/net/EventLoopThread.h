#pragma once
#include <functional>
#include "condition_variable"
#include "myMuduo/base/Thread.h"
#include "myMuduo/base/noncopyable.h"

namespace myMuduo {
namespace net {

class EventLoop;
// EventLoopThread类用于在一个独立的线程中运行EventLoop
class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                    const std::string& name = std::string());
    ~EventLoopThread();
    EventLoop* startLoop();

private:
    void threadFunc();
    
    EventLoop* loop_;
    bool exiting_;
    base::Thread thread_;
    std::condition_variable cond_;
    std::mutex mutex_;
    ThreadInitCallback initCallback_;
};

}  // namespace base
}  // namespace myMuduo