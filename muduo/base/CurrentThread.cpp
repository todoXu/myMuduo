#include "muduo/base/CurrentThread.h"

namespace myMuduo {
namespace base {
namespace CurrentThread {
thread_local int t_cachedTid = 0;
thread_local char t_tidString[32] = {0};
thread_local int t_tidStringLength = 6;
thread_local const char* t_threadName = "unknown";
}  // namespace CurrentThread
}  // namespace base
}  // namespace myMuduo