#pragma once

#include "Logger.h"

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace NLib
{
// 前向声明 - Logger.h 已包含 ELogLevel 定义
class NLogger;

/**
 * @brief 日志分类类
 *
 * 类似UE的LogCategory，用于对日志进行分类管理
 * 每个分类可以有独立的日志级别和前缀
 */
class CLogCategory
{
public:
	/**
	 * @brief 构造函数
	 * @param CategoryName 分类名称
	 * @param DefaultLevel 默认日志级别
	 * @param bVerboseLog 是否详细日志
	 */
	CLogCategory(const std::string& CategoryName, ELogLevel DefaultLevel, bool bVerboseLog = false);

	/**
	 * @brief 获取分类名称
	 * @return 分类名称
	 */
	const std::string& GetCategoryName() const
	{
		return CategoryName;
	}

	/**
	 * @brief 设置分类的日志级别
	 * @param Level 日志级别
	 */
	void SetLevel(ELogLevel Level);

	/**
	 * @brief 获取分类的日志级别
	 * @return 日志级别
	 */
	ELogLevel GetLevel() const;

	/**
	 * @brief 检查是否应该记录指定级别的日志
	 * @param Level 要检查的级别
	 * @return true if should log, false otherwise
	 */
	bool ShouldLog(ELogLevel Level) const;

	/**
	 * @brief 设置是否启用详细日志
	 * @param bEnable 是否启用
	 */
	void SetVerbose(bool bEnable);

	/**
	 * @brief 检查是否启用详细日志
	 * @return true if verbose enabled, false otherwise
	 */
	bool IsVerbose() const;

	/**
	 * @brief 获取格式化的日志前缀
	 * @return 日志前缀字符串
	 */
	std::string GetLogPrefix() const;

private:
	std::string CategoryName;         // 分类名称
	ELogLevel CategoryLevel;          // 分类日志级别
	bool bVerboseEnabled;             // 是否启用详细日志
	mutable std::mutex CategoryMutex; // 线程安全锁
};

/**
 * @brief 日志分类管理器
 *
 * 管理所有的日志分类，提供分类的注册、查找和管理功能
 */
class CLogCategoryManager
{
public:
	/**
	 * @brief 获取全局分类管理器实例
	 * @return 分类管理器实例引用
	 */
	static CLogCategoryManager& GetInstance();

	// 禁用拷贝和赋值
	CLogCategoryManager(const CLogCategoryManager&) = delete;
	CLogCategoryManager& operator=(const CLogCategoryManager&) = delete;

public:
	/**
	 * @brief 注册日志分类
	 * @param Category 日志分类
	 * @return 是否注册成功
	 */
	bool RegisterCategory(std::shared_ptr<CLogCategory> Category);

	/**
	 * @brief 获取日志分类
	 * @param CategoryName 分类名称
	 * @return 日志分类指针，如果不存在返回nullptr
	 */
	std::shared_ptr<CLogCategory> GetCategory(const std::string& CategoryName);

	/**
	 * @brief 创建或获取日志分类
	 * @param CategoryName 分类名称
	 * @param DefaultLevel 默认日志级别
	 * @param bVerboseLog 是否详细日志
	 * @return 日志分类指针
	 */
	std::shared_ptr<CLogCategory> GetOrCreateCategory(const std::string& CategoryName,
	                                                  ELogLevel DefaultLevel,
	                                                  bool bVerboseLog = false);

	/**
	 * @brief 设置所有分类的日志级别
	 * @param Level 日志级别
	 */
	void SetAllCategoriesLevel(ELogLevel Level);

	/**
	 * @brief 获取所有分类名称
	 * @return 分类名称列表
	 */
	std::vector<std::string> GetAllCategoryNames() const;

	/**
	 * @brief 打印所有分类的状态
	 */
	void PrintCategoriesStatus() const;

private:
	CLogCategoryManager() = default;
	~CLogCategoryManager() = default;

private:
	std::unordered_map<std::string, std::shared_ptr<CLogCategory>> Categories;
	mutable std::mutex CategoriesMutex;
};

// === 预定义的日志分类 ===

// 核心系统分类
extern std::shared_ptr<CLogCategory> LogCore;          // 核心系统
extern std::shared_ptr<CLogCategory> LogMemory;        // 内存管理
extern std::shared_ptr<CLogCategory> LogGC;            // 垃圾回收
extern std::shared_ptr<CLogCategory> LogReflection;    // 反射系统
extern std::shared_ptr<CLogCategory> LogSerialization; // 序列化

// 功能模块分类
extern std::shared_ptr<CLogCategory> LogNetwork;   // 网络模块
extern std::shared_ptr<CLogCategory> LogIO;        // IO操作
extern std::shared_ptr<CLogCategory> LogConfig;    // 配置管理
extern std::shared_ptr<CLogCategory> LogEvents;    // 事件系统
extern std::shared_ptr<CLogCategory> LogThreading; // 线程模块

// 调试和性能分类
extern std::shared_ptr<CLogCategory> LogPerformance; // 性能统计
extern std::shared_ptr<CLogCategory> LogDebug;       // 调试信息
extern std::shared_ptr<CLogCategory> LogVerbose;     // 详细日志

/**
 * @brief 初始化预定义的日志分类
 * 在NLogger初始化时调用
 */
void InitializeDefaultLogCategories();

} // namespace NLib

// === NLOG 宏定义 ===

/**
 * @brief 分类日志宏，类似UE的UE_LOG
 * 用法: NLOG(LogCore, Info, "Message: {}", value);
 */
