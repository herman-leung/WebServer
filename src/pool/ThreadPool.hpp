#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <vector>
namespace bre{
class ThreadPool {
public:
    ThreadPool(size_t threadCount = std::thread::hardware_concurrency()) : stop(false) {
        if (threadCount == 0) {
            threadCount = 8;
        }
        threads.reserve(threadCount);
        for (size_t i = 0; i < threadCount; ++i) {
            threads.emplace_back([this] { this->worker(); });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();

        for (std::thread &worker : threads) {
            worker.join();
        }
    }

    template<typename Func, typename... Args>
    void enqueue(Func &&f, Args &&...args) {
        std::function<void()> task = std::bind(std::forward<Func>(f), std::forward<Args>(args)...);

        { // queue_mutex
            std::lock_guard<std::mutex> lock(queue_mutex);

            if (stop) {
                return;
            }
            tasks.emplace(std::move(task));
        } // queue_mutex
        condition.notify_one();
    }

private:
    void worker() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                condition.wait(lock, [this] { return stop || !tasks.empty(); });

                if (stop && tasks.empty())
                    return;

                task = std::move(tasks.front());
                tasks.pop();
            }
            task();
        }
    }

    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

} // namespace bre
#endif //THREADPOOL_H