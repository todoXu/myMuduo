#pragma once

#include <atomic>
#include <functional>
#include <vector>
#include "muduo/base/Timestamp.h"
#include "muduo/base/noncopyable.h"

namespace myMuduo {
namespace net {

class Channel;
class Poller;
class TimerQueue;

//1个线程拥有1个 EventLoop 对象 且该对象只能在该线程中使用
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    Timestamp pollReturnTime() const;
    int64_t iteration() const;
    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);
    size_t queueSize() const;

    //TimerId runAt(Timestamp time, TimerCallback cb);
    //TimerId runAfter(double delay, TimerCallback cb);
    //TimerId runEvery(double interval, TimerCallback cb);
    //void cancel(TimerId timerId);

    void wakeup(); 
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel) const;

    void assertInLoopThread() const;
    bool isInLoopThread() const;
    bool eventHandling() const;
    



private:
    void abortNotInLoopThread();
    void handleRead();  // wake up
    void doPendingFunctors();

    bool looping_;            // 是否在事件循环中
    std::atomic<bool> quit_;  // 是否退出事件循环
    bool eventHandling_;
};

}  // namespace net
}  // namespace myMuduo