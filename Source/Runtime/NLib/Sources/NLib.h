#pragma once

// NLib - Nut引擎核心库
// 提供对象系统、内存管理、容器、反射和日志功能的统一入口

// 版本信息
#define NLIB_VERSION_MAJOR 1
#define NLIB_VERSION_MINOR 0
#define NLIB_VERSION_PATCH 0
#define NLIB_VERSION "1.0.0"

// 核心模块 - 基础对象系统
#include "Core/CObject.h"

// 日志模块 - 统一日志系统
#include "Containers/CArray.h"
#include "Containers/CHashMap.h"
#include "Containers/CString.h"
#include "Logging/CLogger.h"

// LibNut命名空间 - 提供全局工具函数
namespace NLib
{
// 版本信息
struct Version
{
	int Major = NLIB_VERSION_MAJOR;
	int Minor = NLIB_VERSION_MINOR;
	int Patch = NLIB_VERSION_PATCH;
	const char* String = NLIB_VERSION;

	Version() = default;
};

// 获取版本信息
inline const Version& GetVersion()
{
	static Version Version;
	return Version;
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
	size_t TotalAllocated = 0;
	size_t TotalUsed = 0;
	size_t PeakUsage = 0;
	size_t GcObjectsCount = 0;
	size_t ContainerCount = 0;

	MemoryStats() = default;
};

// 获取内存使用统计
MemoryStats GetMemoryStats();

// 强制执行垃圾回收
void ForceGarbageCollect();

// 性能统计
struct PerformanceStats
{
	uint64_t ObjectCreations = 0;
	uint64_t ObjectDestructions = 0;
	uint64_t MemoryAllocations = 0;
	uint64_t MemoryDeallocations = 0;
	uint64_t GcRuns = 0;
	double AverageGcTimeMs = 0.0;

	PerformanceStats() = default;
};

// 获取性能统计数据
PerformanceStats GetPerformanceStats();

// 重置统计计数器
void ResetStats();

// 便利创建函数
CString MakeString(const char* str);
CString MakeString(const std::string& str);
CString MakeString(std::string_view str);

template <typename TType>
CArray<T> MakeArray(std::initializer_list<T> list);

template <typename TKey, typename TValue>
CHashMap<K, V> MakeHashMap(std::initializer_list<std::pair<K, V>> list);

// 模板函数实现
template <typename TType>
inline CArray<T> MakeArray(std::initializer_list<T> list)
{
	CArray<T> array;
	array.Reserve(list.size());
	for (const auto& item : list)
	{
		array.Add(item);
	}
	return array;
}

template <typename TKey, typename TValue>
inline CHashMap<K, V> MakeHashMap(std::initializer_list<std::pair<K, V>> list)
{
	CHashMap<K, V> map;
	map.Reserve(list.size());
	for (const auto& pair : list)
	{
		map.Add(pair.first, pair.second);
	}
	return map;
}

// 日志便利函数
inline void LogInfo(const char* message)
{
	CLogger::Info(message);
}

inline void LogWarning(const char* message)
{
	CLogger::Warn(message);
}

inline void LogError(const char* message)
{
	CLogger::Error(message);
}

// 模板化日志函数
template <typename... Args>
inline void LogInfo(const char* format, Args&&... args)
{
	CLogger::Info(format, std::forward<Args>(args)...);
}

template <typename... Args>
inline void LogWarning(const char* format, Args&&... args)
{
	CLogger::Warn(format, std::forward<Args>(args)...);
}

template <typename... Args>
inline void LogError(const char* format, Args&&... args)
{
	CLogger::Error(format, std::forward<Args>(args)...);
}

} // namespace NLib

// 全局便利宏
#define NLIB_LOG_INFO(...) NLib::LogInfo(__VA_ARGS__)
#define NLIB_LOG_WARNING(...) NLib::LogWarning(__VA_ARGS__)
#define NLIB_LOG_ERROR(...) NLib::LogError(__VA_ARGS__)

// 容器便利宏
#define NLIB_ARRAY(...) NLib::MakeArray({__VA_ARGS__})
#define NLIB_STRING(str) NLib::MakeString(str)
#define NLIB_HASHMAP(...) NLib::MakeHashMap({__VA_ARGS__})

// 对象池便利宏
#define NLIB_CREATE_POOL(Type, ...) NLib::CreateObjectPool<Type>(__VA_ARGS__)

// 自动初始化支持 (可选)
#ifdef NLIB_AUTO_INITIALIZE
namespace NLib
{
namespace Detail
{
struct AutoInitializer
{
	AutoInitializer()
	{
		NLib::Initialize();
	}
	~AutoInitializer()
	{
		NLib::Shutdown();
	}
};
static AutoInitializer GAutoInitializer;
} // namespace Detail
} // namespace NLib
#endif
