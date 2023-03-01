#include <stdio.h>
#include "mysql_conn_pool.h"
#include "../err_handle.h"

MysqlConnPool::MysqlConnPool()
        : maxConn_(0)
        , curConn_(0)
        , freeConn_(0)
        , mutex_()
        , cond_(mutex_)
{}

MysqlConnPool::~MysqlConnPool()
{
    destroyPool();
}

MysqlConnPool *MysqlConnPool::getInstance()
{
    static MysqlConnPool connPool;
    return &connPool;
}

void MysqlConnPool::init(
        std::string url,
        std::string username,
        std::string password,
        std::string databaseName,
        unsigned int port,
        int maxConn)
{
    url_ = url;
    username_ = username;
    password_ = password;
    databaseName_ = databaseName;
    port_ = port;
    maxConn_ = maxConn;

    for (int i = 0; i < maxConn_; i++)
    {
        MYSQL *conn = NULL;

        // 初始化数据库
        if (mysql_init(conn) == NULL)
            err_handle("mysql_init failed");            
        
        // 连接数据库
        if (mysql_real_connect(conn, 
                url_.c_str(), username_.c_str(), 
                password_.c_str(), databaseName_.c_str(), 
                port_, NULL, 0) == NULL)
            err_handle("mysql_real_connect failed");

        connList_.push_back(conn);
        ++freeConn_;
    }
}


MYSQL *MysqlConnPool::getConnection()
{
    MYSQL *conn = NULL;

    if (0 == maxConn_)
        return conn;

    {   // 临界区
        MutexLockGuard lock(mutex_);
        while (freeConn_ == 0)
            cond_.wait();
        
        if (freeConn_ == 0) // destroy 时 notifyAll 会条件可能为真
            return NULL;

        conn = connList_.front();
        connList_.pop_front();

        --freeConn_;
        ++curConn_;
    }
    
    return conn;
}

bool MysqlConnPool::releaseConntion(MYSQL *conn)
{
    if (conn == NULL)
        return false;

    {
        MutexLockGuard lock(mutex_);
        connList_.push_back(conn);
        ++freeConn_;
        --curConn_;
    }
    cond_.notify();

    return true;
}

void MysqlConnPool::destroyPool()
{
    cond_.notifyAll();
    {   // 临界区
        MutexLockGuard lock(mutex_);
        if (connList_.size() > 0)
        {
            std::list<MYSQL *>::iterator it;
            for (it = connList_.begin(); it != connList_.end(); ++it)
            {
                mysql_close(*it);
            }
            maxConn_ = 0;
            curConn_ = 0;
            freeConn_ = 0;
            connList_.clear();
        }
    }
}