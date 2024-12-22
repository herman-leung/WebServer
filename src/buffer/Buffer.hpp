#ifndef buffer_HPP
#define buffer_HPP

#include <iostream>
#include <string>
#include <unistd.h>		// write
#include <sys/uio.h>	// readv
#include <vector>
#include <string>
#include <regex>

namespace bre {


class Buffer {
public:
    explicit Buffer(int initBuffSize = 1024)
        : buffer(initBuffSize), readPos(0), writePos(0) 
    { }

    ~Buffer() = default;

    Buffer(const Buffer&) = delete;              // 禁止拷贝构造
    Buffer& operator=(const Buffer&) = delete;   // 禁止拷贝赋值

    // 移动构造
    Buffer(Buffer&& other) noexcept
        : buffer(std::move(other.buffer)),
        readPos(other.readPos),
        writePos(other.writePos) {
        other.readPos = 0;
        other.writePos = 0;
    }

    // 移动赋值
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            buffer = std::move(other.buffer);
            readPos = other.readPos;
            writePos = other.writePos;
            other.readPos = 0;
            other.writePos = 0;
        }
        return *this;
    }

    // 可写字节数
    size_t WritableBytes() const {
        return buffer.size() - writePos;
    }
    // 可读字节数
    size_t ReadableBytes() const {
        return writePos - readPos;
    }

    const char* Peek() const {
        return buffer.data() + readPos;
    }

    std::string Retrieve(size_t len) {
        if (len > ReadableBytes()) {
            throw std::out_of_range("Buffer::Retrieve: len is too large");
        }
        auto ret = std::string(Peek(), len);
        readPos += len;
        return ret;
    }

    std::string RetrieveUntil(const std::string end) {
        if (ReadableBytes() < end.size()) {
            return "";
        }

        const std::size_t pos = std::string(buffer.data() + readPos, buffer.data() + writePos).find(end);
        if(pos == std::string::npos) {
            // 如果是最后一行
            std::size_t isLastLine = std::string(buffer.data() + readPos, buffer.data() + writePos).find("\n");
            if (isLastLine == std::string::npos) {
                return Retrieve(ReadableBytes());
            } else {
                std::cout << "RetrieveUntil: not found" << std::endl;
                return "";
            }
        }
        return Retrieve(pos + end.size());
    }


    void Clear() {
        readPos = writePos = 0;
        for (auto& item : buffer) {
            item = 0;
        }
    }

    // 获取了所有字符
    std::string RetrieveAll() {
        std::string ret = std::string(Peek(), ReadableBytes());
        Clear();
        return ret;
    }

    std::string ToString() const {
        return std::string(Peek(), ReadableBytes());
    }

    void Append(const std::string& str) {
        Append(str.data(), str.size());
    }
    void Append(const void* data, size_t len) {
        Append(static_cast<const char*>(data), len);
    }
    void Append(const Buffer& buff) {
        Append(buff.Peek(), buff.ReadableBytes());
    }
    // 最终添加数据
    void Append(const char* str, size_t len) {
        if (WritableBytes() < len) {
            expandBuffer(len);
        }
        std::copy(str, str + len, buffer.data() + writePos);
        writePos += len;
    }
#ifdef __linux__
    ssize_t ReadFd(int fd, int* Errno) {
        char buff[65535];
        struct iovec iov[2];
        const size_t writable = WritableBytes();
        /* 分散读， 保证数据全部读完 */
        iov[0].iov_base = buffer.data() + writePos;
        iov[0].iov_len = writable;
        iov[1].iov_base = buff;
        iov[1].iov_len = sizeof(buff);

        const ssize_t len = readv(fd, iov, 2);

        if(len < 0) {
            *Errno = errno;
            if(*Errno == EAGAIN) {
                return len;
            }
            std::cout << "readv error: " << *Errno << std::endl;
        }
        else if(static_cast<size_t>(len) <= writable) {
            writePos += len;

        }
        else {
            writePos = buffer.size();
            Append(buff, len - writable);
            
        }
        return len;
    }

    ssize_t WriteFd(int fd, int* Errno) {
        size_t readSize = ReadableBytes();
        ssize_t len = write(fd, Peek(), readSize);
        if(len < 0) {
            *Errno = errno;
            return len;
        } 
        readPos += len;
        return len;
    }
#endif // __linux__

private:
    void expandBuffer(size_t len) {
        if (WritableBytes() + readPos < len) {
            // 重置缓冲区
            buffer.resize(writePos + len);
        } else {
            // 移动数据
            size_t readable = ReadableBytes();
            std::copy(buffer.data() + readPos, buffer.data() + writePos, buffer.data());
            readPos = 0;
            writePos = readPos + readable;
        }
    }
    
private:
    std::vector<char> buffer;
    size_t readPos;     // 读取偏移量
    size_t writePos;    // 写入偏移量
};


} // namespace bre
#endif // buffer_HPP
