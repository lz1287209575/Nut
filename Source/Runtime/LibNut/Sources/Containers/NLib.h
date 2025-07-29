#pragma once

// NLib - Nut引擎的STL容器库
// 基于NObject的高性能容器系统，集成内存管理和垃圾回收

// 基础对象系统
#include "NObject.h"
#include "NMemoryManager.h"
#include "NGarbageCollector.h"

// 容器基础设施
#include "NContainer.h"

// 具体容器实现
#include "NString.h"
#include "NArray.h"
#include "NHashMap.h"

// 工具和算法
namespace NLib
{
    // 容器工厂函数
    template<typename T>
    inline NArray<T> MakeArray(std::initializer_list<T> init_list)
    {
        return NArray<T>(init_list);
    }
    
    template<typename K, typename V>
    inline NHashMap<K, V> MakeHashMap(std::initializer_list<NKeyValuePair<K, V>> init_list)
    {
        return NHashMap<K, V>(init_list);
    }
    
    // 字符串工具函数
    inline NString MakeString(const char* str)
    {
        return NString(str);
    }
    
    inline NString MakeString(const std::string& str)
    {
        return NString(str.c_str(), str.length());
    }
    
    // 算法工具
    template<typename Container, typename Predicate>
    auto FindIf(Container& container, Predicate pred) -> decltype(container.Begin())
    {
        for (auto it = container.Begin(); it != container.End(); ++it)
        {
            if (pred(*it))
            {
                return it;
            }
        }
        return container.End();
    }
    
    template<typename Container, typename Function>
    void ForEach(Container& container, Function func)
    {
        for (auto it = container.Begin(); it != container.End(); ++it)
        {
            func(*it);
        }
    }
    
    template<typename Container, typename T>
    size_t Count(const Container& container, const T& value)
    {
        size_t count = 0;
        for (auto it = container.Begin(); it != container.End(); ++it)
        {
            if (NEqual<T>{}(*it, value))
            {
                ++count;
            }
        }
        return count;
    }
    
    template<typename Container, typename Predicate>
    size_t CountIf(const Container& container, Predicate pred)
    {
        size_t count = 0;
        for (auto it = container.Begin(); it != container.End(); ++it)
        {
            if (pred(*it))
            {
                ++count;
            }
        }
        return count;
    }
    
    // 内存统计工具
    struct ContainerMemoryStats
    {
        size_t total_allocated;
        size_t total_used;
        size_t string_count;
        size_t array_count;
        size_t hashmap_count;
        
        ContainerMemoryStats()
            : total_allocated(0), total_used(0)
            , string_count(0), array_count(0), hashmap_count(0)
        {}
    };
    
    // 获取所有容器的内存使用统计
    ContainerMemoryStats GetMemoryStats();
    
    // 容器性能测试
    void RunPerformanceTests();
    
    // 容器功能测试
    void RunFunctionalTests();
}

// 常用类型别名
using NStringArray = NArray<NString>;
using NIntArray = NArray<int32_t>;
using NFloatArray = NArray<float>;
using NStringMap = NHashMap<NString, NString>;
using NStringIntMap = NHashMap<NString, int32_t>;
using NIntStringMap = NHashMap<int32_t, NString>;

// 预定义的容器特化
extern template class NArray<int32_t>;
extern template class NArray<float>;
extern template class NArray<NString>;
extern template class NHashMap<NString, NString>;
extern template class NHashMap<NString, int32_t>;
extern template class NHashMap<int32_t, NString>;

// 宏定义简化容器使用
#define NLIB_MAKE_ARRAY(...) NLib::MakeArray({__VA_ARGS__})
#define NLIB_MAKE_STRING(str) NLib::MakeString(str)
#define NLIB_MAKE_HASHMAP(...) NLib::MakeHashMap({__VA_ARGS__})

// 调试和日志宏
#ifdef NLIB_DEBUG
    #define NLIB_LOG_CONTAINER_OP(op, container) \
        NLogger::GetLogger().Debug("NLib: {} on {} (size: {}, capacity: {})", \
            op, typeid(container).name(), container.GetSize(), container.GetCapacity())
#else
    #define NLIB_LOG_CONTAINER_OP(op, container)
#endif

// 性能分析宏
#ifdef NLIB_PROFILE
    #include <chrono>
    #define NLIB_PROFILE_BEGIN(name) \
        auto nlib_profile_start_##name = std::chrono::high_resolution_clock::now()
    #define NLIB_PROFILE_END(name) \
        auto nlib_profile_end_##name = std::chrono::high_resolution_clock::now(); \
        auto nlib_profile_duration_##name = std::chrono::duration_cast<std::chrono::microseconds>( \
            nlib_profile_end_##name - nlib_profile_start_##name); \
        NLogger::GetLogger().Info("NLib Profile [{}]: {} microseconds", #name, \
            nlib_profile_duration_##name.count())
#else
    #define NLIB_PROFILE_BEGIN(name)
    #define NLIB_PROFILE_END(name)
#endif