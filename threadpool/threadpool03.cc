#include "threadpool03.h"

ThreadPool::ThreadPool(int threadsNum)
        : mutex_()
        , cond_(mutex_)
        , isRunning_(true)
        , threadsNum_(threadsNum)
{
    createThreads();
}

ThreadPool::~ThreadPool()
{
    stop();
}

int ThreadPool::createThreads()
{
    // pthread_mutex_init(&mutex_, NULL);
    // pthread_cond_init(&condition_, NULL);
    threads_ = (pthread_t *)malloc(sizeof(pthread_t) * threadsNum_);
    for (int i = 0; i < threadsNum_; i++)
    {
        pthread_create(&threads_[i], NULL, threadFunc, this);
        // pthread_detach(threads_[i]);
    }

    return 0;
}

size_t ThreadPool::addTask(const Task &task)
{
    // pthread_mutex_lock(&mutex_);
    MutexLockGuard lock(mutex_);
    taskQueue_.push_back(task);
    size_t size = taskQueue_.size();
    // pthread_mutex_unlock(&mutex_);
    // pthread_cond_signal(&condition_);
    cond_.notify();
    return size;
}

void ThreadPool::stop()
{
    if (!isRunning_) return;

    isRunning_ = false;
    // pthread_cond_broadcast(&condition_);
    cond_.notifyAll();

    for (int i = 0; i < threadsNum_; i++)
    {
        pthread_join(threads_[i], NULL);
    }

    free(threads_);
    threads_ = NULL;

    // pthread_mutex_destroy(&mutex_);
    // pthread_cond_destroy(&condition_);
}

int ThreadPool::size()
{
    // pthread_mutex_lock(&mutex_);
    MutexLockGuard lock(mutex_);
    size_t size = taskQueue_.size();
    // pthread_mutex_unlock(&mutex_);
    return size;
}

ThreadPool::Task ThreadPool::take()
{
    Task task = NULL;
    // pthread_mutex_lock(&mutex_);
    MutexLockGuard lock(mutex_);
    while (taskQueue_.empty() && isRunning_)
    {
        // pthread_cond_wait(&condition_, &mutex_);
        cond_.wait();
    }

    if (!isRunning_)
    {
        // pthread_mutex_unlock(&mutex_);
        return task;
    }

    assert(!taskQueue_.empty());
    task = taskQueue_.front();
    taskQueue_.pop_front();
    // pthread_mutex_unlock(&mutex_);
    return task;
}

void *ThreadPool::threadFunc(void *arg)
{
    pthread_t tid = pthread_self();
    ThreadPool *pool = static_cast<ThreadPool *>(arg);
    while (pool->isRunning_)
    {
        // printf("want to take a task.\n");
        ThreadPool::Task task = pool->take();
        // printf("taked a task!\n");
        if (!task)
        {
            printf("thread %lu will exit\n", tid);
            break;
        }
        assert(task);
        task();
    }
    return pool;
}