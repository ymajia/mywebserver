#ifndef MUTEX_LOCK_H
#define MUTEX_LOCK_H

#include <pthread.h>
#include <assert.h>
#include <unistd.h>

class MutexLock
{
public:
    MutexLock(): holder_(0)
    {
        pthread_mutex_init(&mutex_, NULL);
    }

    ~MutexLock()
    {
        assert(holder_ == 0);   // 确保销毁时不是lock状态
        pthread_mutex_destroy(&mutex_);
    }

    void lock()
    {
        pthread_mutex_lock(&mutex_);
        holder_ = gettid();
    }

    void unlock()
    {
        holder_ = 0;
        pthread_mutex_unlock(&mutex_);
    }

    pthread_mutex_t *getPthreadMutex()  // 仅供Condition调用
    {
        return &mutex_;
    }

private:
    pthread_mutex_t mutex_;
    pid_t holder_;  // 持有锁的线程

};

#endif