#pragma once

#include "Containers/TArray.h"
#include "Containers/TString.h"
#include "Core/Object.h"
#include "Core/SmartPointers.h"
#include "Logging/LogCategory.h"

#include <filesystem>

#include "Path.generate.h"

namespace NLib
{
/**
 * @brief 路径分隔符常量
 */
struct FPathConstants
{
#if defined(_WIN32) || defined(_WIN64)
	static constexpr char DirectorySeparator = '\\';
	static constexpr char AltDirectorySeparator = '/';
	static constexpr char VolumeSeparator = ':';
	static constexpr const char* DirectorySeparatorString = "\\";
	static constexpr const char* AltDirectorySeparatorString = "/";
#else
	static constexpr char DirectorySeparator = '/';
	static constexpr char AltDirectorySeparator = '/';
	static constexpr char VolumeSeparator = '\0';
	static constexpr const char* DirectorySeparatorString = "/";
	static constexpr const char* AltDirectorySeparatorString = "/";
#endif
	static constexpr const char* CurrentDirectory = ".";
	static constexpr const char* ParentDirectory = "..";
};

/**
 * @brief 路径工具类
 *
 * 提供跨平台的路径操作功能：
 * - 路径标准化和转换
 * - 路径拼接和分解
 * - 相对路径和绝对路径转换
 * - 路径比较和验证
 * - 文件扩展名操作
 */
NCLASS()
class NPath : public NObject
{
	GENERATED_BODY()

public:
	// === 构造函数 ===

	NPath();
	explicit NPath(const CString& InPath);
	explicit NPath(const char* InPath);
	NPath(const NPath& Other);
	NPath(NPath&& Other) noexcept;

	~NPath() = default;

public:
	// === 赋值操作符 ===

	NPath& operator=(const NPath& Other);
	NPath& operator=(NPath&& Other) noexcept;
	NPath& operator=(const CString& InPath);
	NPath& operator=(const char* InPath);

public:
	// === 比较操作符 ===

	bool operator==(const NPath& Other) const;
	bool operator!=(const NPath& Other) const;
	bool operator<(const NPath& Other) const;

public:
	// === 路径拼接操作符 ===

	NPath operator/(const NPath& Other) const;
	NPath operator/(const CString& Other) const;
	NPath operator/(const char* Other) const;

	NPath& operator/=(const NPath& Other);
	NPath& operator/=(const CString& Other);
	NPath& operator/=(const char* Other);

public:
	// === 基本属性 ===

	/**
	 * @brief 获取路径字符串
	 */
	CString ToString() const override
	{
		return PathString;
	}

	/**
	 * @brief 获取C风格字符串
	 */
	const char* GetData() const
	{
		return PathString.GetData();
	}

	/**
	 * @brief 检查路径是否为空
	 */
	bool IsEmpty() const
	{
		return PathString.IsEmpty();
	}

	/**
	 * @brief 清空路径
	 */
	void Clear()
	{
		PathString.Clear();
	}

	/**
	 * @brief 获取路径长度
	 */
	int32_t Length() const
	{
		return static_cast<int32_t>(PathString.Size());
	}

public:
	// === 路径解析 ===

	/**
	 * @brief 获取文件名（包含扩展名）
	 */
	CString GetFileName() const;

	/**
	 * @brief 获取文件名（不包含扩展名）
	 */
	CString GetFileNameWithoutExtension() const;

	/**
	 * @brief 获取文件扩展名
	 */
	CString GetExtension() const;

	/**
	 * @brief 获取目录路径
	 */
	NPath GetDirectoryName() const;

	/**
	 * @brief 获取根目录
	 */
	NPath GetRoot() const;

	/**
	 * @brief 获取所有路径组件
	 */
	TArray<CString, CMemoryManager> GetComponents() const;

public:
	// === 路径操作 ===

	/**
	 * @brief 标准化路径（替换分隔符、处理..和.）
	 */
	NPath& Normalize();

	/**
	 * @brief 获取标准化的路径
	 */
	NPath GetNormalized() const;

