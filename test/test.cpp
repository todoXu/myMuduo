#include <iostream>
#include "muduo/base/Timestamp.h"
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
    return 0;
}