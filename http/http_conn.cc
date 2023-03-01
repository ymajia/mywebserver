#include "http_conn.h"

std::atomic<int> HttpConn::userCount;
const char *const HttpConn::SourceDir = "/home/ma/webserver/mywebserver/resources";

HttpConn::HttpConn()
        : sockfd_(-1)
        , addr_({0})
        , isClose_(true)
        , parser_(new HttpParser(SourceDir))
        , response_(new HttpResponse())
        , readLen_(0)
        , writeLen_(0)
        , srcDir_(SourceDir)
{}

HttpConn::~HttpConn()
{
    close();
}

void HttpConn::init(int sockfd, const sockaddr_in &addr)
{
    assert(sockfd > 0);   
    userCount++;
    sockfd_ = sockfd;
    addr_ = addr;
    isClose_ = false;
    bzero(readBuf_, ReadBufferSize);
    bzero(writeBuf_, WriteBufferSize);
}

int HttpConn::read()
{
    memset(readBuf_, '\0', ReadBufferSize);
    ssize_t byteRead = 0;   // 一次读到的字节数
    readLen_ = 0;

    // 设置主状态机初始状态
    // HttpParser::CheckState checkState = HttpParser::CheckStateRequestline;
    // 循环读取客户数据并分析
    while (true)
    {
        byteRead = recv(sockfd_, readBuf_ + readLen_, ReadBufferSize - readLen_, 0);
        if (byteRead == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            return -1;
        }
        else if (byteRead == 0)
        {
            printf("remote client has closed the connection\n");
            break;
        }
        readLen_ += byteRead;
    }
    return readLen_;
}

int HttpConn::write()
{
    int writeByte = -1;

    while (true)
    {   
        // 向客户端发送数据
        writeByte = writev(sockfd_, writeBufv_, 2);
        
        if (writeByte <= 0)
        {
            break;
        }
        if (writeBufv_[0].iov_len + writeBufv_[1].iov_len == 0) // 传输结束
        {
            break;
        }
        else if (writeByte > writeBufv_[0].iov_len)
        {
            // 一次写的长度大于第一个iov的长度，说明第一个iov已经全部发送出去
            writeBufv_[0].iov_base = NULL;
            writeBufv_[0].iov_len = 0;

            writeBufv_[1].iov_base = (char *)writeBufv_[1].iov_base + (writeByte - writeLen_);
            writeBufv_[1].iov_len -= (writeByte - writeLen_);

            // writeBuffer缓冲区已经全部发送，将其置为0
            writeLen_ = 0;
        }
        else
        {
            // 一次写的长度小于第一个iov的长度，说明第一个iov还未全部发送出去
            writeBufv_[0].iov_base = (char *)writeBufv_[0].iov_base + writeByte;
            writeBufv_[0].iov_len -= writeByte;
        }
    }

    return writeByte;
}

bool HttpConn::process()
{
    parseResult_ = parser_->parse(readBuf_, readLen_);
    switch (parseResult_)
    {
    case HttpParser::NoRequest:
        /* 需要继续读取数据 */
        return false;
    
    case HttpParser::GetRequest:
        // 设置响应的属性
        response_->setStatus(200);
        response_->setPath(parser_->getPath());
        break;

    case HttpParser::BadRequest:
        response_->setStatus(400);
        response_->setPath("/400.html");
        break;

    // case HttpParser::NotFound:
    //     response_->setStatus(404);
    //     response_->setPath("/404.html");
    //     break;

    case HttpParser::InternalError:
        response_->setStatus(500);
        response_->setPath("/500.html");
        break;

    default:
        break;
    }

    response_->setSourceDir(srcDir_);
    response_->setKeepAlive(true);

    // 响应行和响应头
    writeLen_ = response_->makeResponse(writeBuf_, WriteBufferSize);
    writeBufv_[0].iov_base = writeBuf_;
    writeBufv_[0].iov_len = writeLen_;
    // 响应体
    writeBufv_[1].iov_base = response_->getMmfile();
    writeBufv_[1].iov_len = response_->getFileSize();

    return true;
}

void HttpConn::close()
{
    if (isClose_ == false)
    {
        isClose_ = true;
        ::close(sockfd_);
    }
}