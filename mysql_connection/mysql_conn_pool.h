#ifndef MYSQL_CONN_POOL_H
#define MYSQL_CONN_POOL_H

#include <list>
#include <mysql/mysql.h>
#include <pthread.h>
#include <string>
#include "../locker/mutex_lock_guard.h"
#include "../locker/condition.h"

/**
 * @brief 
 * 单例线程池，调用 MysqlConnPool::getInstance() 获取对象
 */
class MysqlConnPool
{
public:
    static MysqlConnPool *getInstance();

    void init(std::string url,
            std::string username,
            std::string password,
            std::string databaseName,
            unsigned int port,
            int maxConn);

    MYSQL *getConnection();             // 获取一个连接
    bool releaseConntion(MYSQL *conn);  // 释放连接
    void destroyPool();                 // 销毁所有连接

private:
    MysqlConnPool();
    ~MysqlConnPool();
    
    // 禁止拷贝构造
    MysqlConnPool(const MysqlConnPool &) = delete;
    // 禁止赋值构造
    MysqlConnPool &operator=(const MysqlConnPool &) = delete;

public:
    std::string url_;
    std::string username_;
    std::string password_;
    std::string databaseName_;
    unsigned int port_;

private:
    std::list<MYSQL *> connList_;
    int maxConn_;   // 最大连接数
    int curConn_;   // 当前已使用的连接数
    int freeConn_;  // 当前空闲的连接数

    // pthread_mutex_t mutex_;
    // pthread_cond_t cond_;
    MutexLock mutex_;
    Condition cond_;
};

#endif