#ifndef BLOCKQUEUE_HPP
#define BLOCKQUEUE_HPP

#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>

namespace bre
{
  template <typename T>
  class BlockQueue
  {
  public:
    explicit BlockQueue(size_t MaxCapacity = 1024) : capacity(MaxCapacity)
    {
      isClose = false;
    }

    ~BlockQueue()
    {
      Close();
    }

    void Clear()
    {
      std::lock_guard<std::mutex> locker(mtx);
      queue.clear();
    }

    bool Empty()
    {
      std::lock_guard<std::mutex> locker(mtx);
      return queue.empty();
    }

    bool Full()
    {
      std::lock_guard<std::mutex> locker(mtx);
      return queue.size() >= capacity;
    }

    void Close()
    {
      {
        std::lock_guard<std::mutex> locker(mtx);
        //
        while (!queue.empty())
        {
          queue.pop();
        }
        isClose = true;
      }
      condProducer.notify_all();
      condConsumer.notify_all();
    }

    size_t Size()
    {
      std::lock_guard<std::mutex> locker(mtx);
      return queue.size();
    }

    size_t Capacity()
    {
      std::lock_guard<std::mutex> locker(mtx);
      return capacity;
    }

    T Back()
    {
      std::lock_guard<std::mutex> locker(mtx);
      return queue.back();
    }

    // 在 Push 方法中，使用 condProducer.wait(locker, [this]() { return queue.size() < capacity || isClose; }); 确保线程在队列未满或队列关闭时被唤醒。
    // 在 Pop 方法中，使用 condConsumer.wait(locker, [this]() { return !queue.empty() || isClose; }); 确保线程在队列不为空或队列关闭时被唤醒。
    void Push(const T &item)
    {
      std::unique_lock<std::mutex> locker(mtx); // 锁定互斥量， 确保线程安全
      condProducer.wait(locker, [&]()
                        { return queue.size() < capacity || isClose; });
      if (isClose)
        return;
      queue.push(item);
      std::cout << "notify one....." << std::endl;
      condConsumer.notify_one();
    }

    bool Pop(T &item)
    {
      std::unique_lock<std::mutex> locker(mtx);
      condConsumer.wait(locker, [&]()
                        { return !queue.empty() || isClose; });
      if (isClose && queue.empty())
        return false;
      item = queue.front();
      queue.pop();
      condProducer.notify_one();
      return true;
    }

    bool Pop(T &item, int timeout)
    {
      std::unique_lock<std::mutex> locker(mtx);
      while (queue.empty())
      {
        if (condConsumer.wait_for(locker, std::chrono::seconds(timeout)) == std::cv_status::timeout)
        {
          return false;
        }
        if (isClose)
        {
          return false;
        }
      }

      item = queue.front();
      queue.pop();
      condProducer.notify_one();
      return true;
    }

    void Flush()
    {
      condConsumer.notify_one();
    }

  private:
    size_t capacity;
    bool isClose;
    std::queue<T> queue;
    std::mutex mtx;
    std::condition_variable condConsumer;
    std::condition_variable condProducer;
  };
}
#endif // BLOCKQUEUE_HPP