#include "myMuduo/net/EventLoopThreadPool.h"
#include <cassert>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>
#include "myMuduo/net/EventLoop.h"
#include "myMuduo/net/EventLoopThread.h"
#include "spdlog/spdlog.h"
namespace myMuduo {
namespace net {

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string& name)
    : baseLoop_(baseLoop)
    , name_(name)
    , started_(false)
    , numThreads_(0)
    , threadIndex_(0)
{
    // 初始化线程池
    if (baseLoop_ == nullptr)
    {
        spdlog::critical("EventLoopThreadPool must be created with a valid base loop");
        abort();
    }
}
EventLoopThreadPool::~EventLoopThreadPool() {}

void EventLoopThreadPool::start(const ThreadInitCallback& cb)
{
    baseLoop_->assertInLoopThread();
    started_ = true;

    for (int i = 0; i < numThreads_; ++i)
    {
        std::string threadName = name_ + std::to_string(i);
        std::unique_ptr<EventLoopThread> threadPtr(new EventLoopThread(cb, threadName));
        
        EventLoop* tmpLoop = threadPtr->startLoop();
        //loops_只是用一下loop 不管理loop
        loops_.push_back(tmpLoop);
        
        //thread的所有权转移到 threads_中
        //管理thread的生命周期 让thread的生命周期和EventLoopThreadPool的生命周期一致
        threads_.push_back(std::move(threadPtr));
    }

    if (numThreads_ == 0 && cb)
    {
        // 如果没有子线程，直接在baseLoop上执行回调
        cb(baseLoop_);
    }
}

EventLoop* EventLoopThreadPool::getNextLoop()
{
    baseLoop_->assertInLoopThread();
    assert(started_);

    // 如果没有线程池，直接返回baseLoop
    if (loops_.empty())
    {
        return baseLoop_;
    }

    // 轮询获取下一个EventLoop
    EventLoop* loop = loops_[threadIndex_];
    threadIndex_ = (threadIndex_ + 1) % loops_.size();
    return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops()
{
    baseLoop_->assertInLoopThread();
    assert(started_);

    if (!loops_.empty())
    {
        return loops_;
    }
    else
    {
        return std::vector<EventLoop*>{baseLoop_};
    }
}

}  // namespace net
}  // namespace myMuduo