#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <vector>
#include "myMuduo/base/Any.h"
#include "myMuduo/base/Callback.h"
#include "myMuduo/base/TimerId.h"
#include "myMuduo/base/TimerQueue.h"
#include "myMuduo/base/Timestamp.h"
#include "myMuduo/base/noncopyable.h"

namespace myMuduo {
namespace net {

class Channel;
class IPoller;

//1个TcpServer拥有1个baseLoop 同时会有1个EventLoopPool
//baseLoop负责监听新连接的到来  并将新连接的connfd分配给tcpConnect
//新连接建立时 从EventLoopPool中获取一个ioLoop来分配给tcpConnnect
//新连接建立时 在baseloop的线程里操作ioLoop 处理连接建立的响应
//对连接的响应都通过对应ioLoop线程来处理
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    Timestamp pollReturnTime() const;
    void runInLoop(Functor cb);
    void queueInLoop(Functor cb);
    size_t queueSize();

    base::TimerId runAt(Timestamp time, base::TimerCallback cb);
    base::TimerId runAfter(double delay, base::TimerCallback cb);
    base::TimerId runEvery(double interval, base::TimerCallback cb);
    void cancel(base::TimerId timerId);

    void wakeup();
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);

    void assertInLoopThread();
    bool isInLoopThread() const;
    bool eventHandling() const;

    void setContext(const base::Any &context);
    const base::Any &getContext() const;

    static EventLoop *getEventLoopOfCurrentThread();

private:
    void abortNotInLoopThread();
    void handleRead(const Timestamp &timestamp);
    void doPendingFunctors();

    void printActiveChannels() const;

    using ChannelList = std::vector<Channel *>;

    std::atomic<bool> looping_;  // 是否在事件循环中
    std::atomic<bool> quit_;     // 是否退出事件循环
    std::atomic<bool> eventHandling_;
    std::atomic<bool> callingPendingFunctors_;
    const pid_t threadId_;      // 创建EventLoop的线程ID
    Timestamp pollReturnTime_;  // 上次poll的返回时间

    std::unique_ptr<IPoller> pollerPtr_;
    std::unique_ptr<myMuduo::base::TimerQueue> timerQueuePtr_;

    int wakeupFd_;  //当baseloop接收到新连接时，通过wakeupfd唤醒ioLoop线程
    std::unique_ptr<Channel> wakeupChannelPtr_;

    base::Any context_;

    ChannelList activeChannels_;
    Channel *currentActiveChannel_;  //currentActiveChannel_不拥有对象 只是临时指向正在处理的Channel

    std::mutex functorMutex_;  // 保护 pendingFunctors_
    std::vector<Functor> pendingFunctors_;
};

}  // namespace net
}  // namespace myMuduo