#include "heapTimer.h"

void HeapTimer::siftup(size_t i)
{
    assert(i >= 0 && i < heap_.size());
    size_t j = (i - 1) / 2;
    while (i > 0)
    {
        if (heap_[i] > heap_[j])
            break;
        swapNode(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

bool HeapTimer::siftdown(size_t i, size_t n)
{
    assert(i >= 0 && i <= heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t save = i;
    size_t j = i * 2 + 1;
    while (j < n)
    {
        if (j + 1 < n && heap_[j] > heap_[j + 1])
            j++;
        if (heap_[i] < heap_[j])
            break;
        swapNode(i, j);
        i = j;
        j = i * 2 + 1;
    }
    // 若没有发生节点交换则返回false
    return save > i;
}

void HeapTimer::swapNode(size_t i, size_t j)
{
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

void HeapTimer::del(size_t i)
{
    assert(!heap_.empty() && i >= 0 && i < heap_.size());
    // 将要删除的节点换到队尾，然后调整堆
    size_t n = heap_.size() - 1;
    if (i < n)
    {
        swapNode(i, n);
        if (!siftdown(i, n))
            siftup(i);
    }
    // 删除队尾元素
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void HeapTimer::add(int id, int timeout, const TimeoutCallback& cb)
{
    assert(id >= 0);
    if (ref_.count(id) == 0)    // 新节点
    {
        size_t i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id, Clock::now() + Ms(timeout), cb});
        siftup(i);
    }
    else    // 已有节点，调整堆
    {
        size_t i = ref_[id];
        heap_[i].expires =  Clock::now() + Ms(timeout);
        heap_[i].cb = cb;
        if (!siftdown(i, heap_.size()))
        {
            siftup(i);
        }
    }
}

void HeapTimer::adjust(int id, int timeout)
{
    assert(!heap_.empty() && ref_.count(id) != 0);
    heap_[ref_[id]].expires = Clock::now() + Ms(timeout);
    siftdown(ref_[id], heap_.size());
}

void HeapTimer::doWork(int id)
{
    assert(!heap_.empty() && ref_.count(id) != 0);
    heap_[ref_[id]].cb();
    del(ref_[id]);
}

void HeapTimer::clear()
{
    ref_.clear();
    heap_.clear();
}

void HeapTimer::tick()
{
    if (heap_.empty())
        return;

    while (!heap_.empty())
    {
        TimerNode node = heap_.front();
        if (std::chrono::duration_cast<Ms>(node.expires - Clock::now()).count() > 0)
            break;
        node.cb();
        pop();
    }
}

void HeapTimer::pop()
{
    assert(!heap_.empty());
    del(0);
}

int HeapTimer::getNextTick()
{
    tick();
    size_t res = -1;
    if (!heap_.empty())
    {
        res = std::chrono::duration_cast<Ms>(heap_.front().expires - Clock::now()).count();
        if (res < 0)
            res = 0;
    }
    return res;
}