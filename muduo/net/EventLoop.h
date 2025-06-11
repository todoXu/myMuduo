#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <vector>
#include "muduo/base/Any.h"
#include "muduo/base/Timestamp.h"
#include "muduo/base/noncopyable.h"

namespace myMuduo {
namespace net {

class Channel;
class IPoller;
class TimerQueue;

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

    void assertInLoopThread();
    bool isInLoopThread() const;
    bool eventHandling() const;

    void setContext(const base::Any &context);
    const base::Any &getContext() const;

    static EventLoop *getEventLoopOfCurrentThread();

private:
    void abortNotInLoopThread();
    void handleRead();
    void doPendingFunctors();

    void printActiveChannels() const;

    using ChannelList = std::vector<Channel *>;

    std::atomic<bool> looping_;            // 是否在事件循环中
    std::atomic<bool> quit_;  // 是否退出事件循环
    std::atomic<bool> eventHandling_;
    std::atomic<bool> callingPendingFunctors_;
    int64_t iteration_;
    const pid_t threadId_;      // 创建EventLoop的线程ID
    Timestamp pollReturnTime_;  // 上次poll的返回时间

    std::unique_ptr<IPoller> pollerPtr_;

    //std::unique_ptr<TimerQueue> timerQueue_;
    
    int wakeupFd_; //当主loop接收到新连接时，唤醒子loop
    std::unique_ptr<Channel> wakeupChannelPtr_;
    
    base::Any context_;

    ChannelList activeChannels_;
    Channel *currentActiveChannel_; //currentActiveChannel_不拥有对象 只是临时指向正在处理的Channel

    std::mutex functorMutex_;  // 保护 pendingFunctors_
    std::vector<Functor> pendingFunctors_;
};

}  // namespace net
}  // namespace myMuduo