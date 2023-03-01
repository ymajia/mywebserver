#ifndef WEB_SERVER_H
#define WEB_SERVER_H
#include <memory>
#include <unordered_map>
#include "epoller.h"
#include "../threadpool/threadpool03.h"
#include "../http/http_conn.h"
#include "../timer/heapTimer.h"

typedef void Sigfunc(int);

class WebServer
{
public:
    static const int MaxEvents = 10000;

public: 
    WebServer(int port, int threadNum, int timeoutMs);
    ~WebServer();

    // void init(int port);
    void eventListen();
    void eventLoop();

private:
    // 处理新连接
    void dealListen();
    // 处理读事件
    void dealRead(int sockfd);
    // 处理写事件
    void dealWrite(int sockfd);
    // 线程函数
    void onRead(int sockfd);
    // 线程函数
    void onWrite(int sockfd);

    void onProcess(HttpConn *);

    void closeConn(HttpConn *);

private:
    static const int MaxFd = 65536;

    int port_;  // 端口号
    int listenfd_;
    int epollfd_;
    bool isClose_;
    int timeoutMs_;

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadPool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> userHttp_;    // sockfd=>HttpConn
};

#endif