#include <iostream>
#include <chrono>
#include "ThreadPool.hpp"
using namespace bre;
void testTask(int id) {
    std::cout << "Executing task " << id << " on thread " 
              << std::this_thread::get_id() << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 模拟耗时操作
}

int main() {
    ThreadPool pool(4);

    for (int i = 0; i < 10; ++i) {
        pool.enqueue(testTask, i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "All tasks have been processed." << std::endl;

    return 0;
}