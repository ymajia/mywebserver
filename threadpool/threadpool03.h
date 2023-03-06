#ifndef THREADPOOL03_H
#define THREADPOOL03_H

#include <deque>
#include <pthread.h>
#include <functional>
#include <assert.h>
#include "../locker/mutex_lock_guard.h"
#include "../locker/condition.h"

class ThreadPool
{
public:
    typedef std::function<void()> Task;

public:
    ThreadPool(int threadNum = 8);
    ~ThreadPool();

    size_t addTask(const Task &task);
    void stop();
    int size();
    Task take();

private:
    int createThreads();
    // 线程函数，不断从线程池的任务队列里取出任务进行处理
    static void *threadFunc(void *arg);

    ThreadPool &operator=(const ThreadPool&);
    ThreadPool(const ThreadPool&);

private:
    volatile bool isRunning_;
    int threadsNum_;
    pthread_t *threads_;
    
    std::deque<Task> taskQueue_;
    // pthread_mutex_t mutex_;
    // pthread_cond_t condition_;
    MutexLock mutex_;
    Condition cond_;
};

#endif