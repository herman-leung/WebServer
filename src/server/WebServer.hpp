#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "epoller.hpp"
#include "../mylog/Log.hpp"
#include "../config/Config.hpp"
#include "../timer/HeapTimer.hpp"
#include "../pool/ThreadPool.hpp"
#include "../http/HttpConn.hpp"

#include <unordered_map>
#include <string>
#include <cstring>
#include <memory>
#include <functional>
#include <fstream>
#include <iostream>
#include <filesystem>

#include <sys/socket.h> // socket, bind, listen, accept
#include <netinet/in.h> // sockaddr_in
#include <arpa/inet.h>  // inet_pton

namespace bre
{

    using std::function;
    using std::ifstream;
    using std::shared_ptr;
    using std::string;
    using std::unique_ptr;
    using std::unordered_map;

    class WebServer
    {
    public:
        WebServer(LogLevel logLevel = LogLevel::DEBUG)
            : isClose(false), epoller(new Epoller()),
              timer(new HeapTimer()), threadpool(new ThreadPool())
        {
            // 初始化
            auto &conf = Config::getInstance();
            port = stoi(conf.Get("PORT").value_or("5678"));
            openLinger = conf.Get("false").value_or("false") == "false" ? false : true;
            timeoutMS = stoi(conf.Get("TIMEOUT").value_or("5000"));
            // 获取资源路径
            srcDir = std::filesystem::current_path().string() + conf.Get("PATH").value_or("/resources");
            // 初始化 HttpConn
            HttpConn::UserCount = 0;
            HttpConn::SrcDir = srcDir.c_str();

            // 初始化数据库
            MySqlPool::Instance();

            // 设置事件模式
            initEventMode(stoi(conf.Get("TRIGMODE").value_or("3")));

            // 初始化socket
            if (!initSocket())
            {
                isClose = true;
            }
            if (isClose)
            {
                throw std::runtime_error("init socket error");
            }

            // 初始化日志
            int logQueSize = stoi(conf.Get("LOGSIZE").value_or("1024"));
            Log::Instance().Init(logLevel, true, "./log", ".log", logQueSize);
            {
                Log::info("WebServer init");
                Log::info("port: {}, openLinger: {}, timeoutMS: {}, \nsrcDir: {}",
                          port, openLinger, timeoutMS, srcDir);
                Log::info("TRIGMode: {}", conf.Get("TRIGMODE").value_or("3"));

                Log::info("srcDir: {}", srcDir);
                Log::info("log level: {}", (int)logLevel);
                Log::info("LogQueSize: {}", logQueSize);
                Log::info("SqlConnPool num: {}, ThreadPool num: {}", conf.Get("SQLNUM").value_or("8"), conf.Get("THREADNUM").value_or("8"));
                Log::info("=====================");
                Log::Instance().Flush();
            }
        }

        ~WebServer()
        {
            close(listenFd);
            isClose = true;
            Log::info("WebServer closed");
        }

        void Start()
        {
            int timeMs = -1;
            if (!isClose)
            {
                Log::info("WebServer start");
            }
            while (!isClose)
            {
                if (timeoutMS > 0)
                {
                    timeMs = timer->GetNextTick().count();
                }
                int eventCnt = epoller->Wait(timeMs);
                for (int i = 0; i < eventCnt; ++i)
                {
                    int fd = epoller->GetEventFd(i);
                    uint32_t events = epoller->GetEvents(i);
                    if (fd == listenFd)
                    {
                        dealListen();
                    }
                    else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
                    {
                        assert(users.count(fd) > 0);
                        closeConn(&users[fd]);
                    }
                    else if (events & EPOLLIN)
                    {
                        assert(users.count(fd) > 0);
                        dealRead(&users[fd]);
                    }
                    else if (events & EPOLLOUT)
                    {
                        assert(users.count(fd) > 0);
                        dealWrite(&users[fd]);
                    }
                    else
                    {
                        Log::err("Unexpected event");
                    }
                }
            }
        }

        void Stop()
        {
            isClose = true;
        }

