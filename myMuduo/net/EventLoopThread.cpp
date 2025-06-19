#include "myMuduo/net/EventLoopThread.h"
#include <mutex>
#include "myMuduo/net/EventLoop.h"

namespace myMuduo {
namespace net {

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const std::string& name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name)
    , cond_()
    , mutex_()
    , initCallback_(cb)
{
}
EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}
EventLoop* EventLoopThread::startLoop()
{
    thread_.start();
    EventLoop* loop = nullptr;

    //获取真实的loop对象
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr)
        {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::threadFunc()
{
    //在新线程中创建EventLoop对象 one loop per thread
    //这个loop生命周期跟新线程一致
    EventLoop loop;
    if (initCallback_)
    {
        initCallback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_all();  // 通知startLoop  EventLoop已经创建完成
    }

    loop.loop();

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = nullptr;
    }
}

}  // namespace net
}  // namespace myMuduo