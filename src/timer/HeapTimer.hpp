#ifndef HEAP_TIMER_HPP
#define HEAP_TIMER_HPP

#include <queue>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <vector>
#include <algorithm>

namespace bre {

using Clock = std::chrono::high_resolution_clock;
using MS = std::chrono::milliseconds;
using TimeStamp = Clock::time_point;
using TimeoutCallback = std::function<void()>;

struct TimerNode {
    int id;
    TimeStamp expires;
    TimeoutCallback cb;

    bool operator<(const TimerNode& other) const noexcept {
        return expires != other.expires ? expires < other.expires : id < other.id;
    }
};

class HeapTimer {
public:
    explicit HeapTimer(size_t initialCapacity = 64) {
        heap.reserve(initialCapacity);
    }
    ~HeapTimer() { Clear(); }

    // 添加定时任务
    void Add(int id, int ms, const TimeoutCallback& callback) {
        if (id < 0) {
            return;
        }
        auto timeout = MS(ms);

        if (nodeIndices.find(id) != nodeIndices.end()) {
            // 已经存在的任务，调整定时时间
            Adjust(id, timeout);
            return;
        }

        heap.emplace_back(id, Clock::now() + timeout, callback);
        nodeIndices[id] = heap.size() - 1;
        siftup(heap.size() - 1);
    }

    // 调整定时任务的过期时间
    void Adjust(int id, MS newTimeout) {
        auto it = nodeIndices.find(id);
        if (it == nodeIndices.end()) {
            return;
        }
        size_t index = it->second;
        heap[index].expires = Clock::now() + newTimeout;
        if (!siftdown(index, heap.size())) {
            siftup(index);
        }
    }

    // 清空所有定时任务
    void Clear() {
        heap.clear();
        nodeIndices.clear();
    }


    // 获取下一个到期任务的时间间隔
	// 如果没有任务，返回一个最大时间间隔
	// 如果有任务，返回到期时间与当前时间的时间间隔
    [[nodiscard]] MS GetNextTick() {
        tick();

        if (heap.empty()) {
            return MS::max();
        }
        
        auto d = std::chrono::duration_cast<MS>(heap.front().expires - Clock::now());
        // 如果到期时间已经过了，返回0
        if (d < MS::zero()) {
            return MS::zero();
        }
        // d < 1ms, 返回1ms, 避免多次调用，如果外层对时间不敏感，可以不用这个判断
        if (d < MS(1)) {
            return MS(1);
        }

        return d;
    }

private:
    void pop() {
        del(0);
    }

    // 删除指定索引的任务
    void del(size_t i) {
		if (heap.empty()) {
			return;
		}
        size_t last = heap.size() - 1;
        swapNode(i, last);
        nodeIndices.erase(heap[last].id);
        heap.pop_back();


        if (i < heap.size()) {
            if (!siftdown(i, heap.size())) {
                siftup(i);
            }
        }
    }

    // 向上调整节点
    void siftup(size_t i) {
         while (i > 0) {
             size_t parent = (i - 1) / 2;
             if (heap[i] < heap[parent]) {
                 swapNode(i, parent);
                 i = parent;
             } else {
                 break;
             }
         }
    }

    // 向下调整节点
    bool siftdown(size_t index, size_t n) {
         size_t i = index;
         while (i * 2 + 1 < n) {
             size_t k = i * 2 + 1;
             if (k + 1 < n && heap[k + 1] < heap[k]) {
                 k++;
             }
             if (heap[k] < heap[i]) {
                 swapNode(i, k);
                 i = k;
             } else {
                 break;
             }
         }
         return i > index;
    }

    // 交换两个节点
    void swapNode(size_t i, size_t j) {
        std::swap(heap[i], heap[j]);
        nodeIndices[heap[i].id] = i;
        nodeIndices[heap[j].id] = j;
    }

    // 处理过期的任务
    void tick() {
        auto now = Clock::now();
        while (!heap.empty() && heap.front().expires <= now) {
            doWork(heap.front().id);
        }
    }

    // 执行指定ID的任务
    void doWork(int id) {
        auto it = nodeIndices.find(id);
        if (it == nodeIndices.end()) {
            return;
        }

        auto index = it->second;
        heap[index].cb();

        del(index);
    }

private:
    std::vector<TimerNode> heap;                    // 定时任务堆
    std::unordered_map<int, size_t> nodeIndices;    // 定时任务引用
};
}   // namespace bre
#endif // HEAP_TIMER_HPP