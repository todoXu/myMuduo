#include "myMuduo/base/Thread.h"
#include <semaphore.h>
#include "myMuduo/base/CurrentThread.h"

namespace myMuduo {
namespace base {

std::atomic<int32_t> Thread::numCreated_(0);

Thread::Thread(ThreadFunc func, const std::string& name)
    : started_(false)
    , joined_(false)
    , threadPtr_(nullptr)
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
        threadPtr_->detach();
    }
}

void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, 0, 0);
    // 需要在 lambda 里访问或修改外部变量时，用 [&] 按引用方式捕获变量，在类成员函数中使用时，[&] 也会隐式地捕获 this 指针
    // 不需要访问外部变量时，用 []
    threadPtr_ = std::unique_ptr<std::thread>(new std::thread([&]() {
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
    threadPtr_->join();
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
