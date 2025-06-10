#include <iostream>
#include "muduo/base/Any.h"
#include "muduo/net/InetAddress.h"
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
    spdlog::info("Address 4: {}", addr1.toIpPort());

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
    return 0;
}

