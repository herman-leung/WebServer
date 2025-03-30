#include "Config.hpp"
#include <iostream>
#include <optional>
using namespace std;
using namespace bre;
// 测试函数
void testConfig() {
    // 假设 config.txt 文件内容如下：
    // key1:value1
    // key2:value2

    std::cout << "Getting configuration values:" << std::endl;
    std::cout << "key1: " << Config::getInstance().Get("key1").value() << std::endl;
    std::cout << "key2: " << Config::getInstance().Get("key2").value() << std::endl;
    std::cout << "missing_key: " << Config::getInstance().Get("missing_key").value() << std::endl;
}

int main() {
    testConfig();
    return 0;
}