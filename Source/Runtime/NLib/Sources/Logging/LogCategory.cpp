#include "LogCategory.h"

#include "Logger.h"

#include <algorithm>
#include <iostream>

// 需要包含fmt库用于格式化
#include "spdlog/fmt/fmt.h"

namespace NLib
{
// === CLogCategory 实现 ===

CLogCategory::CLogCategory(const std::string& CategoryName, ELogLevel DefaultLevel, bool bVerboseLog)
    : CategoryName(CategoryName),
      CategoryLevel(DefaultLevel),
      bVerboseEnabled(bVerboseLog)
{}

void CLogCategory::SetLevel(ELogLevel Level)
{
	std::lock_guard<std::mutex> Lock(CategoryMutex);
	CategoryLevel = Level;
}

ELogLevel CLogCategory::GetLevel() const
{
	std::lock_guard<std::mutex> Lock(CategoryMutex);
	return CategoryLevel;
}

bool CLogCategory::ShouldLog(ELogLevel Level) const
{
	std::lock_guard<std::mutex> Lock(CategoryMutex);
	return Level >= CategoryLevel && Level != ELogLevel::Off;
}

void CLogCategory::SetVerbose(bool bEnable)
{
	std::lock_guard<std::mutex> Lock(CategoryMutex);
	bVerboseEnabled = bEnable;
}

bool CLogCategory::IsVerbose() const
{
	std::lock_guard<std::mutex> Lock(CategoryMutex);
	return bVerboseEnabled;
}

std::string CLogCategory::GetLogPrefix() const
{
	std::lock_guard<std::mutex> Lock(CategoryMutex);
	return "[" + CategoryName + "]";
}

// === CLogCategoryManager 实现 ===

CLogCategoryManager& CLogCategoryManager::GetInstance()
{
	static CLogCategoryManager Instance;
	return Instance;
}

bool CLogCategoryManager::RegisterCategory(std::shared_ptr<CLogCategory> Category)
{
	if (!Category)
	{
		return false;
	}

	std::lock_guard<std::mutex> Lock(CategoriesMutex);

	const std::string& CategoryName = Category->GetCategoryName();
	if (Categories.find(CategoryName) != Categories.end())
	{
		// 分类已存在
		return false;
	}

	Categories[CategoryName] = Category;
	return true;
}

std::shared_ptr<CLogCategory> CLogCategoryManager::GetCategory(const std::string& CategoryName)
{
	std::lock_guard<std::mutex> Lock(CategoriesMutex);

	auto It = Categories.find(CategoryName);
	if (It != Categories.end())
	{
		return It->second;
	}

	return nullptr;
}

std::shared_ptr<CLogCategory> CLogCategoryManager::GetOrCreateCategory(const std::string& CategoryName,
                                                                       ELogLevel DefaultLevel,
                                                                       bool bVerboseLog)
{
	std::lock_guard<std::mutex> Lock(CategoriesMutex);

	auto It = Categories.find(CategoryName);
	if (It != Categories.end())
	{
		return It->second;
	}

	// 创建新分类
	auto NewCategory = std::make_shared<CLogCategory>(CategoryName, DefaultLevel, bVerboseLog);
	Categories[CategoryName] = NewCategory;

	return NewCategory;
}

void CLogCategoryManager::SetAllCategoriesLevel(ELogLevel Level)
{
	std::lock_guard<std::mutex> Lock(CategoriesMutex);

	for (auto& Pair : Categories)
	{
		Pair.second->SetLevel(Level);
	}
}

std::vector<std::string> CLogCategoryManager::GetAllCategoryNames() const
{
	std::lock_guard<std::mutex> Lock(CategoriesMutex);

	std::vector<std::string> CategoryNames;
	CategoryNames.reserve(Categories.size());

	for (const auto& Pair : Categories)
	{
		CategoryNames.push_back(Pair.first);
	}

	std::sort(CategoryNames.begin(), CategoryNames.end());
	return CategoryNames;
}

void CLogCategoryManager::PrintCategoriesStatus() const
{
	std::lock_guard<std::mutex> Lock(CategoriesMutex);

	auto& Logger = NLogger::GetInstance();

	Logger.Info("=== Log Categories Status ===");
	Logger.InfoF("Total Categories: {}", Categories.size());

	for (const auto& Pair : Categories)
	{
		const auto& Category = Pair.second;
		std::string LevelName;

		switch (Category->GetLevel())
		{
		case ELogLevel::Trace:
			LevelName = "Trace";
			break;
		case ELogLevel::Debug:
			LevelName = "Debug";
			break;
		case ELogLevel::Info:
			LevelName = "Info";
			break;
		case ELogLevel::Warning:
			LevelName = "Warning";
			break;
		case ELogLevel::Error:
			LevelName = "Error";
			break;
		case ELogLevel::Critical:
			LevelName = "Critical";
			break;
		case ELogLevel::Off:
			LevelName = "Off";
			break;
		}

		Logger.InfoF(
		    "  {}: Level={}, Verbose={}", Category->GetCategoryName(), LevelName, Category->IsVerbose() ? "Yes" : "No");
	}

	Logger.Info("=============================");
}

// === 预定义的日志分类实现 ===

// 核心系统分类
std::shared_ptr<CLogCategory> LogCore;
std::shared_ptr<CLogCategory> LogMemory;
std::shared_ptr<CLogCategory> LogGC;
std::shared_ptr<CLogCategory> LogReflection;
std::shared_ptr<CLogCategory> LogSerialization;

// 功能模块分类
std::shared_ptr<CLogCategory> LogNetwork;
std::shared_ptr<CLogCategory> LogIO;
std::shared_ptr<CLogCategory> LogConfig;
std::shared_ptr<CLogCategory> LogEvents;
std::shared_ptr<CLogCategory> LogThreading;

// 调试和性能分类
std::shared_ptr<CLogCategory> LogPerformance;
std::shared_ptr<CLogCategory> LogDebug;
std::shared_ptr<CLogCategory> LogVerbose;

void InitializeDefaultLogCategories()
{
	auto& CategoryManager = CLogCategoryManager::GetInstance();

	// 核心系统分类
	LogCore = CategoryManager.GetOrCreateCategory("Core", ELogLevel::Info, false);
	LogMemory = CategoryManager.GetOrCreateCategory("Memory", ELogLevel::Info, false);
	LogGC = CategoryManager.GetOrCreateCategory("GC", ELogLevel::Info, false);
	LogReflection = CategoryManager.GetOrCreateCategory("Reflection", ELogLevel::Info, false);
	LogSerialization = CategoryManager.GetOrCreateCategory("Serialization", ELogLevel::Info, false);

	// 功能模块分类
	LogNetwork = CategoryManager.GetOrCreateCategory("Network", ELogLevel::Info, false);
	LogIO = CategoryManager.GetOrCreateCategory("IO", ELogLevel::Info, false);
	LogConfig = CategoryManager.GetOrCreateCategory("Config", ELogLevel::Info, false);
	LogEvents = CategoryManager.GetOrCreateCategory("Events", ELogLevel::Info, false);
	LogThreading = CategoryManager.GetOrCreateCategory("Threading", ELogLevel::Info, false);

	// 调试和性能分类
	LogPerformance = CategoryManager.GetOrCreateCategory("Performance", ELogLevel::Warning, false);
	LogDebug = CategoryManager.GetOrCreateCategory("Debug", ELogLevel::Debug, true);
	LogVerbose = CategoryManager.GetOrCreateCategory("Verbose", ELogLevel::Trace, true);

	auto& Logger = NLogger::GetInstance();
	Logger.Info("Default log categories initialized");
}

} // namespace NLib