#include "muduo/net/InetAddress.h"

#include <arpa/inet.h>
#include <netdb.h>

#include <cassert>
#include <cstring>

#include "spdlog/spdlog.h"

namespace myMuduo {
namespace net {

InetAddress::InetAddress(uint16_t port, bool loopbackOnly)
{
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    if (loopbackOnly)
    {
        addr_.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // 只接受本机的请求
    }
    else
    {
        addr_.sin_addr.s_addr = htonl(INADDR_ANY);  // 接受任意地址的请求
    }
}

InetAddress::InetAddress(std::string ip, uint16_t port)
{
    memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr) <= 0)
    {
        spdlog::error("Invalid IP address: {}", ip);
    }
}

InetAddress::InetAddress(const struct sockaddr_in &addr)
    : addr_(addr)
{
}

std::string InetAddress::toIp() const
{
    char buf[64] = "";
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    return std::string(buf);
}

std::string InetAddress::toIpPort() const
{
    char buf[64] = "";
    std::string ip = toIp();
    uint16_t port = ntohs(addr_.sin_port);
    snprintf(buf, sizeof(buf), "%s:%u", ip.c_str(), port);

    return std::string(buf);
}

uint16_t InetAddress::port() const { return ntohs(portNetEndian()); }

uint32_t InetAddress::ipv4NetEndian() const
{
    assert(family() == AF_INET);
    return addr_.sin_addr.s_addr;
}

bool InetAddress::resolve(const std::string &hostname, InetAddress *result)
{
    assert(result != nullptr);
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));  //对解析不做任何限制

    int ret = getaddrinfo(hostname.c_str(), nullptr, &hints, &res);
    if (ret != 0)
    {
        spdlog::error("getaddrinfo failed: {}", gai_strerror(ret));
        return false;
    }

    // 遍历所有结果，找到第一个有效的 IPv4 地址
    bool found = false;
    for (struct addrinfo *rp = res; rp != nullptr; rp = rp->ai_next)
    {
        if (rp->ai_family == AF_INET && rp->ai_addrlen == sizeof(struct sockaddr_in))
        {
            struct sockaddr_in *addr = (struct sockaddr_in *)rp->ai_addr;
            result->addr_ = *addr;
            found = true;
            break;
        }
    }

    freeaddrinfo(res);
    return found;
}

};  // namespace net
}  // namespace myMuduo
