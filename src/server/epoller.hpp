#ifndef EPOLLER_HPP
#define EPOLLER_HPP
#include <vector>
#include <stdexcept>
#include <unistd.h>     // close
#include <sys/epoll.h>  // epoll_create, epoll_ctl, epoll_wait
namespace bre {

class Epoller {
public:
    explicit Epoller(int maxEvent = 1024) {
        epollFd = epoll_create(512);
        if(epollFd < 0) {
            throw std::runtime_error("epoll_create error");
        }
        events.resize(maxEvent);
    }

    ~Epoller() {
        close(epollFd);
    }

    bool AddFd(int fd, uint32_t events) {
        if (fd < 0) {
            return false;
        }
        struct epoll_event ev{};
        ev.data.fd = fd;
        ev.events = events;
        return 0 == epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev);
    }

    bool ModFd(int fd, uint32_t events) {
        if (fd < 0) {
            return false;
        }
        struct epoll_event ev{};
        ev.data.fd = fd;
        ev.events = events;
        return 0 == epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev);
    }

    bool DelFd(int fd) {
        if (fd < 0) {
            return false;
        }
        struct epoll_event ev{};
        return 0 == epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev);
    }

    int Wait(int timeoutMs = -1) {
        return epoll_wait(epollFd, events.data(), static_cast<int>(events.size()), timeoutMs);
    }

    int GetEventFd(int i) const {
        if (i < 0 || i >= (int)events.size()) {
            throw std::out_of_range("index out of range");
        }
        return events[i].data.fd;
    }

    uint32_t GetEvents(int i) const {
        if (i < 0 || i >= (int)events.size()) {
            throw std::out_of_range("index out of range");
        }
        return events[i].events;
    }
        
private:
    int epollFd;

    std::vector<struct epoll_event> events;    
};


} // namespace bre

#endif //EPOLLER_HPP