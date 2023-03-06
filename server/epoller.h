#ifndef EPOLLER_H
#define EPOLLER_H
#include <sys/epoll.h>
#include <vector>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

class Epoller
{
public:
    explicit Epoller(int maxEvent = 1024);
    ~Epoller();

    bool addFd(int fd, uint32_t events);
    bool modFd(int fd, uint32_t events);
    bool delFd(int fd);

    int wait(int timeoutMs = -1);
    int getEventFd(size_t i) const;
    uint32_t getEvents(size_t i) const;

private:
    int epollFd_;
    std::vector<struct epoll_event> events_;
};


#endif