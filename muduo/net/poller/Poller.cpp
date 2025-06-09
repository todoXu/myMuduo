#include "muduo/net/poller/Poller.h"
#include "muduo/net/poller/EPollPoller.h"

namespace myMuduo {
namespace net {
IPoller* IPoller::newDefaultPoller(EventLoop* loop)
{
    //暂时只支持EPollPoller
    //可以根据不同平台或需求扩展其他类型的Poller
    return new EPollPoller(loop);
    return nullptr;
}

}  // namespace net
}  // namespace myMuduo