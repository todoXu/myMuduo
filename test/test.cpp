#include <iostream>
#include "chrono"
#include "myMuduo/base/Any.h"
#include "myMuduo/base/CurrentThread.h"
#include "myMuduo/base/Thread.h"
#include "myMuduo/net/InetAddress.h"
#include "spdlog/spdlog.h"

int main()
{
    myMuduo::net::InetAddress addr1(8080, true);  // 只接受本机的请求
    spdlog::info("Address 1: {}", addr1.toIpPort());

    myMuduo::net::InetAddress addr2("127.0.0.1", 8011);
    spdlog::info("Address 2: {}", addr1.toIpPort());

    myMuduo::net::InetAddress addr3;
    myMuduo::net::InetAddress::resolve("localhost", &addr3);
    spdlog::info("Address 3: {}", addr1.toIpPort());

    myMuduo::net::InetAddress addr4;
    myMuduo::net::InetAddress::resolve("www.baidu.com", &addr4);
    spdlog::info("Address 4: {}", addr4.toIpPort());

    myMuduo::base::Any any1(42);
    myMuduo::base::Any any2(std::string("Hello, World!"));
    myMuduo::base::Any any3((float)1.1);

    struct test
    {
        int a;
    };
    myMuduo::base::Any any4(test{222});
    spdlog::info("Any1 type: {}, value: {}", any1.type().name(), *any1.cast<int>());
    spdlog::info("Any2 type: {}, value: {}", any2.type().name(), *any2.cast<std::string>());
    spdlog::info("Any2 type: {}, value: {}", any3.type().name(), *any3.cast<float>());
    spdlog::info("Any4 type: {}, value: {}", any4.type().name(), any4.cast<test>()->a);

    myMuduo::base::Thread thread(
        []() {
            for (int i = 0; i < 10; ++i)
            {
                spdlog::info("Thread is running in thread {}", myMuduo::base::CurrentThread::tid());
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        },
        "TestThread");
    spdlog::info("mainThread {}", myMuduo::base::CurrentThread::tid());
    spdlog::info("Thread started with Name: {}", thread.name());
    thread.start();
    thread.join();
    spdlog::info("Thread ID = {}  Name = {} has finished", thread.tid(), thread.name());

    return 0;
}
