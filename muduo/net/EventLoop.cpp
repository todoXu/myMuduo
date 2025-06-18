#include "muduo/net/EventLoop.h"
#include <signal.h>
#include <sys/eventfd.h>
#include "EventLoop.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/net/Channel.h"
#include "muduo/net/poller/Poller.h"
#include "spdlog/spdlog.h"

namespace myMuduo {
namespace net {

// 在当前线程中存储EventLoop的指针
thread_local EventLoop *t_loopInThisThread = nullptr;
const int kPollTimeMs = 10 * 1000;

class IgnoreSipPipe
{
public:
    IgnoreSipPipe()
    {
        // 忽略SIGPIPE信号，防止在写入已关闭的socket时导致程序异常终止
        signal(SIGPIPE, SIG_IGN);
    }
    ~IgnoreSipPipe() = default;
};

IgnoreSipPipe ignoreSipPipe;

EventLoop *EventLoop::getEventLoopOfCurrentThread() { return t_loopInThisThread; }

//eventloop的方法 除了loop循环,只有2种调用方式
//1、通过本线程回调调用,此时eventloop是醒着的
//2、在其他线程中调用时，先将回调函数放入队列
EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , eventHandling_(false)
    , callingPendingFunctors_(false)
    , threadId_(base::CurrentThread::tid())
    , pollerPtr_(IPoller::newDefaultPoller(this))
    , wakeupFd_(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC))  // 创建一个非阻塞的eventfd
    , wakeupChannelPtr_(new Channel(this, wakeupFd_))
    , currentActiveChannel_(nullptr)
{
    spdlog::debug("EventLoop created in thread {}", threadId_);
    if (t_loopInThisThread)
    {
        spdlog::critical("Another EventLoop {} exists in this thread {}", t_loopInThisThread, threadId_);
        abortNotInLoopThread();
    }
    else
    {
        t_loopInThisThread = this;
    }

    // 设置wakeupChannel的回调函数，当wakeupFd有可读事件时触发
    wakeupChannelPtr_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannelPtr_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannelPtr_->disableAll();
    wakeupChannelPtr_->remove();
    close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;

    while (!quit_)
    {
        activeChannels_.clear();
        //有2类fd wakeupFd 和 connFd
        pollReturnTime_ = pollerPtr_->poll(kPollTimeMs, &activeChannels_);

        printActiveChannels();

        eventHandling_ = true;
        for (auto it = activeChannels_.begin(); it != activeChannels_.end(); ++it)
        {
            currentActiveChannel_ = *it;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = nullptr;
        eventHandling_ = false;
        //处理channel回调完成后 再执行之前queueInLoop里注册的回调
        //主要是baseloop在接收到新连接时, 调用ioLoop的连接建立回调
        doPendingFunctors();
    }

    spdlog::info("EventLoop::loop() - EventLoop {} quit", threadId_);
    looping_ = false;
}

void EventLoop::quit()
{
    //如果是当前线程调用quit，直接设置quit_为true
    //loop()阻塞了代码 本线程想调用quite只能通过回调  那loop就已经醒了  调用完回调后循环就会退出
    //如果是其他线程调用quit，设置quit_为true，并唤醒该loop线程  执行完回调后循环就会退出
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}
Timestamp EventLoop::pollReturnTime() const { return pollReturnTime_; }

//当前loop被其他线程调用runInLoop时，调用queueInLoop，将cb放入队列中，接着wakup唤醒该loop，在该loop中处理cb
//本loop线程调用runInLoop时，直接执行cb
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread())
    {
        cb();  // 如果是当前线程，直接执行回调
    }
    else
    {
        queueInLoop(std::move(cb));  // 否则将回调放入队列中
    }
}

//排队到loop下次循环时执行
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::lock_guard<std::mutex> lock(functorMutex_);
        pendingFunctors_.push_back(std::move(cb));
    }

    //1、非本loop线程调用queueInLoop时，直接唤醒该loop线程
    //2、本loop线程调用queueInLoop时
    //如果当前正在处理回调，则唤醒该loop线程，让loop能进入下次循环，执行该任务
    //如果刚好在115-128行 则不要唤醒  push_back后在doPendingFunctors中会处理
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

size_t EventLoop::queueSize()
{
    std::lock_guard<std::mutex> lock(functorMutex_);
    return pendingFunctors_.size();
}

void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        spdlog::error("EventLoop::wakeup() - write {} bytes instead of {}, fd={}", n, sizeof(one), wakeupFd_);
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    pollerPtr_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    pollerPtr_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    pollerPtr_->hasChannel(channel);
}

void EventLoop::assertInLoopThread()
{
    if (!isInLoopThread())
    {
        abortNotInLoopThread();
    }
}

bool EventLoop::isInLoopThread() const { return threadId_ == base::CurrentThread::tid(); }

bool EventLoop::eventHandling() const { return false; }

void EventLoop::setContext(const base::Any &context) { context_ = context; }

const base::Any &EventLoop::getContext() const { return context_; }

EventLoop *EventLoop::getEventLoopOfCurrentThread() { return t_loopInThisThread; }

void EventLoop::abortNotInLoopThread()
{
    spdlog::critical(
        "EventLoop::abortNotInLoopThread() - EventLoop can only be used in the thread it was "
        "created in, but current thread id is {}, EventLoop thread id is {}",
        base::CurrentThread::tid(), threadId_);

    abort();
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof(one));
    if (n != sizeof(one))
    {
        spdlog::error("EventLoop::handleRead() - read {} bytes instead of {}, fd={}", n, sizeof(one), wakeupFd_);
    }
}
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::lock_guard<std::mutex> lock(functorMutex_);
        functors.swap(pendingFunctors_);
    }

    for (auto it = functors.begin(); it != functors.end(); ++it)
    {
        (*it)();
    }
    callingPendingFunctors_ = false;
}

void EventLoop::printActiveChannels() const
{
    for (auto it = activeChannels_.begin(); it != activeChannels_.end(); ++it)
    {
        Channel *channel = *it;
        spdlog::info("activeChannels_ = {}", channel->reventsToString());
    }

}  // namespace net

}  // namespace myMuduo
