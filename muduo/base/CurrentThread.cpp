#include "muduo/base/CurrentThread.h"

namespace myMuduo {
namespace base {
namespace CurrentThread {
thread_local int t_cachedTid = 0;
thread_local char t_tidString[32] = {0};
thread_local int t_tidStringLength = 6;
thread_local const char* t_threadName = "unknown";
//tid缓存
void cacheTid()
{
    if (t_cachedTid == 0)
    {
        t_cachedTid = static_cast<pid_t>(syscall(SYS_gettid));
        t_tidStringLength = snprintf(t_tidString, sizeof(t_tidString), "%5d", t_cachedTid);
    }
}

pid_t tid()
{
    if (t_cachedTid == 0)
    {
        cacheTid();
    }
    return t_cachedTid;
}

const char* tidString()
{
    if (t_cachedTid == 0)
    {
        cacheTid();
    }
    return t_tidString;
}

int tidStringLength()
{
    if (t_cachedTid == 0)
    {
        cacheTid();
    }
    return t_tidStringLength;
}

const char* threadName()
{
    if (t_cachedTid == 0)
    {
        cacheTid();
    }
    return t_threadName ? t_threadName : "unknown";
}
}  // namespace CurrentThread
}  // namespace base
}  // namespace myMuduo