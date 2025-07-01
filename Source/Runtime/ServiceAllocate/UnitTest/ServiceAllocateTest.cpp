#include <iostream>
#include <thread>
#include <chrono>
#include "../Sources/ServiceAllocateManager.h"
#include "../Sources/ConfigManager.h"
#include "../Sources/Logger.h"

void TestServiceAllocation() {
    std::cout << "=== ServiceAllocate 功能测试 ===" << std::endl;
    // ... 其余内容保持不变 ...
}

int main() {
    try {
        TestServiceAllocation();
    } catch (const std::exception& e) {
        std::cerr << "测试异常: " << e.what() << std::endl;
        return -1;
    }
    return 0;
} 