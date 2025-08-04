#pragma once

/**
 * @file Containers.h
 * @brief Nut容器系统主头文件
 *
 * 包含所有容器类型的定义和类型别名
 */

// 容器基类
#include "TContainer.h"

// 具体容器实现
#include "TArray.h"
#include "THashMap.h"
#include "TString.h"

// 日志分类
#include "Logging/LogCategory.h"

namespace NLib
{
// === 容器统计信息 ===

/**
 * @brief 全局容器统计信息
 */
struct SContainerGlobalStats
{
	size_t TotalContainers = 0;  // 总容器数量
	size_t TotalMemoryUsage = 0; // 总内存使用
	size_t ArrayCount = 0;       // 数组数量
	size_t HashMapCount = 0;     // 哈希映射数量
	size_t StringCount = 0;      // 字符串数量

	void Reset()
	{
		TotalContainers = 0;
		TotalMemoryUsage = 0;
		ArrayCount = 0;
		HashMapCount = 0;
		StringCount = 0;
	}
};

/**
 * @brief 获取全局容器统计信息
 * @return 统计信息
 */
SContainerGlobalStats GetContainerGlobalStats();

/**
 * @brief 打印容器统计信息
 */
void LogContainerStats();

// === 常用容器类型别名 ===

// 基础类型数组
using BoolArray = TArray<bool>;
using ByteArray = TArray<uint8_t>;
using Int8Array = TArray<int8_t>;
using UInt8Array = TArray<uint8_t>;
using Int16Array = TArray<int16_t>;
using UInt16Array = TArray<uint16_t>;
using Int32Array = TArray<int32_t>;
using UInt32Array = TArray<uint32_t>;
using Int64Array = TArray<int64_t>;
using UInt64Array = TArray<uint64_t>;
using FloatArray = TArray<float>;
using DoubleArray = TArray<double>;

// 字符串数组
using StringArray = TArray<CString>;
using WStringArray = TArray<WString>;

// 对象数组
using ObjectPtrArray = TArray<NObjectPtr>;

// 基础类型映射
using StringToIntMap = THashMap<CString, int32_t>;
using StringToFloatMap = THashMap<CString, float>;
using StringToDoubleMap = THashMap<CString, double>;
using StringToBoolMap = THashMap<CString, bool>;
using StringToStringMap = THashMap<CString, CString>;

// 对象映射
using StringToObjectMap = THashMap<CString, NObjectPtr>;
using IntToObjectMap = THashMap<int32_t, NObjectPtr>;

// 反向映射
using IntToStringMap = THashMap<int32_t, CString>;
using FloatToStringMap = THashMap<float, CString>;

// === 容器工厂函数 ===

/**
 * @brief 创建数组
 * @tparam TElementType 元素类型
 * @param InitialCapacity 初始容量
 * @return 数组智能指针
 */
template <typename TElementType>
TSharedPtr<TArray<TElementType>> MakeArray(size_t InitialCapacity = 0)
{
	if (InitialCapacity > 0)
	{
		return NewNObject<TArray<TElementType>>(InitialCapacity);
	}
	else
	{
		return NewNObject<TArray<TElementType>>();
	}
}

/**
 * @brief 创建数组并填充初始值
 * @tparam TElementType 元素类型
 * @param Count 元素数量
 * @param Value 初始值
 * @return 数组智能指针
 */
template <typename TElementType>
TSharedPtr<TArray<TElementType>> MakeArray(size_t Count, const TElementType& Value)
{
	return NewNObject<TArray<TElementType>>(Count, Value);
}

/**
 * @brief 创建数组并使用初始化列表
 * @tparam TElementType 元素类型
 * @param InitList 初始化列表
 * @return 数组智能指针
 */
template <typename TElementType>
TSharedPtr<TArray<TElementType>> MakeArray(std::initializer_list<TElementType> InitList)
{
	return NewNObject<TArray<TElementType>>(InitList);
}

/**
 * @brief 创建哈希映射
 * @tparam TKeyType 键类型
 * @tparam TValueType 值类型
 * @param InitialBucketCount 初始桶数量
 * @return 哈希映射智能指针
 */
template <typename TKeyType, typename TValueType>
TSharedPtr<THashMap<TKeyType, TValueType>> MakeHashMap(size_t InitialBucketCount = 0)
{
	if (InitialBucketCount > 0)
	{
		return NewNObject<THashMap<TKeyType, TValueType>>(InitialBucketCount);
	}
	else
	{
		return NewNObject<THashMap<TKeyType, TValueType>>();
	}
}

/**
 * @brief 创建哈希映射并使用初始化列表
 * @tparam TKeyType 键类型
 * @tparam TValueType 值类型
 * @param InitList 初始化列表
 * @return 哈希映射智能指针
 */
template <typename TKeyType, typename TValueType>
TSharedPtr<THashMap<TKeyType, TValueType>> MakeHashMap(std::initializer_list<std::pair<TKeyType, TValueType>> InitList)
{
	return NewNObject<THashMap<TKeyType, TValueType>>(InitList);
}

/**
 * @brief 创建字符串
 * @param CStr C字符串
 * @return 字符串智能指针
 */
inline TSharedPtr<CString> MakeString(const char* CStr = nullptr)
{
	if (CStr)
	{
		return NewNObject<CString>(CStr);
	}
	else
	{
		return NewNObject<CString>();
	}
}

/**
 * @brief 创建字符串
 * @param StdStr std::string
 * @return 字符串智能指针
 */
inline TSharedPtr<CString> MakeString(const std::string& StdStr)
{
	return NewNObject<CString>(StdStr);
}

/**
 * @brief 创建字符串
 * @param Count 字符数量
 * @param Ch 字符
 * @return 字符串智能指针
 */
inline TSharedPtr<CString> MakeString(size_t Count, char Ch)
{
	return NewNObject<CString>(Count, Ch);
}

// === 容器算法 ===

/**
 * @brief 对数组进行排序
 * @tparam TElementType 元素类型
 * @tparam TComparator 比较器类型
 * @param Array 要排序的数组
 * @param Comparator 比较器
 */
template <typename TElementType, typename TComparator = std::less<TElementType>>
void SortArray(TArray<TElementType>& Array, TComparator Comparator = TComparator{})
{
	Array.Sort(Comparator);
}

/**
 * @brief 在数组中查找元素
 * @tparam TElementType 元素类型
 * @param Array 数组
 * @param Element 要查找的元素
 * @return 元素索引，未找到返回SIZE_MAX
 */
template <typename TElementType>
size_t FindInArray(const TArray<TElementType>& Array, const TElementType& Element)
{
	return Array.Find(Element);
}

/**
 * @brief 检查数组是否包含元素
 * @tparam TElementType 元素类型
 * @param Array 数组
 * @param Element 要检查的元素
 * @return true if contains, false otherwise
 */
template <typename TElementType>
bool ArrayContains(const TArray<TElementType>& Array, const TElementType& Element)
{
	return Array.Contains(Element);
}

/**
 * @brief 从数组中移除所有匹配的元素
 * @tparam TElementType 元素类型
 * @param Array 数组
 * @param Element 要移除的元素
 * @return 移除的元素数量
 */
template <typename TElementType>
size_t RemoveFromArray(TArray<TElementType>& Array, const TElementType& Element)
{
	return Array.RemoveAll(Element);
}

/**
 * @brief 合并两个数组
 * @tparam TElementType 元素类型
 * @param Target 目标数组
 * @param Source 源数组
 */
template <typename TElementType>
void MergeArrays(TArray<TElementType>& Target, const TArray<TElementType>& Source)
{
	for (size_t i = 0; i < Source.Size(); ++i)
	{
		Target.PushBack(Source[i]);
	}
}

/**
 * @brief 反转数组
 * @tparam TElementType 元素类型
 * @param Array 要反转的数组
 */
template <typename TElementType>
void ReverseArray(TArray<TElementType>& Array)
{
	Array.Reverse();
}

/**
 * @brief 复制数组
 * @tparam TElementType 元素类型
 * @param Source 源数组
 * @return 复制的数组
 */
template <typename TElementType>
TArray<TElementType> CopyArray(const TArray<TElementType>& Source)
{
	TArray<TElementType> Result;
	Result.Reserve(Source.Size());
	for (size_t i = 0; i < Source.Size(); ++i)
	{
		Result.PushBack(Source[i]);
	}
	return Result;
}

// === 字符串操作辅助函数 ===

/**
 * @brief 格式化字符串
 * @param Format 格式字符串
 * @param Args 参数
 * @return 格式化后的字符串
 */
template <typename... TArgs>
CString FormatString(const char* Format, TArgs&&... Args)
{
	// 简化实现，实际可以使用更高级的格式化库
	int Size = std::snprintf(nullptr, 0, Format, Args...);
	if (Size <= 0)
	{
		return CString();
	}

	CString Result;
	Result.Resize(Size);
	std::snprintf(const_cast<char*>(Result.CStr()), Size + 1, Format, Args...);
	return Result;
}

/**
 * @brief 连接字符串数组
 * @param Strings 字符串数组
 * @param Separator 分隔符
 * @return 连接后的字符串
 */
CString JoinStrings(const TArray<CString>& Strings, const CString& Separator = ",");

/**
 * @brief 分割字符串
 * @param Source 源字符串
 * @param Delimiter 分隔符
 * @return 分割后的字符串数组
 */
TArray<CString> SplitString(const CString& Source, const CString& Delimiter);

/**
 * @brief 去除字符串两端空白
 * @param Source 源字符串
 * @return 去除空白后的字符串
 */
CString TrimString(const CString& Source);

/**
 * @brief 转换为大写
 * @param Source 源字符串
 * @return 大写字符串
 */
CString ToUpperString(const CString& Source);

/**
 * @brief 转换为小写
 * @param Source 源字符串
 * @return 小写字符串
 */
CString ToLowerString(const CString& Source);

} // namespace NLib