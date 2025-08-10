#pragma once

#include "Logging/LogCategory.h"
#include "TContainer.h"

#include <algorithm>
#include <memory>
#include <utility>

namespace NLib
{
/**
 * @brief TArray - 动态数组容器
 *
 * 类似std::vector的高性能动态数组实现，专为Nut引擎优化
 * 特性：
 * - 使用tcmalloc内存管理
 * - 支持移动语义和完美转发
 * - 提供详细的调试和统计信息
 * - 线程安全的内存操作
 */
template <typename TElementType, typename TAllocator = CMemoryManager>
class TArray : public TSequenceContainer<TElementType, TAllocator>
{
public:
	using BaseType = TSequenceContainer<TElementType, TAllocator>;
	using typename BaseType::AllocatorType;
	using typename BaseType::ConstPointer;
	using typename BaseType::ConstReference;
	using typename BaseType::DifferenceType;
	using typename BaseType::ElementType;
	using typename BaseType::Pointer;
	using typename BaseType::Reference;
	using typename BaseType::SizeType;

	// 迭代器类型
	using Iterator = Pointer;
	using ConstIterator = ConstPointer;
	using ReverseIterator = std::reverse_iterator<Iterator>;
	using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

private:
	Pointer Data;             // 数据指针
	SizeType ArraySize;       // 当前元素数量
	SizeType ArrayCapacity;   // 当前容量
	AllocatorType* Allocator; // 内存分配器

public:
	// === 构造函数和析构函数 ===

	/**
	 * @brief 默认构造函数
	 */
	TArray()
	    : Data(nullptr),
	      ArraySize(0),
	      ArrayCapacity(0),
	      Allocator(&GetDefaultAllocator())
	{
		NLOG(LogCore, Debug, "TArray default constructed");
	}

	/**
	 * @brief 指定容量的构造函数
	 * @param InitialCapacity 初始容量
	 */
	explicit TArray(SizeType InitialCapacity)
	    : Data(nullptr),
	      ArraySize(0),
	      ArrayCapacity(0),
	      Allocator(&GetDefaultAllocator())
	{
		Reserve(InitialCapacity);
		NLOG(LogCore, Debug, "TArray constructed with capacity {}", InitialCapacity);
	}

	/**
	 * @brief 填充构造函数
	 * @param Count 元素数量
	 * @param Value 填充值
	 */
	TArray(SizeType Count, const ElementType& Value)
	    : TArray()
	{
		Resize(Count, Value);
		NLOG(LogCore, Debug, "TArray constructed with {} elements", Count);
	}

	/**
	 * @brief 初始化列表构造函数
	 * @param InitList 初始化列表
	 */
	TArray(std::initializer_list<ElementType> InitList)
	    : TArray()
	{
		Reserve(InitList.size());
		for (const auto& Element : InitList)
		{
			PushBack(Element);
		}
		NLOG(LogCore, Debug, "TArray constructed from initializer list with {} elements", InitList.size());
	}

	/**
	 * @brief 拷贝构造函数
	 * @param Other 要拷贝的数组
	 */
	TArray(const TArray& Other)
	    : Data(nullptr),
	      ArraySize(0),
	      ArrayCapacity(0),
	      Allocator(Other.Allocator)
	{
		if (Other.ArraySize > 0)
		{
			Reserve(Other.ArraySize);
			for (SizeType i = 0; i < Other.ArraySize; ++i)
			{
				new (Data + i) ElementType(Other.Data[i]);
			}
			ArraySize = Other.ArraySize;
		}
		NLOG(LogCore, Debug, "TArray copy constructed with {} elements", ArraySize);
	}

	/**
	 * @brief 移动构造函数
	 * @param Other 要移动的数组
	 */
	TArray(TArray&& Other) noexcept
	    : Data(Other.Data),
	      ArraySize(Other.ArraySize),
	      ArrayCapacity(Other.ArrayCapacity),
	      Allocator(Other.Allocator)
	{
		Other.Data = nullptr;
		Other.ArraySize = 0;
		Other.ArrayCapacity = 0;
		NLOG(LogCore, Debug, "TArray move constructed");
	}

