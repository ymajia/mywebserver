#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <unordered_map>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include "map_utils.h"

class HttpParser
{
public:
    // static const int BufferSize = 4096;
    // 主状态机
    enum CheckState
    {
        CheckStateRequestline = 0,  // 正在分析请求行
        CheckStateHeader            // 正在分析头部字段
    };
    // 从状态机，行的读取状态
    enum LineStatus
    {
        LineOk = 0, // 读取到一个完整的行
        LineBad,    // 行出错
        LineOpen    // 行数据尚不完整
    };
    // 服务器处理HTTP请求的结果
    enum HttpCode
    {
        NoRequest,          // 请求不完整，需要继续读取客户数据
        GetRequest,         // 获得一个完整的客户请求
        BadRequest,         // 客户请求有语法错误
        ForbiddenRequest,   // 客户对资源没有足够的访问权限
        // NotFound,           // 找不到页面
        InternalError,      // 服务器内部错误
        ClosedConnection    // 客户端已经关闭连接
    };

    // 应答
    static const char *szret[2];

public:
    HttpParser();
    HttpParser(const char *sourceDir);

    /* 从状态机，用于解析一行的内容 */
    LineStatus parseLine(char *buffer, int &checkedIndex, int &buffeLen);
    /* 分析请求行 */
    HttpCode parseRequestLine(char *temp, CheckState &checkState);
    /* 分析头部字段 */
    HttpCode parseHeaders(char *temp);
    /* 分析Http请求的入口函数 */
    HttpCode parse(char *buffer, int &bufferLen);

    const char *getMethod() { return method_; }
    const char *getPath() { return path_; }
    const char *getVersion() { return version_; }

    const char *getHeader(const char *key) { return header_[key]; }

private:
    void makeFullPath(const char *srcDir, const char *path);

private:
    // CheckState checkState_;
    // int checkedIndex_;
    // int readIndex_;

    const char *method_;
    const char *path_;
    const char *version_;
    std::unordered_map<const char *, const char *, hash_func, cmp> header_;
    
    // const char *srcDir_;
    // char *fullPath_;
    // struct stat fileStat_;
};

#endif