#pragma once

#include <netinet/in.h>

#include <cstdint>
#include <string>

namespace myMuduo {
namespace net {
// 不考虑 IPv6 的情况，直接使用 sockaddr_in
class InetAddress
{
public:
    explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false);
    InetAddress(std::string ip, uint16_t port);
    explicit InetAddress(const struct sockaddr_in &addr);
    sa_family_t family() const { return addr_.sin_family; }
    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t port() const;
    uint32_t ipv4NetEndian() const;
    uint16_t portNetEndian() const { return addr_.sin_port; }
    static bool resolve(const std::string &hostname, InetAddress *result);
    const struct sockaddr_in *getSockAddr() const { return &addr_; }

private:
    struct sockaddr_in addr_;
};
}  // namespace net
}  // namespace myMuduo