	/**
	 * @brief 拷贝赋值操作符
	 * @param Other 要拷贝的数组
	 * @return 数组引用
	 */
	TArray& operator=(const TArray& Other)
	{
		if (this != &Other)
		{
			Clear();
			DeallocateData();
			
			Allocator = Other.Allocator;
			if (Other.ArraySize > 0)
			{
				Reserve(Other.ArraySize);
				for (SizeType i = 0; i < Other.ArraySize; ++i)
				{
					new (Data + i) ElementType(Other.Data[i]);
				}
				ArraySize = Other.ArraySize;
			}
			NLOG(LogCore, Debug, "TArray copy assigned with {} elements", ArraySize);
		}
		return *this;
	}

	/**
	 * @brief 移动赋值操作符
	 * @param Other 要移动的数组
	 * @return 数组引用
	 */
	TArray& operator=(TArray&& Other) noexcept
	{
		if (this != &Other)
		{
			Clear();
			DeallocateData();

			Data = Other.Data;
			ArraySize = Other.ArraySize;
			ArrayCapacity = Other.ArrayCapacity;
			Allocator = Other.Allocator;

			Other.Data = nullptr;
			Other.ArraySize = 0;
			Other.ArrayCapacity = 0;

			NLOG(LogCore, Debug, "TArray move assigned");
		}
		return *this;
	}

	/**
	 * @brief 析构函数
	 */
	~TArray()
	{
		Clear();
		DeallocateData();
		NLOG(LogCore, Debug, "TArray destroyed");
	}

public:
	// === TContainer接口实现 ===

	SizeType Size() const override
	{
		return ArraySize;
	}

	SizeType MaxSize() const override
	{
		return std::numeric_limits<SizeType>::max() / sizeof(ElementType);
	}

	void Clear() override
	{
		// 调用所有元素的析构函数
		for (SizeType i = 0; i < ArraySize; ++i)
		{
			Data[i].~ElementType();
		}
		ArraySize = 0;
		NLOG(LogCore, Debug, "TArray cleared");
	}

	/**
	 * @brief 清空数组（别名方法）
	 */
	void Empty()
	{
		Clear();
	}

	AllocatorType& GetAllocator() override
	{
		return *Allocator;
	}

	const AllocatorType& GetAllocator() const override
	{
		return *Allocator;
	}

	SizeType GetMemoryUsage() const override
	{
		return ArrayCapacity * sizeof(ElementType);
	}

	void ShrinkToFit() override
	{
		if (ArrayCapacity > ArraySize)
		{
			Reallocate(ArraySize);
			NLOG(LogCore, Debug, "TArray shrunk to fit {} elements", ArraySize);
		}
	}

	bool Equals(const TContainer<ElementType, TAllocator>& Other) const override
	{
		const TArray* OtherArray = dynamic_cast<const TArray*>(&Other);
		if (!OtherArray || OtherArray->Size() != Size())
		{
			return false;
		}

		return std::equal(begin(), end(), OtherArray->begin());
	}

private:
	// SFINAE 帮助器，检测类型是否可哈希
	template<typename T>
	static auto TestHashable(int) -> decltype(std::hash<T>{}(std::declval<T>()), std::true_type{});
	
	template<typename T>
	static std::false_type TestHashable(...);
	
	template<typename T>
	static constexpr bool IsHashable = decltype(TestHashable<T>(0))::value;

public:
	size_t GetHashCode() const override
	{
		size_t Hash = 0;
		for (SizeType i = 0; i < ArraySize; ++i)
		{
			if constexpr (IsHashable<ElementType>)
			{
				// 使用 std::hash
				Hash ^= std::hash<ElementType>{}(Data[i]) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
			}
			else
			{
				// 使用地址作为哈希
				Hash ^= reinterpret_cast<size_t>(&Data[i]) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
			}
		}
		return Hash;
	}

	bool Validate() const override
	{
		bool bValid = true;

		if (ArraySize > ArrayCapacity)
		{
			NLOG(LogCore, Error, "TArray validation failed: size {} > capacity {}", ArraySize, ArrayCapacity);
			bValid = false;
		}

		if (ArrayCapacity > 0 && Data == nullptr)
		{
			NLOG(LogCore, Error, "TArray validation failed: capacity {} but data is null", ArrayCapacity);
			bValid = false;
		}

		if (ArrayCapacity == 0 && Data != nullptr)
		{
			NLOG(LogCore, Error, "TArray validation failed: capacity is 0 but data is not null");
			bValid = false;
		}

		return bValid;
	}

