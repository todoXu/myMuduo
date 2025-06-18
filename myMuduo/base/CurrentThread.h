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
void cacheTid();

pid_t tid();

const char* tidString();

int tidStringLength();

const char* threadName();

}  // namespace CurrentThread

}  // namespace base

}  // namespace myMuduo
