#include "http_response.h"

//定义http响应的一些状态信息
std::unordered_map<int, const char *> StatusMap 
{
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {500, "Internal Error"}
};

std::unordered_map<int, const char *> FormMap 
{
    {400, "Your request has bad syntax or is inherently impossible to staisfy.\n"},
    {403, "You do not have permission to get file form this server.\n"},
    {404, "The requested file was not found on this server.\n"},
    {500, "There was an unusual problem serving the request file.\n"}
};

std::unordered_map<const char *, const char *, hash_func, cmp> TypeMap 
{
    {".html",   "text/html"},
    {".xml",    "text/xml"},
    {".xhtml",  "application/xhtml+xml"},
    {".txt",    "text/plain"},
    {".rtf",    "application/rtf"},
    {".pdf",    "application/pdf"},
    {".word",   "application/nsword"},
    {".png",    "image/png"},
    {".gif",    "image/gif"},
    {".jpg",    "image/jpeg"},
    {".jpeg",   "image/jpeg"},
    {".au",     "audio/basic"},
    {".mpeg",   "video/mpeg"},
    {".mpg",    "video/mpeg"},
    {".avi",    "video/x-msvideo"},
    {".gz",     "application/x-gzip"},
    {".tar",    "application/x-tar"},
    {".css",    "text/css"},
    {".js",     "text/javascript"},
    {".ico",    "image/x-icon"}
};

HttpResponse::HttpResponse()
        : status_(-1)
        , keepAlive_(false)
        , srcDir_("")
        , path_("")
        , fullPath_(NULL)
        , mmFile_(NULL)
        , fileStat_({0})
{

}

HttpResponse::~HttpResponse()
{
    // unmap()
    if (fullPath_ != NULL)
    {
        free(fullPath_);
        fullPath_ = NULL;
    }
    if (mmFile_)
    {
        munmap(mmFile_, fileStat_.st_size);
        mmFile_ = NULL;
    }
}

int HttpResponse::makeResponse(char *buffer, size_t size)
{
    if (mmFile_)
    {
        munmap(mmFile_, fileStat_.st_size);
        mmFile_ = NULL;
    }

    // 拼接目录和请求路径名
    if (strcmp(srcDir_, "") != 0 && strcmp(path_, "") != 0)
    {
        makeFullPath(srcDir_, path_);

        int statRet = stat(fullPath_, &fileStat_);
        if (statRet == -1)
        {
            if (errno == ENOENT)    // 没有该文件 
            {
                status_ = 404;
                makeFullPath(srcDir_, "/404.html");
                printf("fullPath: %s\n", fullPath_);
                statRet = stat(fullPath_, &fileStat_);
                // assert(statRet != -1);
                if (statRet == -1)
                {
                    printf("errno: %d\n", errno);
                    exit(1);
                }
            }
        }
        else if (S_ISDIR(fileStat_.st_mode))        // 路径是个目录
        {
            status_ = 404;
            makeFullPath(srcDir_, "/404.html");
            statRet = stat(fullPath_, &fileStat_);
            assert(statRet != -1);
        }
        else if (!(fileStat_.st_mode & S_IROTH))    // 不可读
        {
            status_ = 403;
            makeFullPath(srcDir_, "/403.html");
            statRet = stat(fullPath_, &fileStat_);
            assert(statRet != -1);
        }
    }
    else if (strcmp(srcDir_, "") == 0)
    {
        printf("srcDir is empty!");
        exit(1);
    }
    else if (strcmp(path_, "") == 0)
    {
        printf("Path is empty!");
        exit(1);
    }

    int writeByte = 0;
    // 处理响应行
    writeByte += addStatusLine(buffer, size - writeByte);
    // 处理响应头
    if (keepAlive_)
    {
        writeByte += addHeader(buffer + writeByte, size - writeByte, "Connection", "keep-alive");
    }
    else
    {
        writeByte += addHeader(buffer + writeByte, size - writeByte, "Connection", "close");
    }
    // writeByte += addHeader(buffer + writeByte, size - writeByte, "Content-type", "text/html");
    writeByte += addContentType(buffer + writeByte, size - writeByte);
    writeByte += addHeader(buffer + writeByte, size - writeByte, "Content-length", fileStat_.st_size);

    writeByte += addBlankLine(buffer + writeByte, size - writeByte);
    addContent(buffer + writeByte, size - writeByte);

    return writeByte;
}

int HttpResponse::addStatusLine(char *buffer, size_t size)
{
    int writeByte = snprintf(buffer, size, "%s %d %s\r\n", "HTTP/1.1", status_, StatusMap[status_]);
    assert(writeByte >= 0);
    return writeByte;
}

int HttpResponse::addContentType(char *buffer, size_t size)
{
    const char *type = strchr(fullPath_, '.');
    if (type != NULL && TypeMap.count(type) == 1)
    {
        return addHeader(buffer, size, "Content-type", TypeMap[type]);
    }
    return addHeader(buffer, size, "Content-type", "text/plain");
}

int HttpResponse::addHeader(char *buffer, size_t size, const char *key, const char *value)
{
    int writeByte = snprintf(buffer, size, "%s:%s\r\n", key, value);
    assert(writeByte >= 0);
    return writeByte;
}

int HttpResponse::addHeader(char *buffer, size_t size, const char *key, int value)
{
    int writeByte = snprintf(buffer, size, "%s:%d\r\n", key, value);
    assert(writeByte >= 0);
    return writeByte;
}

int HttpResponse::addBlankLine(char *buffer, size_t size)
{
    int writeByte = snprintf(buffer, size, "\r\n");
    assert(writeByte >= 0);
    return writeByte;
}

int HttpResponse::addContent(char *buffer, size_t size)
{
    /* 可以先判断fileStat_ */

    // 只读打开页面文件
    printf("fullPath: %s\n", fullPath_);
    int fd = open(fullPath_, O_RDONLY);
    // assert(fd != -1);   /* 可以修改为发送错误页面 */
    if (fd == -1)
    {
        // printf("errno: %d\n", errno);
        exit(1);
    }

    // 文件映射
    void *mmRet = mmap(0, fileStat_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    assert(mmRet != MAP_FAILED);
    mmFile_ = (char *)mmRet;

    return fileStat_.st_size;
}

void HttpResponse::makeFullPath(const char *srcDir, const char *path)
{
    if (fullPath_ != NULL)
    {
        free(fullPath_);
        fullPath_ = NULL;
    }

    fullPath_ = (char *)malloc(strlen(srcDir) + strlen(path));
    strcpy(fullPath_, srcDir);
    strcat(fullPath_, path);
}