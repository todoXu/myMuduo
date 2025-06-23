#pragma once
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string>
#include "myMuduo/base/noncopyable.h"

namespace myMuduo {
namespace net {

class InetAddress;

class Socket : noncopyable
{
public:
    explicit Socket(int sockfd);
    ~Socket();

    int fd() const;
    bool getTcpInfo(struct tcp_info *) const;
    bool getTcpInfoString(std::string &info) const;

    void bindAddress(const InetAddress &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);
    static int getSocketError(int sockfd);
    void shutdownWrite();
    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int sockfd_;
};

}  // namespace net
}  // namespace myMuduo