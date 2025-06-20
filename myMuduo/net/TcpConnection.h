#pragma once
#include <memory>
#include "myMuduo/base/noncopyable.h"

namespace myMuduo {
namespace net {
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
};

}  // namespace net
}  // namespace myMuduo