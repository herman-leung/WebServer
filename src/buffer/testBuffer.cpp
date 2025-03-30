#include <iostream>
#include <chrono>
#include <functional>
#include "Buffer.hpp"

class Test {
    std::chrono::time_point<std::chrono::system_clock> start;
    std::function<void()> callable;
public:
    Test(std::function<void()> func) : callable(func) {
        start = std::chrono::system_clock::now();
        if (callable) {
            callable();
        }
    }
    ~Test() {
        auto end = std::chrono::system_clock::now(); // 获取当前时间点
        std::chrono::duration<double, std::milli> d = end - start; // 计算时间差，并转换为毫秒
        std::cout << "Spend time: " << d.count() << "ms\n"; 
    }
};

void TestBuffer() {
    bre::Buffer buffer;

    // 测试初始状态
    std::cout << "Initial Writable Bytes: " << buffer.WritableBytes() << std::endl;
    std::cout << "Initial Readable Bytes: " << buffer.ReadableBytes() << std::endl;

    // 添加
    buffer.Append("Hello, ");
    char str[] = "World!";
    buffer.Append(str, 6);  // Buffer: Hello,World!

    // 获取
    std::cout << "获取: " << buffer.Retrieve(6) << std::endl;   // 获取“Hello,”  Buffer: World!
    // 寻找
    std::cout << "寻找: " << buffer.RetrieveUntil("o") << std::endl; // “Wo”    Buffer: rld!

    // 可读字符数和可写字符数
    std::cout << "Writable Bytes: " << buffer.WritableBytes() << std::endl;
    std::cout << "Readable Bytes: " << buffer.ReadableBytes() << std::endl;
    
    buffer.Append(str, 6);
    buffer.Retrieve(6);

    // 写入超过1024个字符
    for(int i = 0; i < 1000; ++i) {
        buffer.Append(str, 6);
    }
    buffer.Retrieve(5990);      // d!World!World! 从后往前14个字符剩下

    // 获取所有字符
    std::cout << "buffer.RetrieveAll(): " << buffer.RetrieveAll() <<"\n";

    // 清空
    buffer.Clear();

    // 测试清空后状态
    std::cout << "After Clear Writable Bytes: " << buffer.WritableBytes() << std::endl;
    std::cout << "After Clear Readable Bytes: " << buffer.ReadableBytes() << std::endl;
}

void TestPerformance() {
    bre::Buffer buffer;
    char str[] = "world!";
    for(int i = 0; i < 10000; ++i) {
        buffer.Append(str, 6);
        buffer.Retrieve(6);
    }
}
int main() {
    Test([]{TestBuffer();});
    Test([]{TestPerformance();});
    return 0;
}