	typename BaseType::SContainerStats GetStats() const override
	{
		typename BaseType::SContainerStats Stats;
		Stats.ElementCount = ArraySize;
		Stats.Capacity = ArrayCapacity;
		Stats.MemoryUsage = GetMemoryUsage();
		Stats.WastedMemory = (ArrayCapacity - ArraySize) * sizeof(ElementType);
		Stats.LoadFactor = ArrayCapacity > 0 ? static_cast<double>(ArraySize) / ArrayCapacity : 0.0;
		return Stats;
	}

public:
	// === TSequenceContainer接口实现 ===

	Reference At(SizeType Index) override
	{
		BaseType::CheckIndex(Index, ArraySize);
		return Data[Index];
	}

	ConstReference At(SizeType Index) const override
	{
		BaseType::CheckIndex(Index, ArraySize);
		return Data[Index];
	}

	Reference operator[](SizeType Index) override
	{
		return Data[Index];
	}

	ConstReference operator[](SizeType Index) const override
	{
		return Data[Index];
	}

	Reference Front() override
	{
		if (ArraySize == 0)
		{
			NLOG(LogCore, Error, "TArray::Front() called on empty array");
			throw std::runtime_error("Front() called on empty TArray");
		}
		return Data[0];
	}

	ConstReference Front() const override
	{
		if (ArraySize == 0)
		{
			NLOG(LogCore, Error, "TArray::Front() called on empty array");
			throw std::runtime_error("Front() called on empty TArray");
		}
		return Data[0];
	}

	Reference Back() override
	{
		if (ArraySize == 0)
		{
			NLOG(LogCore, Error, "TArray::Back() called on empty array");
			throw std::runtime_error("Back() called on empty TArray");
		}
		return Data[ArraySize - 1];
	}

	ConstReference Back() const override
	{
		if (ArraySize == 0)
		{
			NLOG(LogCore, Error, "TArray::Back() called on empty array");
			throw std::runtime_error("Back() called on empty TArray");
		}
		return Data[ArraySize - 1];
	}

	/**
	 * @brief 获取最后一个元素 (别名方法)
	 * @return 最后一个元素的引用
	 */
	Reference Last()
	{
		return Back();
	}

	/**
	 * @brief 获取最后一个元素 (别名方法，常量版本)
	 * @return 最后一个元素的常量引用
	 */
	ConstReference Last() const
	{
		return Back();
	}

	/**
	 * @brief 获取原始数据指针
	 * @return 原始数据指针
	 */
	Pointer GetData()
	{
		return Data;
	}

	/**
	 * @brief 获取原始数据指针（常量版本）
	 * @return 原始数据指针
	 */
	ConstPointer GetData() const
	{
		return Data;
	}

	void PushBack(const ElementType& Element) override
	{
		EnsureCapacity(ArraySize + 1);
		new (Data + ArraySize) ElementType(Element);
		++ArraySize;
	}

	void PushBack(ElementType&& Element) override
	{
		EnsureCapacity(ArraySize + 1);
		new (Data + ArraySize) ElementType(std::move(Element));
		++ArraySize;
	}

	void PopBack() override
	{
		if (ArraySize == 0)
		{
			NLOG(LogCore, Error, "TArray::PopBack() called on empty array");
			throw std::runtime_error("PopBack() called on empty TArray");
		}
		--ArraySize;
		Data[ArraySize].~ElementType();
	}

	void Insert(SizeType Index, const ElementType& Element) override
	{
		InsertInternal(Index, Element);
	}

	void Insert(SizeType Index, ElementType&& Element) override
	{
		InsertInternal(Index, std::move(Element));
	}

	void RemoveAt(SizeType Index) override
	{
		BaseType::CheckIndex(Index, ArraySize);

		// 调用要移除元素的析构函数
		Data[Index].~ElementType();

		// 移动后续元素
		for (SizeType i = Index; i < ArraySize - 1; ++i)
		{
			new (Data + i) ElementType(std::move(Data[i + 1]));
			Data[i + 1].~ElementType();
		}

		--ArraySize;
	}

	void RemoveRange(SizeType StartIndex, SizeType Count) override
	{
		if (Count == 0)
			return;

		BaseType::CheckIndex(StartIndex, ArraySize);
		SizeType EndIndex = std::min(StartIndex + Count, ArraySize);
		SizeType ActualCount = EndIndex - StartIndex;

		// 调用要移除元素的析构函数
		for (SizeType i = StartIndex; i < EndIndex; ++i)
		{
			Data[i].~ElementType();
		}

		// 移动后续元素
		SizeType MoveCount = ArraySize - EndIndex;
		for (SizeType i = 0; i < MoveCount; ++i)
		{
			new (Data + StartIndex + i) ElementType(std::move(Data[EndIndex + i]));
			Data[EndIndex + i].~ElementType();
		}

		ArraySize -= ActualCount;
	}

public:
	// === 容量管理 ===

