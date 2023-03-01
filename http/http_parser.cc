#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include "../err_handle.h"
#include "http_parser.h"

HttpParser::HttpParser()
        : method_(NULL)
        , path_(NULL)
        , version_(NULL)
        // , srcDir_("")
        // , fullPath_(NULL)
        // , fileStat_({0})
{

}

HttpParser::HttpParser(const char *sourceDir)
        : method_(NULL)
        , path_(NULL)
        , version_(NULL)
        // , srcDir_(sourceDir)
        // , fullPath_(NULL)
        // , fileStat_({0})
{

}
// const char *HttpParser::szret[2] = 
// {
//     "I get a correct result\n",
//     "Something wrong\n"
// };

/* 从状态机，用于解析一行的内容 */
HttpParser::LineStatus HttpParser::parseLine(char *buffer, int &checkedIndex, int &bufferLen)
{
    char temp;
    /**
     * checkedIndex指向buffer中当前正在分析的字节，readIndex指向buffer中客户数据尾部的下一字节
     * buffer中0-checkIndex字节已经分析完毕，checkedIndex-(readIndex-1)字节由下面循环分析
     */
    for (; checkedIndex < bufferLen; ++checkedIndex)
    {
        // 获得当前要分析的字节
        temp = buffer[checkedIndex];
        if (temp == '\r')
        {
            // 若'\r'是当前读取客户数据的最后一个字符，表示没有分析到一个完整的行，需要继续读取数据
            if ((checkedIndex + 1) == bufferLen)
            {
                return LineOpen;
            }
            // 下一个字符是'\n'，成功读取到一行
            else if (buffer[checkedIndex + 1] == '\n')
            {
                buffer[checkedIndex++] = '\0';
                buffer[checkedIndex++] = '\0';
                return LineOk;
            }
            // 否则说明语法存在问题
            return LineBad;
        }
        else if (temp == '\n')
        {
            // 读取到'\n'则前一个字符必须是'\r'才合法
            if ((checkedIndex > 1) && buffer[checkedIndex - 1] == '\r')
            {
                buffer[checkedIndex - 1] = '\0';
                buffer[checkedIndex++] = '\0';
                return LineOk;
            }
            return LineBad;
        }
    }
    // 所有内容分析完毕且没有碰到'\r'字符，则返回LineOpen，继续读取客户数据
    return LineOpen;
}

/* 分析请求行 */
HttpParser::HttpCode HttpParser::parseRequestLine(char *temp, HttpParser::CheckState &checkState)
{
    char *path = strpbrk(temp, " \t");
    // 如果请求行中没有空格或'\t'字符，则HTTP请求一定有问题
    if (!path)
    {
        return BadRequest;
    }
    *path++ = '\0';
    // 获得请求类型
    method_ = temp;
    if ((strcasecmp(method_, "GET")
            && strcasecmp(method_, "POST")
            && strcasecmp(method_, "HEAD")
            && strcasecmp(method_, "OPTIONS")
            && strcasecmp(method_, "PUT")
            && strcasecmp(method_, "DELETE")
            && strcasecmp(method_, "TRACE")
            && strcasecmp(method_, "CONNECT")) != 0)
    {
        // 若请求类型不是以上8种，则存在语法错误
        printf("语法错误\n");
        fflush(stdout);
        return BadRequest;
    }
    
    // 跳过空格或'\t'字符
    path += strspn(path, " \t");
    char *version = strpbrk(path, " \t");
    if (!version) return BadRequest;
    *version++ = '\0';
    version += strspn(version, " \t");
    // 获得HTTP版本
    version_ = version;
    if ((strcasecmp(version_, "HTTP/1.0")
            && strcasecmp(version_, "HTTP/1.1")
            && strcasecmp(version_, "HTTP/2.0")
            && strcasecmp(version_, "HTTP/3.0")) != 0)
    {
        printf("语法错误\n");
        fflush(stdout);
        return BadRequest;
    }

    // 去除path前面的http或https域名，只保留域名后面的路径
    if (strncasecmp(path, "http://", 7) == 0)
    {
        path += 7;
        path = strchr(path, '/');
    }
    else if (strncasecmp(path, "https://", 8) == 0)
    {
        path += 8;
        path = strchr(path, '/');
    }
    if (!path || path[0] != '/')
    {
        return BadRequest;
    }
    path_ = path;

    // makeFullPath(srcDir_, path_);
    // if (stat(fullPath_, &fileStat_) == -1)
    // {
    //     /* 需要检查错误码 */
    //     return HttpCode::NotFound;
    // }
    // if (!(fileStat_.st_mode & S_IROTH)) // 不可读
    // {
    //     return HttpCode::ForbiddenRequest;
    // }
    // if (S_ISDIR(fileStat_.st_mode))     // 该路径是个目录
    // {
    //     return HttpCode::NotFound;
    // }

    // Http请求行处理完毕，状态转移到头部字段的分析
    checkState = CheckStateHeader;
    return NoRequest;
}

/* 分析头部字段 */
HttpParser::HttpCode HttpParser::parseHeaders(char *temp)
{
    // 遇到一个空行，说明我们得到了一个正确的Http请求
    if (temp[0] == '\0')
    {
        return GetRequest;
    }

    char *val = strpbrk(temp, ":");
    *val++ = '\0';
    val += strspn(val, " \t");
    header_[temp] = val;

    return NoRequest;
}

