#pragma once
#include <cassert>
#include <cstddef>
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

    explicit Buffer(int initialSize = kInitialSize)
        : buffer_(kCheapPrepend + initialSize)
        , headIndex_(kCheapPrepend)
        , tailIndex_(kCheapPrepend) {};

    ~Buffer() = default;

    Buffer(const Buffer&) = delete;  // 禁止拷贝构造
    size_t readableBytesLength() const { return tailIndex_ - headIndex_; }

    size_t writableBytesLength() const { return buffer_.size() - tailIndex_; }

    size_t prependableBytesLength() const { return headIndex_; }

    //返回数据的起始位置
    const char* peek() { return begin() + headIndex_; }

    //标记缓冲区头部的一部分数据为“已消耗”，使其不再可读
    //这通常在数据被应用程序完整处理后调用。
    void retrieve(size_t len)
    {
        assert(len <= readableBytesLength());
        if (len < readableBytesLength())
        {
            headIndex_ += len;
        }
        else
        {
            //数据已经被读完了 重置缓冲区的数据索引位置
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        headIndex_ = kCheapPrepend;
        tailIndex_ = kCheapPrepend;
    }

    std::string retrieveAllAsString() { return retrieveAsString(readableBytesLength()); }

    std::string retrieveAsString(size_t len)
    {
        assert(len <= readableBytesLength());
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    void ensureWritableBytes(size_t len)
    {
        if (writableBytesLength() < len)
        {
            //如果可写空间不足，扩展缓冲区
            makeSpace(len);
        }
        assert(writableBytesLength() >= len);
    }

private:
    char* begin() { return buffer_.data(); }

    void makeSpace(size_t len)
    {
        //实际可用空间 = 可写空间 + 预留空间
        size_t realLeftSize = writableBytesLength() + prependableBytesLength();

        //尽可能内部整理 不要重新分配内存
        if (realLeftSize >= kCheapPrepend + len)
        {
            size_t dataSize = readableBytesLength();
            std::copy(begin() + headIndex_, begin() + tailIndex_, begin() + kCheapPrepend);
            headIndex_ = kCheapPrepend;
            tailIndex_ = headIndex_ + dataSize;
            assert(dataSize == readableBytesLength());
        }
        //空间不够 要resize
        else
        {
            /*
            方案1 手动扩容 空间不浪费 但性能较差
            size_t dataSize = readableBytesLength();
            size_t newSize = dataSize + len + kCheapPrepend;
            std::vector<char> newBuffer(newSize);
            std::copy(begin() + headerIndex_, begin() + tailerIndex_, newBuffer.data() + kCheapPrepend);
            buffer_.swap(newBuffer);
            headerIndex_ = kCheapPrepend;
            tailerIndex_ = kCheapPrepend + dataSize;
            */

            //方案2 直接扩容  有空间浪费（包含了之前的预留空间） 但性能更好
            buffer_.resize(tailIndex_ + len);
        }
    }

    std::vector<char> buffer_;
    size_t headIndex_;  //数据起始位置
    size_t tailIndex_;  //数据结束位置
};
}  // namespace net
}  // namespace myMuduo

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