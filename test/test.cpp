#include <iostream>
#include "muduo/base/Timestamp.h"
#include "muduo/net/InetAddress.h"
int main()
{
    myMuduo::net::InetAddress addr1(8080, true); // 只接受本机的请求
    std::cout << "Address 1: " << addr1.toIpPort() << std::endl;

    myMuduo::net::InetAddress addr2("127.0.0.1", 8011);
    std::cout << "Address 2: " << addr2.toIpPort() << std::endl;

    myMuduo::net::InetAddress addr3;
    myMuduo::net::InetAddress::resolve("localhost", &addr3);
    std::cout << "Address 3: " << addr3.toIpPort() << std::endl;
    myMuduo::net::InetAddress addr4;
    myMuduo::net::InetAddress::resolve("www.baidu.com", &addr4);
    std::cout << "Address 4: " << addr4.toIpPort() << std::endl;
    return 0;
}