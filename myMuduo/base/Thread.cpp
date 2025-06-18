#include "myMuduo/base/Thread.h"
#include <semaphore.h>
#include "myMuduo/base/CurrentThread.h"

namespace myMuduo {
namespace base {

std::atomic<int32_t> Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string& name)
    : started_(false)
    , joined_(false)
    , thread_(nullptr)
    , func_(std::move(func))
    , tid_(0)
    , name_(name)
{
    numCreated_.fetch_add(1);
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
    {
        thread_->detach();
    }
}

void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, 0, 0);
    // 需要在 lambda 里访问或修改外部变量（如成员变量）时，用 [&] 或 [this]。
    // 不需要访问外部变量时，用 []
    thread_ = std::unique_ptr<std::thread>(new std::thread([this, &sem]() {
        //获取真正的线程ID  而不是Thread的线程ID
        tid_ = CurrentThread::tid();
        sem_post(&sem);
        func_();
    }));

    //确保获取到真实的线程ID
    sem_wait(&sem);
}

//等待线程结束
void Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int32_t num = numCreated_.load();
    if (name_.empty())
    {
        name_ = "Thread-" + std::to_string(num);
    }
}
}  // namespace base
}  // namespace myMuduo
