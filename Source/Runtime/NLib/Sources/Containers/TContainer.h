#pragma once

#include "Core/Object.h"
#include "Logging/LogCategory.h"
#include "Memory/MemoryManager.h"

#include <algorithm>
#include <initializer_list>
#include <iterator>

namespace NLib
{
/**
 * @brief 容器迭代器类型
 */
enum class EIteratorType
{
	Forward,       // 正向迭代器
	Bidirectional, // 双向迭代器
	Random         // 随机访问迭代器
};

/**
 * @brief 容器分配器策略
 */
enum class EAllocatorPolicy
{
	Default, // 默认分配策略
	Pool,    // 池分配策略
	Stack,   // 栈分配策略
	Custom   // 自定义分配策略
};

/**
 * @brief 容器基类接口
 *
 * 所有Nut容器的基础接口，提供统一的容器操作
 */
template <typename TElementType, typename TAllocator = CMemoryManager>
class TContainer
{
public:
	// === 类型定义 ===
	using ElementType = TElementType;
	using SizeType = size_t;
	using DifferenceType = ptrdiff_t;
	using Reference = TElementType&;
	using ConstReference = const TElementType&;
	using Pointer = TElementType*;
	using ConstPointer = const TElementType*;
	using AllocatorType = TAllocator;

public:
	// === 构造函数和析构函数 ===

	/**
	 * @brief 默认构造函数
	 */
	TContainer() = default;

	/**
	 * @brief 析构函数
	 */
	virtual ~TContainer() = default;

	// 禁用拷贝构造，容器应该显式拷贝或移动
	TContainer(const TContainer&) = delete;
	TContainer& operator=(const TContainer&) = delete;

	// 允许移动语义
	TContainer(TContainer&& Other) noexcept = default;
	TContainer& operator=(TContainer&& Other) noexcept = default;

public:
	// === 容量相关 ===

	/**
	 * @brief 获取容器元素数量
	 * @return 元素数量
	 */
	virtual SizeType Size() const = 0;

	/**
	 * @brief 检查容器是否为空
	 * @return true if empty, false otherwise
	 */
	virtual bool IsEmpty() const
	{
		return Size() == 0;
	}

	/**
	 * @brief 获取容器最大容量
	 * @return 最大容量
	 */
	virtual SizeType MaxSize() const = 0;

	/**
	 * @brief 清空容器
	 */
	virtual void Clear() = 0;

public:
	// === 内存管理 ===

	/**
	 * @brief 获取分配器实例
	 * @return 分配器引用
	 */
	virtual AllocatorType& GetAllocator() = 0;

	/**
	 * @brief 获取分配器实例（常量版本）
	 * @return 分配器常量引用
	 */
	virtual const AllocatorType& GetAllocator() const = 0;

	/**
	 * @brief 获取容器使用的内存字节数
	 * @return 内存使用量
	 */
	virtual SizeType GetMemoryUsage() const = 0;

	/**
	 * @brief 收缩容器以释放未使用的内存
	 */
	virtual void ShrinkToFit() = 0;

public:
	// === 比较操作 ===

	/**
	 * @brief 比较两个容器是否相等
	 * @param Other 另一个容器
	 * @return true if equal, false otherwise
	 */
	virtual bool Equals(const TContainer& Other) const = 0;

	/**
	 * @brief 获取容器的哈希值
	 * @return 哈希值
	 */
	virtual size_t GetHashCode() const = 0;

public:
	// === 调试和诊断 ===

	/**
	 * @brief 验证容器的内部状态
	 * @return true if valid, false otherwise
	 */
	virtual bool Validate() const = 0;

	/**
	 * @brief 获取容器的统计信息
	 */
	struct SContainerStats
	{
		SizeType ElementCount = 0;
		SizeType Capacity = 0;
		SizeType MemoryUsage = 0;
		SizeType WastedMemory = 0;
		double LoadFactor = 0.0;
	};

	virtual SContainerStats GetStats() const = 0;

	/**
	 * @brief 输出容器的调试信息
	 */
	virtual void LogDebugInfo() const
	{
		auto Stats = GetStats();
		NLOG(LogCore,
		     Debug,
		     "Container Stats - Elements: {}, Capacity: {}, Memory: {} bytes",
		     Stats.ElementCount,
		     Stats.Capacity,
		     Stats.MemoryUsage);
	}

protected:
	// === 辅助方法 ===

	/**
	 * @brief 计算容器增长的新容量
	 * @param CurrentCapacity 当前容量
	 * @param MinimumCapacity 最小所需容量
	 * @return 新容量
	 */
	static SizeType CalculateGrowth(SizeType CurrentCapacity, SizeType MinimumCapacity)
	{
		// 采用1.5倍增长策略，避免内存浪费
		SizeType NewCapacity = CurrentCapacity + (CurrentCapacity >> 1);
		return std::max(NewCapacity, MinimumCapacity);
	}

	/**
	 * @brief 检查索引是否有效
	 * @param Index 索引
	 * @param Size 容器大小
	 * @return true if valid, false otherwise
	 */
	static bool IsValidIndex(SizeType Index, SizeType Size)
	{
		return Index < Size;
	}