	/**
	 * @brief 获取当前容量
	 * @return 当前容量
	 */
	SizeType Capacity() const
	{
		return ArrayCapacity;
	}

	/**
	 * @brief 预留指定容量
	 * @param NewCapacity 新容量
	 */
	void Reserve(SizeType NewCapacity)
	{
		if (NewCapacity > ArrayCapacity)
		{
			Reallocate(NewCapacity);
			NLOG(LogCore, Debug, "TArray reserved capacity {}", NewCapacity);
		}
	}

	/**
	 * @brief 调整数组大小
	 * @param NewSize 新大小
	 */
	void Resize(SizeType NewSize)
	{
		if (NewSize > ArraySize)
		{
			EnsureCapacity(NewSize);
			for (SizeType i = ArraySize; i < NewSize; ++i)
			{
				new (Data + i) ElementType();
			}
		}
		else if (NewSize < ArraySize)
		{
			for (SizeType i = NewSize; i < ArraySize; ++i)
			{
				Data[i].~ElementType();
			}
		}
		ArraySize = NewSize;
	}

	/**
	 * @brief 调整数组大小并填充指定值
	 * @param NewSize 新大小
	 * @param Value 填充值
	 */
	void Resize(SizeType NewSize, const ElementType& Value)
	{
		if (NewSize > ArraySize)
		{
			EnsureCapacity(NewSize);
			for (SizeType i = ArraySize; i < NewSize; ++i)
			{
				new (Data + i) ElementType(Value);
			}
		}
		else if (NewSize < ArraySize)
		{
			for (SizeType i = NewSize; i < ArraySize; ++i)
			{
				Data[i].~ElementType();
			}
		}
		ArraySize = NewSize;
	}

public:
	// === 迭代器支持 ===

	Iterator begin()
	{
		return Data;
	}
	ConstIterator begin() const
	{
		return Data;
	}
	ConstIterator cbegin() const
	{
		return Data;
	}

	Iterator end()
	{
		return Data + ArraySize;
	}
	ConstIterator end() const
	{
		return Data + ArraySize;
	}
	ConstIterator cend() const
	{
		return Data + ArraySize;
	}

	ReverseIterator rbegin()
	{
		return ReverseIterator(end());
	}
	ConstReverseIterator rbegin() const
	{
		return ConstReverseIterator(end());
	}
	ConstReverseIterator crbegin() const
	{
		return ConstReverseIterator(end());
	}

	ReverseIterator rend()
	{
		return ReverseIterator(begin());
	}
	ConstReverseIterator rend() const
	{
		return ConstReverseIterator(begin());
	}
	ConstReverseIterator crend() const
	{
		return ConstReverseIterator(begin());
	}

public:
	// === 高级操作 ===

	/**
	 * @brief 查找元素
	 * @param Element 要查找的元素
	 * @return 元素索引，未找到返回SIZE_MAX
	 */
	SizeType Find(const ElementType& Element) const
	{
		for (SizeType i = 0; i < ArraySize; ++i)
		{
			if (Data[i] == Element)
			{
				return i;
			}
		}
		return SIZE_MAX;
	}

	/**
	 * @brief 检查是否包含指定元素
	 * @param Element 要检查的元素
	 * @return true if contains, false otherwise
	 */
	bool Contains(const ElementType& Element) const
	{
		return Find(Element) != SIZE_MAX;
	}

	/**
	 * @brief 移除第一个匹配的元素
	 * @param Element 要移除的元素
	 * @return 是否成功移除
	 */
	bool Remove(const ElementType& Element)
	{
		SizeType Index = Find(Element);
		if (Index != SIZE_MAX)
		{
			RemoveAt(Index);
			return true;
		}
		return false;
	}

	/**
	 * @brief 移除所有匹配的元素
	 * @param Element 要移除的元素
	 * @return 移除的元素数量
	 */
	SizeType RemoveAll(const ElementType& Element)
	{
		SizeType RemovedCount = 0;
		for (SizeType i = 0; i < ArraySize;)
		{
			if (Data[i] == Element)
			{
				RemoveAt(i);
				++RemovedCount;
			}
			else
			{
				++i;
			}
		}
		return RemovedCount;
	}

