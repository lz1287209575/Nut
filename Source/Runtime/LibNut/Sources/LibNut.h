#pragma once

// LibNut - Nut引擎核心库
// 提供对象系统、内存管理、容器、反射和日志功能的统一入口

// 版本信息
#define LIBNUT_VERSION_MAJOR 1
#define LIBNUT_VERSION_MINOR 0
#define LIBNUT_VERSION_PATCH 0
#define LIBNUT_VERSION "1.0.0"

// 平台和编译器检测
#ifdef _WIN32
    #define LIBNUT_PLATFORM_WINDOWS
    #ifdef _WIN64
        #define LIBNUT_PLATFORM_WIN64
    #else
        #define LIBNUT_PLATFORM_WIN32
    #endif
#elif defined(__APPLE__)
    #define LIBNUT_PLATFORM_APPLE
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #define LIBNUT_PLATFORM_MACOS
    #endif
#elif defined(__linux__)
    #define LIBNUT_PLATFORM_LINUX
#endif

// 编译器特定设置
#if defined(__clang__)
    #define LIBNUT_COMPILER_CLANG
#elif defined(__GNUC__)
    #define LIBNUT_COMPILER_GCC
#elif defined(_MSC_VER)
    #define LIBNUT_COMPILER_MSVC
#endif

// API导出宏
#ifndef LIBNLIB_API
    #if defined(LIBNUT_PLATFORM_WINDOWS)
        #if defined(LIBNUT_BUILD_DLL)
            #define LIBNLIB_API __declspec(dllexport)
        #elif defined(LIBNUT_USE_DLL)
            #define LIBNLIB_API __declspec(dllimport)
        #else
            #define LIBNLIB_API
        #endif
    #else
        #define LIBNLIB_API
    #endif
#endif

// 调试和断言宏
#ifdef DEBUG
    #define LIBNUT_DEBUG 1
    #include <cassert>
    #define LIBNUT_ASSERT(condition, message) assert((condition) && (message))
    #define LIBNUT_DEBUG_ONLY(code) code
#else
    #define LIBNUT_DEBUG 0
    #define LIBNUT_ASSERT(condition, message)
    #define LIBNUT_DEBUG_ONLY(code)
#endif

// 性能分析开关
#ifdef LIBNUT_ENABLE_PROFILING
    #define LIBNUT_PROFILE 1
#else
    #define LIBNUT_PROFILE 0
#endif

// 核心模块 - 基础对象系统
#include "Core/NObject.h"

// 内存管理模块 - 内存分配、GC和管理器
#include "Memory/NAllocator.h"
#include "Memory/NMemoryManager.h"
#include "Memory/NGarbageCollector.h"

// 日志模块 - 统一日志系统
#include "Logging/NLogger.h"

// 反射模块 - 运行时类型信息
#include "Reflection/NObjectReflection.h"

// 容器模块 - STL容器库
#include "Containers/NContainer.h"
#include "Containers/NString.h"
#include "Containers/NArray.h"
#include "Containers/NHashMap.h"
#include "Containers/NLib.h"

// LibNut命名空间 - 提供全局工具函数
namespace LibNut
{
    // 版本信息
    struct Version
    {
        int major;
        int minor;
        int patch;
        const char* string;
        
        Version() : major(LIBNUT_VERSION_MAJOR), minor(LIBNUT_VERSION_MINOR), 
                   patch(LIBNUT_VERSION_PATCH), string(LIBNUT_VERSION) {}
    };
    
    // 获取版本信息
    inline const Version& GetVersion()
    {
        static Version version;
        return version;
    }
    
    // 初始化LibNut库
    // 必须在使用任何LibNut功能之前调用
    bool Initialize();
    
    // 反初始化LibNut库
    // 程序结束前调用，清理资源
    void Shutdown();
    
    // 库状态查询
    bool IsInitialized();
    
    // 内存统计
    struct MemoryStats
    {
        size_t total_allocated;
        size_t total_used;
        size_t peak_usage;
        size_t gc_objects_count;
        size_t container_count;
        
        MemoryStats() : total_allocated(0), total_used(0), peak_usage(0),
                       gc_objects_count(0), container_count(0) {}
    };
    
    // 获取内存使用统计
    MemoryStats GetMemoryStats();
    
    // 强制执行垃圾回收
    void ForceGarbageCollect();
    
    // 性能统计
    struct PerformanceStats
    {
        uint64_t object_creations;
        uint64_t object_destructions;
        uint64_t memory_allocations;
        uint64_t memory_deallocations;
        uint64_t gc_runs;
        double average_gc_time_ms;
        
        PerformanceStats() : object_creations(0), object_destructions(0),
                           memory_allocations(0), memory_deallocations(0),
                           gc_runs(0), average_gc_time_ms(0.0) {}
    };
    
    // 获取性能统计数据
    PerformanceStats GetPerformanceStats();
    
    // 重置统计计数器
    void ResetStats();
    
    // 日志便利函数
    inline void LogInfo(const char* message) 
    { 
        NLogger::GetLogger().Info(message); 
    }
    
    inline void LogWarning(const char* message) 
    { 
        NLogger::GetLogger().Warning(message); 
    }
    
    inline void LogError(const char* message) 
    { 
        NLogger::GetLogger().Error(message); 
    }
    
    // 模板化日志函数
    template<typename... Args>
    inline void LogInfo(const char* format, Args&&... args)
    {
        NLogger::GetLogger().Info(format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    inline void LogWarning(const char* format, Args&&... args)
    {
        NLogger::GetLogger().Warning(format, std::forward<Args>(args)...);
    }
    
    template<typename... Args>
    inline void LogError(const char* format, Args&&... args)
    {
        NLogger::GetLogger().Error(format, std::forward<Args>(args)...);
    }
    
    // 容器工厂函数 (从NLib命名空间中提升)
    using NLib::MakeArray;
    using NLib::MakeHashMap;
    using NLib::MakeString;
    
    // 常用类型别名
    using String = NString;
    template<typename T> using Array = NArray<T>;
    template<typename K, typename V> using HashMap = NHashMap<K, V>;
    using StringArray = NArray<NString>;
    using StringMap = NHashMap<NString, NString>;
}

// 全局便利宏
#define LIBNUT_LOG_INFO(...) LibNut::LogInfo(__VA_ARGS__)
#define LIBNUT_LOG_WARNING(...) LibNut::LogWarning(__VA_ARGS__)
#define LIBNUT_LOG_ERROR(...) LibNut::LogError(__VA_ARGS__)

// 容器便利宏
#define LIBNUT_ARRAY(...) LibNut::MakeArray({__VA_ARGS__})
#define LIBNUT_STRING(str) LibNut::MakeString(str)
#define LIBNUT_HASHMAP(...) LibNut::MakeHashMap({__VA_ARGS__})

// 自动初始化支持 (可选)
#ifdef LIBNUT_AUTO_INITIALIZE
namespace LibNut { namespace Detail {
    struct AutoInitializer {
        AutoInitializer() { LibNut::Initialize(); }
        ~AutoInitializer() { LibNut::Shutdown(); }
    };
    static AutoInitializer g_auto_initializer;
}}
#endif