	/**
	 * @brief 断言索引有效性
	 * @param Index 索引
	 * @param Size 容器大小
	 */
	static void CheckIndex(SizeType Index, SizeType Size)
	{
		if (!IsValidIndex(Index, Size))
		{
			NLOG(LogCore, Error, "Container index out of bounds: {} >= {}", Index, Size);
			throw std::out_of_range("Container index out of bounds");
		}
	}
};

/**
 * @brief 序列容器基类
 *
 * 为数组、链表等序列容器提供统一接口
 */
template <typename TElementType, typename TAllocator = CMemoryManager>
class TSequenceContainer : public TContainer<TElementType, TAllocator>
{
public:
	using BaseType = TContainer<TElementType, TAllocator>;
	using typename BaseType::ConstReference;
	using typename BaseType::ElementType;
	using typename BaseType::Reference;
	using typename BaseType::SizeType;

public:
	// === 元素访问 ===

	/**
	 * @brief 访问指定位置的元素（带边界检查）
	 * @param Index 索引
	 * @return 元素引用
	 */
	virtual Reference At(SizeType Index) = 0;
	virtual ConstReference At(SizeType Index) const = 0;

	/**
	 * @brief 访问指定位置的元素（不做边界检查）
	 * @param Index 索引
	 * @return 元素引用
	 */
	virtual Reference operator[](SizeType Index) = 0;
	virtual ConstReference operator[](SizeType Index) const = 0;

	/**
	 * @brief 访问第一个元素
	 * @return 第一个元素引用
	 */
	virtual Reference Front() = 0;
	virtual ConstReference Front() const = 0;

	/**
	 * @brief 访问最后一个元素
	 * @return 最后一个元素引用
	 */
	virtual Reference Back() = 0;
	virtual ConstReference Back() const = 0;

public:
	// === 修改操作 ===

	/**
	 * @brief 在末尾添加元素
	 * @param Element 要添加的元素
	 */
	virtual void PushBack(const ElementType& Element) = 0;
	virtual void PushBack(ElementType&& Element) = 0;

	/**
	 * @brief 在末尾构造元素
	 * @tparam TArgs 构造参数类型
	 * @param Args 构造参数
	 * @return 新构造元素的引用
	 */
	template <typename... TArgs>
	Reference EmplaceBack(TArgs&&... Args)
	{
		return DoEmplaceBack(std::forward<TArgs>(Args)...);
	}

	/**
	 * @brief 移除最后一个元素
	 */
	virtual void PopBack() = 0;

	/**
	 * @brief 在指定位置插入元素
	 * @param Index 插入位置
	 * @param Element 要插入的元素
	 */
	virtual void Insert(SizeType Index, const ElementType& Element) = 0;
	virtual void Insert(SizeType Index, ElementType&& Element) = 0;

	/**
	 * @brief 移除指定位置的元素
	 * @param Index 要移除的位置
	 */
	virtual void RemoveAt(SizeType Index) = 0;

	/**
	 * @brief 移除指定范围的元素
	 * @param StartIndex 开始位置
	 * @param Count 要移除的元素数量
	 */
	virtual void RemoveRange(SizeType StartIndex, SizeType Count) = 0;

protected:
	// === 子类实现接口 ===
	virtual Reference DoEmplaceBack() = 0;
};

/**
 * @brief 关联容器基类
 *
 * 为映射、集合等关联容器提供统一接口
 */
template <typename TKeyType, typename TValueType, typename TAllocator = CMemoryManager>
class TAssociativeContainer : public TContainer<std::pair<TKeyType, TValueType>, TAllocator>
{
public:
	using KeyType = TKeyType;
	using ValueType = TValueType;
	using PairType = std::pair<TKeyType, TValueType>;
	using BaseType = TContainer<PairType, TAllocator>;
	using typename BaseType::SizeType;

public:
	// === 查找操作 ===

	/**
	 * @brief 查找指定键的元素
	 * @param Key 要查找的键
	 * @return 是否找到
	 */
	virtual bool Contains(const KeyType& Key) const = 0;

	/**
	 * @brief 查找指定键对应的值
	 * @param Key 要查找的键
	 * @return 值的指针，未找到返回nullptr
	 */
	virtual ValueType* Find(const KeyType& Key) = 0;
	virtual const ValueType* Find(const KeyType& Key) const = 0;

	/**
	 * @brief 获取指定键的值计数
	 * @param Key 要查找的键
	 * @return 值的数量
	 */
	virtual SizeType Count(const KeyType& Key) const = 0;

public:
	// === 修改操作 ===

	/**
	 * @brief 插入键值对
	 * @param Key 键
	 * @param Value 值
	 * @return 是否成功插入
	 */
	virtual bool Insert(const KeyType& Key, const ValueType& Value) = 0;
	virtual bool Insert(const KeyType& Key, ValueType&& Value) = 0;

	/**
	 * @brief 移除指定键的元素
	 * @param Key 要移除的键
	 * @return 移除的元素数量
	 */
	virtual SizeType Remove(const KeyType& Key) = 0;
};

} // namespace NLib