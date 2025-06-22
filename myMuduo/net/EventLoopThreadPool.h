#pragma once
#include <functional>
#include <string>
#include <vector>
#include "myMuduo/base/noncopyable.h"
#include "myMuduo/net/EventLoopThread.h"
#include "myMuduo/net/Callback.h"

namespace myMuduo {
namespace net {

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    EventLoopThreadPool(EventLoop* baseLoop, const std::string& name = std::string());
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }
    void start(const ThreadInitCallback& cb = ThreadInitCallback());

    EventLoop* getNextLoop();
    std::vector<EventLoop*> getAllLoops();

    bool started() const { return started_; }
    const std::string& name() const { return name_; }

private:
    EventLoop *baseLoop_;
    std::string name_;
    bool started_;
    int numThreads_;
    //不拥有loop的实际线程对象
    std::vector<EventLoop*> loops_;
    //实际的线程对象
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    int threadIndex_;


};

}  // namespace net
}  // namespace myMuduo