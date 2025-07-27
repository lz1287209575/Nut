#pragma once

/**
 * @file NLib.h
 * @brief NLib 主头文件 - 包含所有核心功能
 * 
 * 这个文件提供了NLib框架的统一入口点，
 * 包括内存管理、对象系统、垃圾回收等核心功能
 */

// 核心系统
#include "NMemoryManager.h"
#include "NAllocator.h"
#include "NObject.h"
#include "NGarbageCollector.h"
#include "NLogger.h"

// 测试和示例
#include "NTestObject.h"

namespace Nut {

/**
 * @brief NLib 框架初始化配置
 */
struct NLibConfig {
    // 内存管理配置
    bool bEnableMemoryProfiling = false;    // 是否启用内存分析
    
    // GC配置
    NGarbageCollector::EGCMode GCMode = NGarbageCollector::EGCMode::Adaptive;
    uint32_t GCIntervalMs = 5000;           // GC间隔（毫秒）
    bool bEnableBackgroundGC = true;        // 是否启用后台GC
    size_t GCMemoryThreshold = 100 * 1024 * 1024;  // GC内存阈值（100MB）
    
    // 日志配置
    NLogger::LogLevel LogLevel = NLogger::LogLevel::Info;
    bool bEnableFileLogging = true;         // 是否启用文件日志
    std::string LogFilePath = "NLib.log"; // 日志文件路径
};

/**
 * @brief NLib 框架核心类
 * 
 * 负责整个框架的初始化、配置和生命周期管理
 * 提供统一的访问接口
 */
class NLib {
public:
    /**
     * @brief 初始化NLib框架
     * @param Config 初始化配置
     * @return true if successful, false otherwise
     */
    static bool Initialize(const NLibConfig& Config = NLibConfig{});
    
    /**
     * @brief 关闭NLib框架
     */
    static void Shutdown();
    
    /**
     * @brief 检查框架是否已初始化
     */
    static bool IsInitialized();
    
    /**
     * @brief 获取内存管理器实例
     */
    static NMemoryManager& GetMemoryManager();
    
    /**
     * @brief 获取垃圾回收器实例
     */
    static NGarbageCollector& GetGarbageCollector();
    
    /**
     * @brief 运行GC测试套件
     */
    static void RunGCTests();
    
    /**
     * @brief 打印框架状态信息
     */
    static void PrintStatus();
    
    /**
     * @brief 获取框架版本信息
     */
    static const char* GetVersion();

private:
    static bool bInitialized;
    static NLibConfig CurrentConfig;
};

// 便利宏定义

/**
 * @brief 创建托管对象的便利宏
 */
#define CREATE_OBJECT(Type, ...) NObject::Create<Type>(__VA_ARGS__)

/**
 * @brief 手动触发GC的便利宏
 */
#define TRIGGER_GC() NLib::GetGarbageCollector().Collect()

/**
 * @brief 快速日志宏
 */
#define NLIB_LOG_INFO(msg) NLogger::Info(msg)
#define NLIB_LOG_DEBUG(msg) NLogger::Debug(msg)
#define NLIB_LOG_WARNING(msg) NLogger::Warning(msg)
#define NLIB_LOG_ERROR(msg) NLogger::Error(msg)

} // namespace Nut