/* 分析Http请求的入口函数 */
HttpParser::HttpCode HttpParser::parse(char *buffer, int &bufferLen)
{
    int startLine = 0;                      // 记录一行在buffer中的起始位置
    int checkIndex = 0;                     // 记录当前已经分析了多少字节的客户数据
    LineStatus lineStatus = LineOk;         // 记录当前行的读取状态
    HttpCode retcode = NoRequest;           // 记录Http请求的处理结果
    HttpParser::CheckState checkState = HttpParser::CheckStateRequestline;  // 设置主状态机初始状态

    // 主状态机，用于从buffer中取出所有完整的行
    while ((lineStatus = parseLine(buffer, checkIndex, bufferLen)) == LineOk)
    {
        char *temp = buffer + startLine;    // startLine是行在buffer中的起始位置
        startLine = checkIndex;             // 记录下一行的起始位置
        // checkState记录主状态机当前状态
        switch (checkState)
        {
        case CheckStateRequestline:         // 分析请求行
            retcode = parseRequestLine(temp, checkState);
            if (retcode != NoRequest)
            {
                // 发生错误，返回错误状态
                return retcode;
            }
            break;
        case CheckStateHeader:              // 分析头部字段
            retcode = parseHeaders(temp);
            if (retcode != NoRequest)
            {
                // 发生错误或获取到一个完整的请求，返回请求状态
                return retcode;
            }
            break;
        default:
            return InternalError;
        }
    }

    // 若没有读取到一个完整的行，表示还需要继续读取客户数据才能进一步分析
    if (lineStatus == LineOpen)
    {
        return NoRequest;
    }
    else
    {
        return BadRequest;
    }
}

// void HttpParser::makeFullPath(const char *srcDir, const char *path)
// {
//     if (fullPath_ != NULL)
//     {
//         free(fullPath_);
//         fullPath_ = NULL;
//     }

//     fullPath_ = (char *)malloc(strlen(srcDir) + strlen(path));
//     strcpy(fullPath_, srcDir);
//     strcat(fullPath_, path);
// }

/*
int main(int argc, char *argv[])
{
    printf("test\n");
    fflush(stdout);
    if (argc < 2)
    {
        printf("usage: %s <port>\n", basename(argv[0]));
        return 1;
    }
    
    int port = atoi(argv[1]);
    
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    // inet_pton(AF_INET, INADDR_ANY, &address.sin_addr);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    if (listenfd < 0)
    {
        err_handle("socket failed");
    }
    if (bind(listenfd, (struct sockaddr *)&address, sizeof(address)) == -1)
    {
        err_handle("bind failed");
    }

    printf("start listen...\n");
    fflush(stdout);
    if (listen(listenfd, 5) == -1)
    {
        err_handle("listenfd failed");
    }

    struct sockaddr_in clientAddress;
    socklen_t clientAddrLen = sizeof(clientAddress);
    int fd = accept(listenfd, (struct sockaddr *)&clientAddress, &clientAddrLen);
    printf("accept...\n");
    fflush(stdout);
    if (fd == -1)
    {
        err_handle("accept failed\n");
    }
    else
    {
        char buffer[HttpParser::BufferSize];// 读缓冲区
        memset(buffer, '\0', HttpParser::BufferSize);
        ssize_t dataRead = 0;               // 一次读到的字节数
        int readIndex = 0;                  // 当前已经读取了多少字节的客户数据
        int checkedIndex = 0;               // 当前已经分析了多少字节的客户数据
        int startLine = 0;                  // 行在buffer中的起始位置

        HttpParser httpParser;

        // 设置主状态机初始状态
        HttpParser::CheckState checkState = HttpParser::CheckStateRequestline;
        // 循环读取客户数据并分析
        while (1)
        {
            dataRead = recv(fd, buffer + readIndex, HttpParser::BufferSize - readIndex, 0);
            if (dataRead == -1)
            {
                printf("reading failed\n");
                break;
            }
            else if (dataRead == 0)
            {
                printf("remote client has closed the connection\n");
                break;
            }
            readIndex += dataRead;

            // 分析目前已经获得的所有客户数据
            HttpParser::HttpCode result = httpParser.parseContent(buffer, 
                    checkedIndex, checkState, readIndex, startLine);
            if (result == HttpParser::NoRequest)
            {
                continue;
            }
            else if (result == HttpParser::GetRequest)
            {
                send(fd, HttpParser::szret[0], strlen(HttpParser::szret[0]), 0);
                printf("Http method:\t%s\n", httpParser.getMethod());
                printf("Http version:\t%s\n", httpParser.getVersion());
                printf("Http path:\t%s\n", httpParser.getPath());
                printf("User-Agent:\t%s\n", httpParser.getHeader("User-Agent"));
                printf("Connection:\t%s\n", httpParser.getHeader("Connection"));
                
                break;
            }
            else
            {
                send(fd, HttpParser::szret[1], strlen(HttpParser::szret[1]), 0);
                break;
            }
        }
        close(fd);
    }
    close(listenfd);
    return 0;
}

*/