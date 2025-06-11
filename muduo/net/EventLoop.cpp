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
};

IgnoreSipPipe ignoreSipPipe;

//占位
EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , eventHandling_(false)
    , callingPendingFunctors_(false)
    , iteration_(0)
    , threadId_(base::CurrentThread::tid())
    , pollerPtr_(IPoller::newDefaultPoller(this))
    , wakeupFd_(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC))  // 创建一个非阻塞的eventfd
    , wakeupChannelPtr_(new Channel(this, wakeupFd_))
{
}

EventLoop::~EventLoop() {}
void EventLoop::loop() {}
void EventLoop::quit() {}
Timestamp EventLoop::pollReturnTime() const { return Timestamp(); }
int64_t EventLoop::iteration() const { return 0; }
void EventLoop::runInLoop(Functor cb) { cb(); }
void EventLoop::queueInLoop(Functor cb) { cb(); }
size_t EventLoop::queueSize() const { return size_t(); }

void EventLoop::wakeup() {}
void EventLoop::updateChannel(Channel *channel) { channel->fd(); }
void EventLoop::removeChannel(Channel *channel) { channel->fd(); }
bool EventLoop::hasChannel(Channel *channel) const
{
    channel->fd();
    return false;
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

void EventLoop::setContext(const base::Any &context) {
    context_ = context;
}

const base::Any &EventLoop::getContext() const { return context_; }

EventLoop *EventLoop::getEventLoopOfCurrentThread() { return nullptr; }
void EventLoop::abortNotInLoopThread()
{
    spdlog::critical(
        "EventLoop::abortNotInLoopThread() - EventLoop can only be used in the thread it was "
        "created in, but current thread id is {}, EventLoop thread id is {}",
        base::CurrentThread::tid(), threadId_);

    abort();
}

void EventLoop::handleRead() {}
void EventLoop::doPendingFunctors() {}
void EventLoop::printActiveChannels() const {}
}  // namespace net

}  // namespace myMuduo
