#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <signal.h>
#include "ServiceAllocateManager.h"
#include "ConfigManager.h"
#include "Logger.h"

std::unique_ptr<FServiceAllocateManager> GServiceAllocateManager;
std::unique_ptr<FConfigManager> GConfigManager;
std::unique_ptr<FLogger> GLogger;

// 信号处理函数
void SignalHandler(int Signal) {
    if (Signal == SIGINT || Signal == SIGTERM) {
        GLogger->Info("收到退出信号，正在关闭服务...");
        if (GServiceAllocateManager) {
            GServiceAllocateManager->Shutdown();
        }
        exit(0);
    }
}

int main() {
    try {
        // 初始化日志系统
        GLogger = std::make_unique<FLogger>();
        GLogger->Info("ServiceAllocate 服务启动中...");
        
        // 初始化配置管理器
        GConfigManager = std::make_unique<FConfigManager>();
        if (!GConfigManager->LoadConfig("Configs/ServiceAllocateConfig.json")) {
            GLogger->Error("加载配置文件失败");
            return -1;
        }
        
        GLogger->Info("配置文件加载成功");
        
        // 初始化服务分配管理器
        GServiceAllocateManager = std::make_unique<FServiceAllocateManager>(GConfigManager.get());
        if (!GServiceAllocateManager->Initialize()) {
            GLogger->Error("服务分配管理器初始化失败");
            return -1;
        }
        
        GLogger->Info("ServiceAllocate 服务启动成功，监听端口: " + std::to_string(GConfigManager->GetListenPort()));
        
        // 注册信号处理
        signal(SIGINT, SignalHandler);
        signal(SIGTERM, SignalHandler);
        
        // 启动服务
        GServiceAllocateManager->Start();
        
        // 主循环
        while (GServiceAllocateManager->IsRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        GLogger->Info("ServiceAllocate 服务正常退出");
        
    } catch (const std::exception& e) {
        std::cerr << "ServiceAllocate 服务异常退出: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
