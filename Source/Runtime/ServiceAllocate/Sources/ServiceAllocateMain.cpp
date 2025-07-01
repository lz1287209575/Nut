#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <signal.h>
#include "ServiceAllocateManager.h"
#include "ConfigManager.h"
#include "Logger.h"

std::unique_ptr<ServiceAllocateManager> g_serviceManager;
std::unique_ptr<ConfigManager> g_configManager;
std::unique_ptr<Logger> g_logger;

// 信号处理函数
void SignalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        g_logger->Info("收到退出信号，正在关闭服务...");
        if (g_serviceManager) {
            g_serviceManager->Shutdown();
        }
        exit(0);
    }
}

int main() {
    try {
        // 初始化日志系统
        g_logger = std::make_unique<Logger>();
        g_logger->Info("ServiceAllocate 服务启动中...");
        
        // 初始化配置管理器
        g_configManager = std::make_unique<ConfigManager>();
        if (!g_configManager->LoadConfig("Configs/ServiceAllocateConfig.json")) {
            g_logger->Error("加载配置文件失败");
            return -1;
        }
        
        g_logger->Info("配置文件加载成功");
        
        // 初始化服务分配管理器
        g_serviceManager = std::make_unique<ServiceAllocateManager>(g_configManager.get());
        if (!g_serviceManager->Initialize()) {
            g_logger->Error("服务分配管理器初始化失败");
            return -1;
        }
        
        g_logger->Info("ServiceAllocate 服务启动成功，监听端口: " + 
                      std::to_string(g_configManager->GetListenPort()));
        
        // 注册信号处理
        signal(SIGINT, SignalHandler);
        signal(SIGTERM, SignalHandler);
        
        // 启动服务
        g_serviceManager->Start();
        
        // 主循环
        while (g_serviceManager->IsRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        g_logger->Info("ServiceAllocate 服务正常退出");
        
    } catch (const std::exception& e) {
        std::cerr << "ServiceAllocate 服务异常退出: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