	/**
	 * @brief 转换为绝对路径
	 */
	NPath& MakeAbsolute();

	/**
	 * @brief 获取绝对路径
	 */
	NPath GetAbsolute() const;

	/**
	 * @brief 转换为相对路径
	 */
	NPath GetRelative(const NPath& BasePath) const;

	/**
	 * @brief 改变扩展名
	 */
	NPath& ChangeExtension(const CString& NewExtension);

	/**
	 * @brief 获取改变扩展名后的路径
	 */
	NPath WithExtension(const CString& NewExtension) const;

public:
	// === 路径检查 ===

	/**
	 * @brief 检查是否是绝对路径
	 */
	bool IsAbsolute() const;

	/**
	 * @brief 检查是否是相对路径
	 */
	bool IsRelative() const;

	/**
	 * @brief 检查是否只是文件名
	 */
	bool IsFileName() const;

	/**
	 * @brief 检查路径是否有效
	 */
	bool IsValid() const;

	/**
	 * @brief 检查是否包含无效字符
	 */
	bool HasInvalidCharacters() const;

public:
	// === 静态工具方法 ===

	/**
	 * @brief 拼接多个路径
	 */
	template <typename... TArgs>
	static NPath Combine(const TArgs&... Args)
	{
		NPath Result;
		CombineImpl(Result, Args...);
		return Result;
	}

	/**
	 * @brief 获取当前工作目录
	 */
	static NPath GetCurrentDirectory();

	/**
	 * @brief 获取临时目录
	 */
	static NPath GetTempDirectory();

	/**
	 * @brief 获取用户目录
	 */
	static NPath GetUserDirectory();

	/**
	 * @brief 获取应用程序目录
	 */
	static NPath GetApplicationDirectory();

	/**
	 * @brief 检查字符是否是路径分隔符
	 */
	static bool IsSeparator(char Ch);

	/**
	 * @brief 检查字符是否是无效的路径字符
	 */
	static bool IsInvalidPathChar(char Ch);

	/**
	 * @brief 标准化路径分隔符
	 */
	static CString NormalizeSeparators(const CString& Path);

	/**
	 * @brief 获取两个路径的公共前缀
	 */
	static NPath GetCommonPrefix(const NPath& Path1, const NPath& Path2);

private:
	// === 内部实现 ===

	/**
	 * @brief 递归拼接路径的内部实现
	 */
	template <typename TFirst, typename... TRest>
	static void CombineImpl(NPath& Result, const TFirst& First, const TRest&... Rest)
	{
		Result /= First;
		if constexpr (sizeof...(Rest) > 0)
		{
			CombineImpl(Result, Rest...);
		}
	}

	/**
	 * @brief 单参数拼接的终止条件
	 */
	static void CombineImpl(NPath& Result)
	{
		// 空实现，用于递归终止
	}

	/**
	 * @brief 分割路径字符串
	 */
	TArray<CString, CMemoryManager> SplitPath() const;

	/**
	 * @brief 从std::filesystem::path转换
	 */
	void FromStdPath(const std::filesystem::path& StdPath);

	/**
	 * @brief 转换为std::filesystem::path
	 */
	std::filesystem::path ToStdPath() const;

private:
	CString PathString; // 路径字符串
};

// === 全局操作符 ===

/**
 * @brief 字符串与路径的拼接
 */
inline NPath operator/(const CString& Left, const NPath& Right)
{
	return NPath(Left) / Right;
}

/**
 * @brief C字符串与路径的拼接
 */
inline NPath operator/(const char* Left, const NPath& Right)
{
	return NPath(Left) / Right;
}

// === 便捷函数 ===

/**
 * @brief 创建路径的便捷函数
 */
inline NPath MakePath(const CString& PathStr)
{
	return NPath(PathStr);
}

/**
 * @brief 拼接路径的便捷函数
 */
template <typename... TArgs>
inline NPath JoinPaths(const TArgs&... Args)
{
	return NPath::Combine(Args...);
}

// === 类型别名 ===
using FPath = NPath;
using FPathConstants = FPathConstants;

} // namespace NLib