	/**
	 * @brief 排序数组
	 */
	template <typename TComparator = std::less<ElementType>>
	void Sort(TComparator Comp = TComparator{})
	{
		std::sort(begin(), end(), Comp);
	}

	/**
	 * @brief 反转数组
	 */
	void Reverse()
	{
		std::reverse(begin(), end());
	}

protected:
	// === BaseType虚函数实现 ===

	Reference DoEmplaceBack() override
	{
		EnsureCapacity(ArraySize + 1);
		new (Data + ArraySize) ElementType();
		return Data[ArraySize++];
	}

private:
	// === 内部方法 ===

	/**
	 * @brief 获取默认分配器
	 * @return 分配器引用
	 */
	static AllocatorType& GetDefaultAllocator()
	{
		extern CMemoryManager& GetMemoryManager();
		return GetMemoryManager();
	}

	/**
	 * @brief 确保容量足够
	 * @param RequiredCapacity 所需容量
	 */
	void EnsureCapacity(SizeType RequiredCapacity)
	{
		if (RequiredCapacity > ArrayCapacity)
		{
			SizeType NewCapacity = BaseType::CalculateGrowth(ArrayCapacity, RequiredCapacity);
			Reallocate(NewCapacity);
		}
	}

	/**
	 * @brief 重新分配内存
	 * @param NewCapacity 新容量
	 */
	void Reallocate(SizeType NewCapacity)
	{
		if (NewCapacity == 0)
		{
			DeallocateData();
			Data = nullptr;
			ArrayCapacity = 0;
			return;
		}

		Pointer NewData = static_cast<Pointer>(Allocator->AllocateObject(NewCapacity * sizeof(ElementType)));
		if (!NewData)
		{
			NLOG(LogCore, Error, "TArray failed to allocate {} bytes", NewCapacity * sizeof(ElementType));
			throw std::bad_alloc();
		}

		// 移动现有元素到新内存
		for (SizeType i = 0; i < ArraySize; ++i)
		{
			new (NewData + i) ElementType(std::move(Data[i]));
			Data[i].~ElementType();
		}

		// 释放旧内存
		if (Data)
		{
			Allocator->DeallocateObject(Data);
		}

		Data = NewData;
		ArrayCapacity = NewCapacity;
	}

	/**
	 * @brief 释放数据内存
	 */
	void DeallocateData()
	{
		if (Data)
		{
			Allocator->DeallocateObject(Data);
			Data = nullptr;
			ArrayCapacity = 0;
		}
	}

	/**
	 * @brief 插入元素的内部实现
	 * @tparam TElementArg 元素类型
	 * @param Index 插入位置
	 * @param Element 要插入的元素
	 */
	template <typename TElementArg>
	void InsertInternal(SizeType Index, TElementArg&& Element)
	{
		if (Index > ArraySize)
		{
			BaseType::CheckIndex(Index, ArraySize + 1);
		}

		EnsureCapacity(ArraySize + 1);

		// 移动后续元素
		for (SizeType i = ArraySize; i > Index; --i)
		{
			new (Data + i) ElementType(std::move(Data[i - 1]));
			Data[i - 1].~ElementType();
		}

		// 插入新元素
		new (Data + Index) ElementType(std::forward<TElementArg>(Element));
		++ArraySize;
	}
};

// === 类型别名 ===

template <typename TElementType>
using TArrayPtr = TSharedPtr<TArray<TElementType>>;

// 常用类型的别名
using IntArray = TArray<int32_t>;
using FloatArray = TArray<float>;
using DoubleArray = TArray<double>;
using StringArray = TArray<CString>;
using ObjectArray = TArray<NObjectPtr>;

// Global equality operator for TArray
template <typename TElementType, typename TAllocator>
bool operator==(const TArray<TElementType, TAllocator>& Left, const TArray<TElementType, TAllocator>& Right)
{
	return Left.Equals(Right);
}

} // namespace NLib

// std::hash specialization for TArray
namespace std
{
template <typename TElementType, typename TAllocator>
struct hash<NLib::TArray<TElementType, TAllocator>>
{
	size_t operator()(const NLib::TArray<TElementType, TAllocator>& Array) const noexcept
	{
		return Array.GetHashCode();
	}
};
}