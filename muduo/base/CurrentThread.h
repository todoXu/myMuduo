#pragma once

#include <sys/syscall.h>
#include <unistd.h>
#include <cstdio>

namespace myMuduo {
namespace base {
namespace CurrentThread {
extern thread_local pid_t t_cachedTid;
extern thread_local char t_tidString[32];
extern thread_local int t_tidStringLength;
extern thread_local const char* t_threadName;

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
