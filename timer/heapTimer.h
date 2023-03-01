#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h>
#include <functional>
#include <assert.h>
#include <chrono>

typedef std::function<void()> TimeoutCallback;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds Ms;
typedef Clock::time_point TimeStamp;

struct TimerNode
{
    int id;
    TimeStamp expires;
    TimeoutCallback cb;
    bool operator<(const TimerNode &t)
    {
        return expires < t.expires;
    }
    bool operator>(const TimerNode &t)
    {
        return expires > t.expires;
    }
};

class HeapTimer
{
public:
    HeapTimer()
    {
        // 调整容量为64
        heap_.reserve(64);
    }
    ~HeapTimer() { clear(); }
    
    // 添加新节点，堆尾插入，调整堆
    void add(int id, int timeout, const TimeoutCallback& cb);
    // 调整指定id的节点
    void adjust(int id, int timeout);
    // 删除指定节点，并触发回调函数
    void doWork(int id);
    // 清空堆
    void clear();
    // 清除超时节点
    void tick();
    // 弹出堆顶节点
    void pop();
    // 获取下次超时时间
    int getNextTick();

private:
    // 上移节点
    void siftup(size_t i);
    // 下移节点
    bool siftdown(size_t i, size_t n);
    // 删除指定位置的节点
    void del(size_t i);
    // 交换节点
    void swapNode(size_t i, size_t j);

private:
    std::vector<TimerNode> heap_;
    std::unordered_map<int, size_t> ref_;   // heap::id : i
};

#endif