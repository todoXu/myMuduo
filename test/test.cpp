#include <functional>
#include <string>
#include "myMuduo/base/Timestamp.h"
#include "myMuduo/net/Buffer.h"
#include "myMuduo/net/EventLoop.h"
#include "myMuduo/net/TcpServer.h"

class ChatServer
{
public:
    ChatServer(myMuduo::net::EventLoop *loop, const myMuduo::net::InetAddress &listenAddr, const std::string &name)
        : loop_(loop)
        , server_(loop, listenAddr, name)
    {
        // 设置连接回调函数
        server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, std::placeholders::_1));
        // 设置消息回调函数
        server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, std::placeholders::_1,
                                             std::placeholders::_2, std::placeholders::_3));
        // 设置线程池大小
        server_.setThreadNum(4);
    }

    void start()
    {
        server_.start();
        printf("ChatServer started at %s\n", server_.ipPort().c_str());
    }

private:
    // 处理连接和断开
    void onConnection(const myMuduo::net::TcpConnectionPtr &tcpConnectionPtr)
    {
        if (tcpConnectionPtr->connected())
        {
            printf("ChatServer - New connection from %s To %s\n",
                   tcpConnectionPtr->getPeerAddress().toIpPort().c_str(),
                   tcpConnectionPtr->getLocalAddress().toIpPort().c_str());
        }
        else
        {
            printf("ChatServer - Connection from %s closed\n",
                   tcpConnectionPtr->getPeerAddress().toIpPort().c_str());
            tcpConnectionPtr->shutdown();  // 关闭连接
            loop_->quit();                 // 退出事件循环
        }
    }
    // 处理接收到的消息
    void onMessage(const myMuduo::net::TcpConnectionPtr &tcpConnectionPtr, myMuduo::net::Buffer *buffer,
                   myMuduo::Timestamp timestamp)
    {
        std::string buf = buffer->retrieveAllAsString();
        printf("ChatServer - Received message from %s: %s time = %s\n",
               tcpConnectionPtr->getPeerAddress().toIpPort().c_str(), buf.c_str(), timestamp.toFormattedString().c_str());
        tcpConnectionPtr->send(buf);  // Echo the message back to the client
    }
    myMuduo::net::EventLoop *loop_;
    myMuduo::net::TcpServer server_;
};

int main(int, char **)
{
    myMuduo::net::EventLoop eventloop;
    myMuduo::net::InetAddress listenAddr("127.0.0.1", 8888);
    std::string serverName = "ChatServer";
    ChatServer server(&eventloop, listenAddr, serverName);
    server.start();
    eventloop.loop();  // 相当于epoll_wait,阻塞等待事件发生 1个线程1个eventloop

    return 0;
}
