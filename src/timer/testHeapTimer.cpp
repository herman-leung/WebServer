#include <iostream>
#include <chrono>
#include <thread>
#include "HeapTimer.hpp"

using namespace bre;

std::chrono::time_point start = std::chrono::high_resolution_clock::now();
// 任务函数
void taskFunction(std::chrono::milliseconds delay) {
    auto now = std::chrono::high_resolution_clock::now();
    std::cout << "Task started after " << delay.count() << "ms, current time is "
        << std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() << "ms \n";
}

// 测试函数
void testTaskFunction() {
    using namespace std::chrono_literals;
    // 创建一个堆定时器实例
    start = std::chrono::high_resolution_clock::now();
    HeapTimer timer;
    for (int i = 1; i < 5; ++i) {
        int taskId = i;
        timer.Add(taskId, 200 * i, std::bind(&taskFunction, i * 200ms));
    }

    
    MS sleepTime(2000);     // 等待一段时间，确保任务有足够的时间被执行
    while (std::chrono::high_resolution_clock::now() - sleepTime < start) {
        auto t = timer.GetNextTick();
		if (t == MS::max()) {
			break;
		}
        std::cout << "Next tick: " << (t).count() << "ms\n";
		std::this_thread::sleep_for(t);
    }
}

int main() {
    testTaskFunction();
    return 0;

}