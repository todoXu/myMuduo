#include "myMuduo/net/Buffer.h"
#include <sys/uio.h>
#include <algorithm>
#include <cassert>

namespace myMuduo {
namespace net {

const char Buffer::kCRLF[] = "\r\n";

Buffer::Buffer(int initialSize)
    : buffer_(kCheapPrepend + initialSize)
    , headIndex_(kCheapPrepend)
    , tailIndex_(kCheapPrepend) {};

Buffer::~Buffer() = default;

size_t Buffer::readableBytesLength() const { return tailIndex_ - headIndex_; }

size_t Buffer::writableBytesLength() const { return buffer_.size() - tailIndex_; }

size_t Buffer::prependableBytesLength() const { return headIndex_; }

//返回数据的起始位置
const char* Buffer::peek() { return begin() + headIndex_; }

//标记缓冲区头部的一部分数据为“已消耗”，使其不再可读
//这通常在数据被应用程序完整处理后调用。
void Buffer::retrieve(size_t len)
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

void Buffer::retrieveAll()
{
    headIndex_ = kCheapPrepend;
    tailIndex_ = kCheapPrepend;
}

std::string Buffer::retrieveAllAsString() { return retrieveAsString(readableBytesLength()); }

std::string Buffer::retrieveAsString(size_t len)
{
    assert(len <= readableBytesLength());
    std::string result(peek(), len);
    retrieve(len);
    return result;
}

void Buffer::ensureWritableBytes(size_t len)
{
    if (writableBytesLength() < len)
    {
        //如果可写空间不足，扩展缓冲区
        makeSpace(len);
    }
    assert(writableBytesLength() >= len);
}

void Buffer::hasWritten(size_t len)
{
    assert(len <= writableBytesLength());
    tailIndex_ += len;
}

void Buffer::append(const char* data, size_t len)
{
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    hasWritten(len);
}

void Buffer::prepend(const char* data, size_t len)
{
    assert(len <= prependableBytesLength());
    headIndex_ -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d + len, begin() + headIndex_);
}

const char* Buffer::findCRLF(const char* start)
{
    assert(start >= peek());
    assert(start <= beginWrite());
    const void* crlf = std::search(start, static_cast<const char*>(beginWrite()), kCRLF, kCRLF + 2);
    return crlf == beginWrite() ? nullptr : static_cast<const char*>(crlf);
}

const char* Buffer::findCRLF()
{
    const char* start = peek();
    return findCRLF(start);
}

void Buffer::swap(Buffer& rhs)
{
    buffer_.swap(rhs.buffer_);
    std::swap(headIndex_, rhs.headIndex_);
    std::swap(tailIndex_, rhs.tailIndex_);
}

//从文件描述符中读取数据
//epoll工作在LT模式 就算缓冲区不够 下次epoll还是会通知
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
    char extraBuffer[65536];  // 64KB的额外缓冲区
    struct iovec iovec[2];
    size_t writableBytesSize = writableBytesLength();
    iovec[0].iov_base = const_cast<char*>(beginWrite());
    iovec[0].iov_len = writableBytesSize;
    iovec[1].iov_base = extraBuffer;         // 额外缓冲区
    iovec[1].iov_len = sizeof(extraBuffer);  // 额外缓冲区长度

    //buffer可能扩容过 可写区域很大就不用额外缓冲区了
    const int iovcnt = (writableBytesSize < sizeof(extraBuffer)) ? 2 : 1;
    const ssize_t n = readv(fd, iovec, iovcnt);
    if (n < 0)
    {
        *savedErrno = errno;
    }
    else if (static_cast<size_t>(n) <= writableBytesSize)
    {
        hasWritten(n);
    }
    //要把额外缓冲区的数据也添加到当前buffer中
    else
    {
        size_t extraLen = n - writableBytesSize;
        hasWritten(writableBytesSize);
        append(extraBuffer, extraLen);
    }
    return n;
}

char* Buffer::begin() { return buffer_.data(); }
char* Buffer::beginWrite() { return begin() + tailIndex_; }
void Buffer::makeSpace(size_t len)
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

}  // namespace net
}  // namespace myMuduo
