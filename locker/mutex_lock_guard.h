#ifndef MUTEX_LOCK_GUARD_H
#define MUTEX_LOCK_GUARD_H

#include "mutex_lock.h"

/**
 * @brief 
 * 使用时先创建MutexLock对象，在作用域内创建
 * MutexLockGuard对象，离开作用域自动销毁。
 */
class MutexLockGuard
{
public:
    explicit MutexLockGuard(MutexLock &mutex)
            : mutex_(mutex)
    {
        mutex_.lock();
    }

    ~MutexLockGuard()
    {
        mutex_.unlock();
    }

private:
    MutexLock &mutex_;
};

#endif