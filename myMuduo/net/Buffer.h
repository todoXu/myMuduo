#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace myMuduo {
namespace net {

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      headerIndex   <=   tailerIndex    <=     size

class Buffer
{
public:
    //实际buffer在多次使用后永远会有至少8字节的预留空间来写入头
    //由扩容和buffer构造方式来保证  头少于8字节的直接用prepend就可以写入  超过8字节的需要用sendMyComplexPacket这个例子里的方式来构造buffer
    static const size_t kCheapPrepend = 8;    // 初始预留空间为8字节
    static const size_t kInitialSize = 1024;  // 初始大小为1024字节

    explicit Buffer(int initialSize = kInitialSize);

    ~Buffer();

    Buffer(const Buffer&) = delete;  // 禁止拷贝构造

    size_t readableBytesLength() const;
    size_t writableBytesLength() const;

    size_t prependableBytesLength() const;

    const char* peek();

    char* beginWrite();

    void retrieve(size_t len);

    void retrieveAll();

    std::string retrieveAllAsString();

    std::string retrieveAsString(size_t len);

    void ensureWritableBytes(size_t len);

    void hasWritten(size_t len);

    void append(const char* data, size_t len);

    void prepend(const char* data, size_t len);

    const char* findCRLF(const char* start);

    const char* findCRLF();

    ssize_t readFd(int fd, int* savedErrno);

    void swap(Buffer& rhs);

private:
    char* begin();

    void makeSpace(size_t len);

    std::vector<char> buffer_;
    size_t headIndex_;  //数据起始位置
    size_t tailIndex_;  //数据结束位置
    static const char kCRLF[];
};

}  // namespace net
}  // namespace myMuduo

// Definition of static member

/*
演示如何构造并发送一个复杂头部的数据包

// 假设这是我们的自定义头部结构
struct MyComplexHeader
{
  int64_t checksum;
  int64_t timestamp;
  int32_t payload_len;
};

void sendMyComplexPacket(const muduo::net::TcpConnectionPtr& conn,
                         const muduo::string& payload)
{
  // 在实际应用中，我们通常直接使用 TcpConnection 自带的 outputBuffer_
  // 这里为了演示，我们创建一个临时的 Buffer。
  // 如果是使用 outputBuffer_，第一步应该是 outputBuffer_->retrieveAll() 来清空。
  muduo::net::Buffer buf;

  // 1. 定义头部大小，并为头部预留空间
  const size_t headerLen = sizeof(MyComplexHeader);
  buf.ensureWritableBytes(headerLen); // 确保有足够空间写入
  buf.hasWritten(headerLen);          // 关键：将 writerIndex_ 向右移动，"占住"头部的空间

  // 2. 追加真正的数据体
  // 即使这一步因为 payload 太大而触发了 makeSpace 的内部整理，
  // 我们预留的 headerLen 字节空间也会被当作有效数据一并移动，不会丢失。
  buf.append(payload);

  // 3. 计算并填充头部
  MyComplexHeader header;
  header.payload_len = static_cast<int32_t>(payload.size());
  header.timestamp = muduo::Timestamp::now().microSecondsSinceEpoch();
  
  // 计算校验和。注意：校验和是针对数据体的，数据体在预留的头部空间之后。
  const char* payload_start = buf.peek() + headerLen;
  header.checksum = calculateChecksum(payload_start, payload.size());

  // 将所有头部字段转换成网络字节序
  header.payload_len = muduo::net::sockets::hostToNetwork32(header.payload_len);
  header.timestamp = muduo::net::sockets::hostToNetwork64(header.timestamp);
  header.checksum = muduo::net::sockets::hostToNetwork64(header.checksum);

  // 4. 回头将计算好的头部内容拷贝到之前预留的空间中
  char* header_start = buf.begin() + buf.readerIndex();
  ::memcpy(header_start, &header, headerLen);

  // 5. 发送整个 Buffer
  // 此时 Buffer 中的内容是 [填充好的头部] + [数据体]
  conn->send(&buf);
}
*/