    private:
        bool initSocket()
        {
            sockaddr_in addr;
            if (port < 1024 || port > 65535)
            {
                port = 5678;
            }
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
            linger optLinger = {};
            if (openLinger)
            {
                optLinger.l_onoff = 1;  // 直到数据发送完毕或超时
                optLinger.l_linger = 3; // 超时时间
            }

            listenFd = socket(AF_INET, SOCK_STREAM, 0);

            if (listenFd < 0)
            {
                Log::err("Create socket error");
                return false;
            }
            int ret = setsockopt(listenFd, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
            if (ret < 0)
            {
                close(listenFd);
                Log::err("Init linger error");
                return false;
            }
            int opt = 1;

            ret = setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            if (ret < 0)
            {
                close(listenFd);
                Log::err("Set reuseAddr error");
                return false;
            }

            ret = bind(listenFd, (sockaddr *)&addr, sizeof(addr));
            if (ret < 0)
            {
                close(listenFd);
                Log::err("Bind port: {} error: {}", port, strerror(errno));
                return false;
            }

            ret = listen(listenFd, 6);
            if (ret < 0)
            {
                close(listenFd);
                Log::err("Listen port: {} error", port);
                return false;
            }

            ret = epoller->AddFd(listenFd, EPOLLIN | listenEvent);
            if (ret == 0)
            {
                close(listenFd);
                Log::err("Add listen error");
                return false;
            }

            setFdNonblock(listenFd);
            Log::info("Server port: {}", port);

            return true;
        }

        void initEventMode(int trigMode)
        {
            listenEvent = EPOLLRDHUP;              // 对端关闭连接
            connEvent = EPOLLONESHOT | EPOLLRDHUP; // EPOLLRDHUP: 对端关闭连接
            switch (trigMode)
            {
            case 0:
                break;
            case 1:
                connEvent |= EPOLLET;
                break;
            case 2:
                listenEvent |= EPOLLET;
                break;
            case 3:
                listenEvent |= EPOLLET;
                connEvent |= EPOLLET;
                break;
            default:
                listenEvent |= EPOLLET;
                connEvent |= EPOLLET;
                break;
            }
            HttpConn::IsET = (connEvent & EPOLLET);
        }

        void addClient(int fd, sockaddr_in addr)
        {
            if (fd < 0)
            {
                Log::err("fd error");
                return;
            }
            users[fd].Init(fd, addr);
            if (timeoutMS > 0)
            {
                timer->Add(fd, timeoutMS,
                           std::bind(&WebServer::closeConn, this, &users[fd]));
            }
            epoller->AddFd(fd, connEvent | EPOLLIN);
            setFdNonblock(fd);
            Log::info("Client[{}]({}:{}) in, fd: {}",
                      HttpConn::UserCount, inet_ntoa(addr.sin_addr),
                      ntohs(addr.sin_port), fd);
        }

        void dealListen()
        {
            sockaddr_in addr;
            socklen_t len = sizeof(addr);
            do
            {
                int fd = accept(listenFd, (sockaddr *)&addr, &len);
                if (fd <= 0)
                { // 连接失败
                    return;
                }
                else if (HttpConn::UserCount >= MAX_FD)
                {
                    sendError(fd, "Server busy");
                    Log::warn("Clients is full");
                    return;
                }
                addClient(fd, addr);
            } while (listenEvent & EPOLLET);
        }

        void dealWrite(HttpConn *client)
        {
            if (client == nullptr)
            {
                Log::err("client is nullptr");
                throw std::invalid_argument("client is nullptr");
            }
            extentTime(client);
            threadpool->enqueue(std::bind(&WebServer::onWrite, this, client));
        }

        void dealRead(HttpConn *client)
        {
            if (client == nullptr)
            {
                Log::err("client is nullptr");
                throw std::invalid_argument("client is nullptr");
            }
            extentTime(client);
            threadpool->enqueue(std::bind(&WebServer::onRead, this, client));
        }

        void sendError(int fd, const char *info)
        {
            int ret = send(fd, info, strlen(info), 0);
            if (ret < 0)
            {
                Log::err("send error to client error");
            }
            close(fd);
        }

        void extentTime(HttpConn *client)
        {
            if (client == nullptr)
            {
                Log::err("client is nullptr");
                throw std::invalid_argument("client is nullptr");
            }
            if (timeoutMS > 0)
            {
                timer->Adjust(client->GetFd(), (MS)timeoutMS);
            }
        }

        void closeConn(HttpConn *client)
        {
            if (client == nullptr)
            {
                Log::err("client is nullptr");
                throw std::invalid_argument("client is nullptr");
            }
            // Log::info("Client close fd: {}", client->GetFd());
            epoller->DelFd(client->GetFd());
            client->Close();
        }

        void onRead(HttpConn *client)
        {
            if (client == nullptr)
            {
                Log::err("client is nullptr");
                throw std::invalid_argument("client is nullptr");
            }
            int ret = -1;
            int readErrno = 0;
            ret = client->Read(&readErrno);
            if (ret <= 0 && readErrno != EAGAIN)
            {
                closeConn(client);
                return;
            }
            onProcess(client);
        }

        void onWrite(HttpConn *client)
        {
            if (client == nullptr)
            {
                Log::err("client is nullptr");
                throw std::invalid_argument("client is nullptr");
            }
            int ret = -1;
            int writeErrno = 0;
            ret = client->Write(&writeErrno);
            if (client->ToWriteBytes() == 0)
            {
                if (client->IsKeepAlive())
                {
                    onProcess(client);
                    return;
                }
            }
            else if (ret < 0)
            {
                if (writeErrno == EAGAIN)
                {
                    epoller->ModFd(client->GetFd(), connEvent | EPOLLOUT);
                    return;
                }
                else
                {
                    Log::err("send data error");
                }
            }
        }

        void onProcess(HttpConn *client)
        {
            if (client == nullptr)
            {
                Log::err("client is nullptr");
                throw std::invalid_argument("client is nullptr");
            }
            if (client->Process())
            {
                epoller->ModFd(client->GetFd(), connEvent | EPOLLOUT);
            }
            else
            {
                epoller->ModFd(client->GetFd(), connEvent | EPOLLIN);
            }
        }

        static int setFdNonblock(int fd)
        {
            if (fd < 0)
            {
                Log::err("fd error");
                throw std::invalid_argument("fd error in setFdNonblock: " + std::to_string(fd));
            }
            int flag = fcntl(fd, F_GETFL);
            flag |= O_NONBLOCK;
            return fcntl(fd, F_SETFL, flag);
        }

    private:
        static const int MAX_FD = 65536;
        int port = 5678;
        bool openLinger = false;
        int timeoutMS = 10000; /* 毫秒MS */
        bool isClose;
        int listenFd;
        std::string srcDir;

        uint32_t listenEvent;
        uint32_t connEvent;

        std::unique_ptr<Epoller> epoller = nullptr;
        std::unique_ptr<HeapTimer> timer = nullptr;
        std::unique_ptr<ThreadPool> threadpool = nullptr;

        std::unordered_map<int, HttpConn> users;
    };

} // namespace bre

#endif // WEBSERVER_HPP