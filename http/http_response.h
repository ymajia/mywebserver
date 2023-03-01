#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string.h>
#include <unordered_map>
#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "map_utils.h"

class HttpResponse
{
public:
    HttpResponse();
    ~HttpResponse();

    // 生成响应行和响应头
    int makeResponse(char *buffer, size_t size);

    void setStatus(int status) { status_ = status; }
    void setKeepAlive(bool keepAlive) { keepAlive_ = keepAlive; }
    void setSourceDir(const char *src) { srcDir_ = src; }
    void setPath(const char *path) { path_ = path; }
    // void setFullPath(const char *path);

    char *getMmfile() { return mmFile_; }
    int getFileSize() { return fileStat_.st_size; }

private:
    // 设置请求行
    int addStatusLine(char *buffer, size_t size);
    // 设置头部字段
    int addHeader(char *buffer, size_t size, const char *key, const char *value);
    // 设置头部字段
    int addHeader(char *buffer, size_t size, const char *key, int value);
    // 添加内容类型
    int addContentType(char *buffer, size_t size);

    // 添加空行
    int addBlankLine(char *buffer, size_t size);

    int addContent(char *buffer, size_t size);
    void makeFullPath(const char *srcDir, const char *path);
private:
    int status_;
    bool keepAlive_;

    const char *srcDir_;
    const char *path_;
    char *fullPath_;
    char *mmFile_;
    struct stat fileStat_;
};

#endif