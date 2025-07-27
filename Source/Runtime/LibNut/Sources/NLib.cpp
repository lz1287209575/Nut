#include "NLib.h"
#include <iostream>

namespace Nut {

// 静态成员初始化
bool NLib::bInitialized = false;
NLibConfig NLib::CurrentConfig = {};

bool NLib::Initialize(const NLibConfig& Config) {
    if (bInitialized) {
        NLogger::Warning("NLib already initialized");
        return true;
    }
    
    CurrentConfig = Config;
    
    try {
        // 1. 初始化日志系统
        NLogger::SetLogLevel(Config.LogLevel);
        if (Config.bEnableFileLogging) {
            // TODO: 实现文件日志功能
        }
        
        NLogger::Info("🚀 Initializing NLib Framework");
        NLogger::Info("Version: " + std::string(GetVersion()));
        
        // 2. 初始化内存管理器
        auto& MemMgr = NMemoryManager::GetInstance();
        MemMgr.Initialize(Config.bEnableMemoryProfiling);
        NLogger::Info("✅ Memory Manager initialized");
        
        // 3. 初始化垃圾回收器
        auto& GC = NGarbageCollector::GetInstance();
        GC.Initialize(Config.GCMode, Config.GCIntervalMs, Config.bEnableBackgroundGC);
        GC.SetMemoryThreshold(Config.GCMemoryThreshold);
        NLogger::Info("✅ Garbage Collector initialized");
        
        bInitialized = true;
        
        NLogger::Info("🎉 NLib Framework initialized successfully!");
        PrintStatus();
        
        return true;
        
    } catch (const std::exception& e) {
        NLogger::Error("Failed to initialize NLib: " + std::string(e.what()));
        return false;
    }
}

void NLib::Shutdown() {
    if (!bInitialized) {
        return;
    }
    
    NLogger::Info("🔄 Shutting down NLib Framework");
    
    try {
        // 1. 关闭垃圾回收器
        auto& GC = NGarbageCollector::GetInstance();
        GC.Shutdown();
        NLogger::Info("✅ Garbage Collector shutdown");
        
        // 2. 关闭内存管理器（最后关闭，因为其他组件可能依赖它）
        auto& MemMgr = NMemoryManager::GetInstance();
        auto FinalStats = MemMgr.GetStats();
        
        NLogger::Info("📊 Final Memory Statistics:");
        NLogger::Info("  - Total Allocated: " + std::to_string(FinalStats.TotalAllocated) + " bytes");
        NLogger::Info("  - Current Usage: " + std::to_string(FinalStats.CurrentUsage) + " bytes");
        NLogger::Info("  - Peak Usage: " + std::to_string(FinalStats.PeakUsage) + " bytes");
        
        if (FinalStats.CurrentUsage > 0) {
            NLogger::Warning("Memory leak detected: " + 
                             std::to_string(FinalStats.CurrentUsage) + 
                             " bytes still allocated");
        }
        
        bInitialized = false;
        
        NLogger::Info("👋 NLib Framework shutdown completed");
        
    } catch (const std::exception& e) {
        NLogger::Error("Error during NLib shutdown: " + std::string(e.what()));
    }
}

bool NLib::IsInitialized() {
    return bInitialized;
}

NMemoryManager& NLib::GetNMemoryManager() {
    if (!bInitialized) {
        throw std::runtime_error("NLib not initialized. Call NLib::Initialize() first.");
    }
    return NMemoryManager::GetInstance();
}

NGarbageCollector& NLib::GetNGarbageCollector() {
    if (!bInitialized) {
        throw std::runtime_error("NLib not initialized. Call NLib::Initialize() first.");
    }
    return NGarbageCollector::GetInstance();
}

void NLib::RunGCTests() {
    if (!bInitialized) {
        NLogger::Error("NLib not initialized. Cannot run tests.");
        return;
    }
    
    NLogger::Info("🧪 Starting NLib GC Test Suite");
    GCTester::RunAllTests();
}

void NLib::PrintStatus() {
    if (!bInitialized) {
        std::cout << "❌ NLib Framework: Not Initialized" << std::endl;
        return;
    }
    
    std::cout << "=== NLib Framework Status ===" << std::endl;
    std::cout << "Version: " << GetVersion() << std::endl;
    std::cout << "Status: ✅ Initialized" << std::endl;
    
    // 内存管理器状态
    auto& MemMgr = NMemoryManager::GetInstance();
    auto MemStats = MemMgr.GetStats();
    std::cout << "\n📊 Memory Manager:" << std::endl;
    std::cout << "  - Current Usage: " << MemStats.CurrentUsage << " bytes" << std::endl;
    std::cout << "  - Peak Usage: " << MemStats.PeakUsage << " bytes" << std::endl;
    std::cout << "  - Total Allocations: " << MemStats.AllocationCount << std::endl;
    
    // 垃圾回收器状态
    auto& GC = NGarbageCollector::GetInstance();
    auto GCStats = GC.GetStats();
    std::cout << "\n🔄 Garbage Collector:" << std::endl;
    std::cout << "  - Mode: " << static_cast<int>(GC.GetGCMode()) << std::endl;
    std::cout << "  - Objects Alive: " << GCStats.ObjectsAlive << std::endl;
    std::cout << "  - Total Collections: " << GCStats.TotalCollections << std::endl;
    std::cout << "  - Objects Collected: " << GCStats.ObjectsCollected << std::endl;
    std::cout << "  - Is Collecting: " << (GC.IsCollecting() ? "Yes" : "No") << std::endl;
    
    std::cout << "===============================" << std::endl;
}

const char* NLib::GetVersion() {
    return "1.0.0-alpha";
}

} // namespace Nut