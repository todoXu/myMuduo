#include "myMuduo/net/poller/EPollPoller.h"

// 基类不要包含子类的头文件，避免循环依赖，所以新建一个默认Poller的实现文件
// 该文件实现了IPoller接口的默认实现，暂时只支持EPollPoller
namespace myMuduo {
namespace net {
IPoller* IPoller::newDefaultPoller(EventLoop* loop)
{
    //暂时只支持EPollPoller
    //可以根据不同平台或需求扩展其他类型的Poller
    return new EPollPoller(loop);
}

}  // namespace net
}  // namespace myMuduo