#define NLOG(Category, Level, Format, ...)                                                                             \
	do                                                                                                                 \
	{                                                                                                                  \
		if ((Category) && (Category)->ShouldLog(NLib::ELogLevel::Level))                                               \
		{                                                                                                              \
			auto& Logger = NLib::NLogger::GetInstance();                                                               \
			std::string FormattedMessage = (Category)->GetLogPrefix() + " " + fmt::format(Format, ##__VA_ARGS__);      \
			Logger.Log(NLib::ELogLevel::Level, FormattedMessage);                                                      \
		}                                                                                                              \
	} while (0)

/**
 * @brief 条件日志宏
 * 用法: NLOG_IF(bCondition, LogCore, Info, "Message");
 */
#define NLOG_IF(Condition, Category, Level, Format, ...)                                                               \
	do                                                                                                                 \
	{                                                                                                                  \
		if ((Condition) && (Category) && (Category)->ShouldLog(NLib::ELogLevel::Level))                                \
		{                                                                                                              \
			auto& Logger = NLib::NLogger::GetInstance();                                                               \
			std::string FormattedMessage = (Category)->GetLogPrefix() + " " + fmt::format(Format, ##__VA_ARGS__);      \
			Logger.Log(NLib::ELogLevel::Level, FormattedMessage);                                                      \
		}                                                                                                              \
	} while (0)

/**
 * @brief 一次性日志宏 (只记录一次)
 * 用法: NLOG_ONCE(LogCore, Warning, "This will only appear once");
 */
#define NLOG_ONCE(Category, Level, Format, ...)                                                                        \
	do                                                                                                                 \
	{                                                                                                                  \
		static bool bLoggedOnce = false;                                                                               \
		if (!bLoggedOnce && (Category) && (Category)->ShouldLog(NLib::ELogLevel::Level))                               \
		{                                                                                                              \
			bLoggedOnce = true;                                                                                        \
			auto& Logger = NLib::NLogger::GetInstance();                                                               \
			std::string FormattedMessage = (Category)->GetLogPrefix() + " " + fmt::format(Format, ##__VA_ARGS__);      \
			Logger.Log(NLib::ELogLevel::Level, FormattedMessage);                                                      \
		}                                                                                                              \
	} while (0)

/**
 * @brief 限频日志宏 (每N次调用记录一次)
 * 用法: NLOG_THROTTLE(100, LogCore, Debug, "This appears every 100 calls");
 */
#define NLOG_THROTTLE(N, Category, Level, Format, ...)                                                                 \
	do                                                                                                                 \
	{                                                                                                                  \
		static int CallCount = 0;                                                                                      \
		if ((++CallCount % (N)) == 1 && (Category) && (Category)->ShouldLog(NLib::ELogLevel::Level))                   \
		{                                                                                                              \
			auto& Logger = NLib::NLogger::GetInstance();                                                               \
			std::string FormattedMessage = (Category)->GetLogPrefix() + " " + fmt::format(Format, ##__VA_ARGS__);      \
			Logger.Log(NLib::ELogLevel::Level, FormattedMessage);                                                      \
		}                                                                                                              \
	} while (0)

/**
 * @brief 声明自定义日志分类的宏
 * 用法: DECLARE_LOG_CATEGORY(LogMyModule, Info, false);
 */
#define DECLARE_LOG_CATEGORY(CategoryName, DefaultLevel, bVerbose)                                                     \
	extern std::shared_ptr<NLib::CLogCategory> CategoryName;

/**
 * @brief 定义自定义日志分类的宏
 * 用法: DEFINE_LOG_CATEGORY(LogMyModule, Info, false);
 */
#define DEFINE_LOG_CATEGORY(CategoryName, DefaultLevel, bVerbose)                                                      \
	std::shared_ptr<NLib::CLogCategory> CategoryName = NLib::CLogCategoryManager::GetInstance().GetOrCreateCategory(   \
	    #CategoryName, NLib::ELogLevel::DefaultLevel, bVerbose);

// === 便捷日志宏 ===

// 核心系统
#define NLOG_CORE(Level, Format, ...) NLOG(NLib::LogCore, Level, Format, ##__VA_ARGS__)
#define NLOG_MEMORY(Level, Format, ...) NLOG(NLib::LogMemory, Level, Format, ##__VA_ARGS__)
#define NLOG_GC(Level, Format, ...) NLOG(NLib::LogGC, Level, Format, ##__VA_ARGS__)
#define NLOG_THREADING(Level, Format, ...) NLOG(NLib::LogThreading, Level, Format, ##__VA_ARGS__)

// 功能模块
#define NLOG_NETWORK(Level, Format, ...) NLOG(NLib::LogNetwork, Level, Format, ##__VA_ARGS__)
#define NLOG_IO(Level, Format, ...) NLOG(NLib::LogIO, Level, Format, ##__VA_ARGS__)
#define NLOG_CONFIG(Level, Format, ...) NLOG(NLib::LogConfig, Level, Format, ##__VA_ARGS__)

// 调试和性能
#define NLOG_PERF(Level, Format, ...) NLOG(NLib::LogPerformance, Level, Format, ##__VA_ARGS__)
#define NLOG_DEBUG(Level, Format, ...) NLOG(NLib::LogDebug, Level, Format, ##__VA_ARGS__)
#define NLOG_VERBOSE(Level, Format, ...) NLOG(NLib::LogVerbose, Level, Format, ##__VA_ARGS__)
#define NLOG_EVENTS(Level, Format, ...) NLOG(NLib::LogEvents, Level, Format, ##__VA_ARGS__)
