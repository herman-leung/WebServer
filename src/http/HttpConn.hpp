#ifndef HTTP_CONN_HPP
#define HTTP_CONN_HPP

#include "../buffer/Buffer.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"


#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>


namespace bre
{
    
class HttpConn {
public:
    HttpConn() {
        fd = -1;
        addr = {};
        isClose = true;
    }

    ~HttpConn() {
        Close();
    }

    void Init(int sockFd, const sockaddr_in& addr) {
        if (sockFd < 0) {
            throw std::invalid_argument("sockFd < 0");
        }
        UserCount++;
        fd = sockFd;
        this->addr = addr;
        isClose = false;
        readBuff.Clear();
        writeBuff.Clear();
        //Log::info("Client[{}]({}:{}) in, fd:{}", UserCount, GetIP(), GetPort(), fd);
    }

    ssize_t Read(int* saveErrno) {
        ssize_t len = -1;
        do {
            len = readBuff.ReadFd(fd, saveErrno);
            if (len <= 0) {
                break;
            }
        } while (IsET);
        std::cout << readBuff.ToString() << std::endl;
        return len;
    }

    ssize_t Write(int* saveErrno) {
        ssize_t len = -1;
        do {
            len = writev(fd, iov, iovCnt);
            if (len <= 0) {     // 出错
                *saveErrno = errno;
                break;
            }
            if (iov[0].iov_len + iov[1].iov_len == 0) { // 数据已经发送完毕
                break;
            } else if (static_cast<size_t>(len) <= iov[0].iov_len) {    // 只发送了部分数据
                iov[0].iov_base = static_cast<char*>(iov[0].iov_base) + len;
                iov[0].iov_len -= len;
                writeBuff.Retrieve(len);
            } else {    // 发送了全部数据
                size_t tlen = len - iov[0].iov_len;
                iov[1].iov_base = static_cast<char*>(iov[1].iov_base) + tlen;
                iov[1].iov_len -= tlen;
                if (iov[0].iov_len) {
                    writeBuff.RetrieveAll();
                    iov[0].iov_len = 0;
                }
            }
        } while (IsET || ToWriteBytes() > 10240);
        return len;
    }

    void Close() {
        response.UnmapFile();
        if (isClose == false) {
            UserCount--;
            isClose = true;
            close(fd);
            //Log::info("Client[%d](%s:%d) quit, fd:%d", UserCount, GetIP(), GetPort(), fd);
        }
    }

    int GetFd() const {
        return fd;
    }

    int GetPort() const {
        return addr.sin_port;
    }

    const char* GetIP() const {
        return inet_ntoa(addr.sin_addr);
    }
    
    sockaddr_in GetAddr() const {
        return addr;
    }
    
    bool Process() {
        request.Init();
        if (readBuff.ReadableBytes() <= 0) {
            return false;
        }
        bool praseCorrect = request.Parse(readBuff);
        string path =  request.Path();
        if (praseCorrect) {
            Log::info("{}", path);
            response.Init(SrcDir, path, request.IsKeepAlive(), 200);
        } else {
            response.Init(SrcDir, path, false, 400);
        }
        response.MakeResponse(writeBuff);
        // 响应头部
        iov[0].iov_base = const_cast<char*>(writeBuff.Peek());
        iov[0].iov_len = writeBuff.ReadableBytes();
        iovCnt = 1;

        // 如果是文件请求 则iov[1]指向文件
        // 文件请求
        if (response.FileLen() > 0 && response.File()) {
            iov[1].iov_base = response.File();
            iov[1].iov_len = response.FileLen();
            iovCnt = 2;
        }
        return true;
    }

    int ToWriteBytes() { 
        return iov[0].iov_len + iov[1].iov_len; 
    }

    bool IsKeepAlive() const {
        return request.IsKeepAlive();
    }

    static bool IsET;
    static const char* SrcDir;
    static int UserCount;
    
private:
    int fd;
    struct  sockaddr_in addr;

    bool isClose;
    
    int iovCnt;             // iov计数, 1表示只有iov[0], 2表示iov[0]和iov[1]
    struct iovec iov[2];    // iov[0]指向writeBuff, iov[1]指向文件
    
    Buffer readBuff; // 读缓冲区
    Buffer writeBuff; // 写缓冲区

    HttpRequest request;
    HttpResponse response;
};

bool HttpConn::IsET = false;
int HttpConn::UserCount  = 0;
const char* HttpConn::SrcDir = nullptr;

} // namespace bre





#endif // HTTP_CONN_HPP