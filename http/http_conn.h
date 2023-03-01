#ifndef HTTP_CONN_H
#define HTTP_CONN_H
#include "http_parser.h"
#include "http_response.h"
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <atomic>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <memory>
#include <sys/uio.h>

class HttpConn
{
public:
    HttpConn();
    ~HttpConn();

    void init(int sockfd, const sockaddr_in &addr);
    int getFd() {
        return sockfd_;
    }

    // 读取客户端数据
    int read();
    // 将HTTP响应报文写给客户端
    int write();
    /**
     * @brief 解析HTTP请求并生成HTTP响应报文
     * 
     * @return true 表示正确读取报文或发生错误，已经生成响应报文，需要注册EPOLLOUT事件，
     * @return false 表示没有读取到完整的报文，需要继续读取，需要注册EPOLLIN事件
     */
    bool process();

    void close();

public:
    const static int ReadBufferSize = 4096;
    const static int WriteBufferSize = 4096;
    const static char *const SourceDir;
    static std::atomic<int> userCount;

private:
    int sockfd_;
    struct sockaddr_in addr_;
    bool isClose_;

    std::unique_ptr<HttpParser> parser_;
    HttpParser::HttpCode parseResult_;
    // HttpParser::CheckState checkState_;
    
    std::unique_ptr<HttpResponse> response_;

    // 读缓冲区
    char readBuf_[ReadBufferSize];
    int readLen_;
    // 写缓冲区（响应头）
    char writeBuf_[WriteBufferSize];
    int writeLen_;
    struct iovec writeBufv_[2];

    const char *srcDir_;
};

#endif