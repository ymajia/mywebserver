#include "web_server.h"
#include "err_handle.h"
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <assert.h>
#include <string.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <signal.h>
#include <functional>

int setnonblocking(int fd);
Sigfunc* addsig(int sig, Sigfunc *handler, bool restart = true);


WebServer::WebServer(int port, int threadNum, int timeoutMs)
        : port_(port)
        , timeoutMs_(timeoutMs)
        , isClose_(false)
        , epoller_(new Epoller())
        , threadPool_(new ThreadPool(threadNum))
        , timer_(new HeapTimer())
{
    // 开始监听事件
    eventListen();
}

WebServer::~WebServer()
{
    for (auto it = userHttp_.begin(); it != userHttp_.end(); ++it)
    {
        closeConn(&it->second);
    }
    isClose_ = true;
    close(epollfd_);
    close(listenfd_);
}

void WebServer::eventListen()
{
    printf("==== start eventListen ====\n");
    fflush(stdout);

    // 设置sockaddr
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port_);

    listenfd_ = socket(PF_INET, SOCK_STREAM, 0);
    if (listenfd_ == -1)
    {
        err_handle("socket failed");
    }

    // 优雅关闭连接，直到所剩数据发送完毕或超时
    struct linger optLinger = {0};
    optLinger.l_onoff = 1;  // 启用优雅关闭
    optLinger.l_linger = 1;
    setsockopt(listenfd_, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    
    // 设置端口复用
    int optval = 1;         // 表示启用选项
    setsockopt(listenfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // bind
    if (-1 == bind(listenfd_, (struct sockaddr *)&address, sizeof(address)))
    {
        err_handle("bind failed");
    }
    // listen
    if (-1 == listen(listenfd_, 5))
    {
        err_handle("listen failed");
    }

    epoller_->addFd(listenfd_, EPOLLIN | EPOLLET | EPOLLRDHUP);
    setnonblocking(listenfd_);
}

void WebServer::eventLoop()
{
    if (!isClose_)
    {
        printf("====Server Start====\n");
        fflush(stdout);
    }

    int timeMs = -1;

    while (!isClose_)
    {
        if (timeoutMs_ > 0)
        {
            // 获取堆中下一次超时的时间
            timeMs = timer_->getNextTick();
        }
        printf("timeMs: %d\n", timeMs);
        fflush(stdout);
        int eventCnt = epoller_->wait(timeMs);
        for (int i = 0; i < eventCnt; i++)
        {
            // 处理事件
            // 获取发生事件的sockfd
            int fd = epoller_->getEventFd(i);
            // 获取发生的事件
            uint32_t events = epoller_->getEvents(i);
            if (fd == listenfd_)
            {
                printf("有新连接\n");
                fflush(stdout);

                dealListen();
            }
            else if (events & EPOLLRDHUP)
            {
                /* 处理错误 */
                printf("EPOLLRDHUP, sockfd: %d\n", fd);
                fflush(stdout);
                closeConn(&userHttp_[fd]);
            }
            else if (events & EPOLLHUP)
            {
                printf("EPOLLHUP, sockfd: %d\n", fd);
                fflush(stdout);
                closeConn(&userHttp_[fd]);
            }
            else if (events & EPOLLERR)
            {
                printf("EPOLLERR, sockfd: %d\n", fd);
                fflush(stdout);
                closeConn(&userHttp_[fd]);
            }
            else if (events & EPOLLIN)
            {
                printf("有读事件发生\n");
                fflush(stdout);

                // dealRead(fd);
                // 向线程池中添加读任务
                threadPool_->addTask(std::bind(&WebServer::onRead, this, fd));
            }
            else if (events & EPOLLOUT)
            {
                printf("有写事件发生\n");
                fflush(stdout);

                // dealWrite(fd);
                // 向线程池中添加写任务
                threadPool_->addTask(std::bind(&WebServer::onWrite, this, fd));
            }
        }
    }
}

void WebServer::dealListen()
{
    struct sockaddr_in cliAddr;
    socklen_t len = sizeof(cliAddr);
    int acceptFd = -1;
    while ((acceptFd = accept(listenfd_, (struct sockaddr *)&cliAddr, &len)) > 0)
    {
        if (HttpConn::userCount >= MaxFd)
        {
            printf("clients is full");
            fflush(stdout);
            return;
        }

        printf("accepted a new client, fd = %d\n", acceptFd);
        fflush(stdout);

        // Add Client
        // 在userHttp的map中加入acceptFd到cliAddr的映射
        userHttp_[acceptFd].init(acceptFd, cliAddr);
        // 若设置了超时事件，加入timer堆中
        if (timeoutMs_ > 0)
        {
            timer_->add(acceptFd, timeoutMs_, std::bind(&WebServer::closeConn, this, &userHttp_[acceptFd]));
        }
        // 监听acceptFd的读事件
        bool ret = epoller_->addFd(acceptFd, EPOLLIN | EPOLLONESHOT | EPOLLRDHUP | EPOLLET);
        if (ret == false)
        {
            printf("epoll addFd failed!\n");
            fflush(stdout);
            exit(1);
        }
        // 设置非阻塞
        setnonblocking(acceptFd);
    }
    // do
    // {
    //     int acceptFd = accept(listenfd_, (struct sockaddr *)&cliAddr, &len);
    //     if (acceptFd < 0)
    //     {
    //         // 没有新连接了
    //         return;
    //     }
    //     else if (HttpConn::userCount >= MaxFd)
    //     {
    //         printf("clients is full");
    //         fflush(stdout);
    //         return;
    //     }

    //     printf("accepted a new client, fd = %d\n", acceptFd);
    //     fflush(stdout);

    //     // Add Client
    //     // 在userHttp的map中加入acceptFd到cliAddr的映射
    //     userHttp_[acceptFd].init(acceptFd, cliAddr);
    //     // 若设置了超时事件，加入timer堆中
    //     if (timeoutMs_ > 0)
    //     {
    //         timer_->add(acceptFd, timeoutMs_, std::bind(&WebServer::closeConn, this, &userHttp_[acceptFd]));
    //     }
    //     // 监听acceptFd的读事件
    //     bool ret = epoller_->addFd(acceptFd, EPOLLIN | EPOLLONESHOT | EPOLLRDHUP | EPOLLET);
    //     if (ret == false)
    //     {
    //         printf("epoll addFd failed!\n");
    //         fflush(stdout);
    //         exit(1);
    //     }
    //     // 设置非阻塞
    //     setnonblocking(acceptFd);

    // } while (listenEvent_ & EPOLLET);
}

// void WebServer::dealRead(int sockfd)
// {
//     threadPool_->addTask(std::bind(&WebServer::onRead, this, sockfd));
// }

// void WebServer::dealWrite(int sockfd)
// {
//     threadPool_->addTask(std::bind(&WebServer::onWrite, this, sockfd));
// }

void WebServer::onRead(int sockfd)
{
    HttpConn *conn = &userHttp_[sockfd];

    // 读取客户端数据
    int readRet = conn->read();
    if (readRet == -1)
    {
        // 删除监听事件，关闭连接
        closeConn(conn);
        return;
    }
    else if (readRet == 0)   // 没有读取到数据
    {
        // 继续监听读事件
        epoller_->modFd(conn->getFd(), EPOLLIN | EPOLLONESHOT | EPOLLRDHUP | EPOLLET);
        return;
    }

    onProcess(conn);
}

void WebServer::onWrite(int sockfd)
{
    HttpConn *conn = &userHttp_[sockfd];

    int writeRet = conn->write();
    epoller_->modFd(conn->getFd(), EPOLLIN | EPOLLONESHOT | EPOLLRDHUP | EPOLLET);
}

void WebServer::onProcess(HttpConn *conn)
{
    // 处理读取的结果，即解析HTTP请求
    bool procRet = conn->process();

    /* 这里的实现不够直观，需要考虑一下process的返回值 */
    if (procRet)
    {
        epoller_->modFd(conn->getFd(), EPOLLOUT | EPOLLONESHOT | EPOLLRDHUP | EPOLLET);
    }
    else
    {
        epoller_->modFd(conn->getFd(), EPOLLIN | EPOLLONESHOT | EPOLLRDHUP | EPOLLET);
    }
}

void WebServer::closeConn(HttpConn *conn)
{
    epoller_->delFd(conn->getFd());
    conn->close();
}

Sigfunc* addsig(int sig, Sigfunc *handler, bool restart)
{
    struct sigaction act, oact;
    act.sa_handler = handler;
    sigfillset(&act.sa_mask);
    act.sa_flags = 0;
    if (restart)
    {
        act.sa_flags |= SA_RESTART;
    }
    else
    {
        act.sa_flags |= SA_INTERRUPT;
    }
    if (sigaction(sig, &act, &oact) < 0)
    {
        return SIG_ERR;
    }
    return oact.sa_handler;
}

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}