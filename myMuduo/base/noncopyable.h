#pragma once
/*
noncopyable被用来禁止类的拷贝构造和赋值操作。
它是一个基类，其他类可以继承它来实现禁止拷贝和赋值的功能。
*/
namespace myMuduo {
class noncopyable
{
public:
    noncopyable(const noncopyable &) = delete;             // 禁止拷贝构造
    noncopyable &operator=(const noncopyable &) = delete;  // 禁止赋值操作
    noncopyable(noncopyable &&) = delete;                  // 禁止移动构造
    noncopyable &operator=(noncopyable &&) = delete;       // 禁止移动赋值

protected:
    noncopyable() = default;
    ~noncopyable() = default;
};
}  // namespace myMuduo