#include "myMuduo/net/Socket.h"
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Socket.h"
#include "myMuduo/net/InetAddress.h"
#include "spdlog/spdlog.h"

namespace myMuduo {
namespace net {

Socket::Socket(int sockfd)
    : sockfd_(sockfd)
{
}
Socket::~Socket() { close(sockfd_); }

int Socket::fd() const { return sockfd_; }

void Socket::bindAddress(const InetAddress &localaddr)
{
    const struct sockaddr_in *addr = localaddr.getSockAddr();
    if (0 != bind(sockfd_, reinterpret_cast<const struct sockaddr *>(addr), sizeof(sockaddr_in)))
    {
        spdlog::critical("Socket::bindAddress error: {}", strerror(errno));
        abort();
    }
}

void Socket::listen()
{
    if (0 != ::listen(sockfd_, SOMAXCONN))
    {
        spdlog::critical("Socket::listen error: {}", strerror(errno));
        abort();
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addrlen = sizeof(addr);
    int connfd = ::accept(sockfd_, reinterpret_cast<struct sockaddr *>(&addr), &addrlen);
    if (connfd < 0)
    {
        spdlog::critical("Socket::accept error: {}", strerror(errno));
        return -1;  // accept failed
    }
    if (peeraddr)
    {
        peeraddr->setSockAddrInet(addr);
    }
    return connfd;
}

void Socket::shutdownWrite() { shutdown(sockfd_, SHUT_WR); }

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof(optval)));
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(sockfd_, IPPROTO_TCP, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof(optval)));
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(sockfd_, IPPROTO_TCP, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof(optval)));
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    setsockopt(sockfd_, IPPROTO_TCP, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof(optval)));
}

int Socket::createNonblockingOrDie(sa_family_t family)
{
    int sockfd = socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        spdlog::critical("Socket::createNonblockingOrDie error: {}", strerror(errno));
        abort();
    }
    return sockfd;
}

}  // namespace net
}  // namespace myMuduo