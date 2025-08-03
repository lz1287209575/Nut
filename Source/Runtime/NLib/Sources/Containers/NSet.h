#pragma once

#include "Logging/CLogger.h"
#include "CContainer.h"
#include "NLibMacros.h"

#include <initializer_list>
#include <iterator>
#include <utility>

namespace NLib
{

// 前向声明
template <typename TType>
class CArray;
class CGarbageCollector;

template <typename TType>
NCLASS()
class LIBNLIB_API CSet : public CContainer
{
	GENERATED_BODY()

public:
	using ValueType = T;
	using Reference = const T&;
	using ConstReference = const T&;
	using Pointer = const T*;
	using ConstPointer = const T*;

	// 基于哈希表的桶结构 (类似NHashMap但只存储值)
	struct Bucket
	{
		T Value;
		size_t Distance; // Robin Hood哈希距离
		bool bOccupied;  // 是否被占用

		Bucket()
		    : Distance(0),
		      bOccupied(false)
		{}
	};

	class Iterator {
public:
    virtual ~Iterator() = default;
    virtual void Next() = 0;
    virtual bool HasNext() const = 0;
				++Index;
			}
		}

		const T& operator*() const
		{
			return Buckets[Index].Value;
		}
		const T* operator->() const
		{
			return &Buckets[Index].Value;
		}

		Iterator& operator++()
		{
			++Index;
			while (Index < Capacity && !Buckets[Index].bOccupied)
			{
				++Index;
			}
			return *this;
		}

		Iterator operator++(int)
		{
			Iterator temp(*this);
			++(*this);
			return temp;
		}

		bool operator==(const Iterator& Other) const
		{
			return Index == Other.Index;
		}

		bool operator!=(const Iterator& Other) const
		{
			return Index != Other.Index;
		}

	private:
		Bucket* Buckets;
		size_t Index;
		size_t Capacity;

		friend class CSet<T>;
	};

	using ConstIterator = Iterator; // NSet的迭代器总是const的

	// 构造函数
	CSet();
	explicit CSet(size_t InitialCapacity);
	CSet(std::initializer_list<T> InitList);
	template <typename TInputIterator>
	CSet(InputIterator First, InputIterator Last);
	CSet(const CSet& Other);
	CSet(CSet&& Other) noexcept;

	// 析构函数
	virtual ~CSet();

	// 赋值操作符
	CSet& operator=(const CSet& Other);
	CSet& operator=(CSet&& Other) noexcept;
	CSet& operator=(std::initializer_list<T> InitList);

	// 容器接口实现
	size_t GetSize() const override
	{
		return Size;
	}
	size_t GetCapacity() const override
	{
		return Capacity;
	}
	bool IsEmpty() const override
	{
		return Size == 0;
	}
	void Clear() override;

	// 容量管理
	void Reserve(size_t NewCapacity);
	void Rehash(size_t BucketCount);
	float GetLoadFactor() const
	{
		return Capacity > 0 ? static_cast<float>(Size) / Capacity : 0.0F;
	}
	float GetMaxLoadFactor() const
	{
		return MaxLoadFactor;
	}
	void SetMaxLoadFactor(float Factor)
	{
		MaxLoadFactor = Factor;
	}

	// 元素操作
	std::pair<Iterator, bool> Insert(const T& Value);
	std::pair<Iterator, bool> Insert(T&& Value);

	template <typename... TArgs>
	std::pair<Iterator, bool> Emplace(TArgs&&... Args);

	bool Erase(const T& Value);
	Iterator Erase(Iterator Pos);

	// 查找操作
	Iterator Find(const T& Value);
	ConstIterator Find(const T& Value) const;

	bool Contains(const T& Value) const;
	size_t Count(const T& Value) const; // 对于Set总是返回0或1

	// 集合操作
	CSet Union(const CSet& Other) const;
	CSet Intersection(const CSet& Other) const;
	CSet Difference(const CSet& Other) const;
	CSet SymmetricDifference(const CSet& Other) const;

	bool IsSubsetOf(const CSet& Other) const;
	bool IsSupersetOf(const CSet& Other) const;
	bool IsDisjointWith(const CSet& Other) const;

	// 批量操作
	void Merge(const CSet& Other);
	void Merge(CSet&& Other);

	template <typename TInputIterator>
	void Insert(InputIterator First, InputIterator Last);

	// 转换为数组
	CArray<T> ToArray() const;

	// 迭代器
	Iterator Begin()
	{
		return Iterator(Buckets, 0, Capacity);
	}
	Iterator End()
	{
		return Iterator(Buckets, Capacity, Capacity);
	}
	ConstIterator Begin() const
	{
		return ConstIterator(Buckets, 0, Capacity);
	}
	ConstIterator End() const
	{
		return ConstIterator(Buckets, Capacity, Capacity);
	}
	ConstIterator CBegin() const
	{
		return ConstIterator(Buckets, 0, Capacity);
	}
	ConstIterator CEnd() const
	{
		return ConstIterator(Buckets, Capacity, Capacity);
	}

	// C++ range-based for loop support (lowercase methods)
	Iterator begin()
	{
		return Begin();
	}
	Iterator end()
	{
		return End();
	}
	ConstIterator begin() const
	{
		return Begin();
	}
	ConstIterator end() const
	{
		return End();
	}

	// 比较操作符
	bool operator==(const CSet& Other) const;
	bool operator!=(const CSet& Other) const;

	// CObject 接口重写
	virtual bool Equals(const CObject* Other) const override;
	virtual size_t GetHashCode() const override;
	virtual CString ToString() const override;

private:
	Bucket* Buckets;
	size_t Size;
	size_t Capacity;
	float MaxLoadFactor;

	static constexpr size_t DEFAULT_CAPACITY = 16;
	static constexpr float DEFAULT_MAX_LOAD_FACTOR = 0.75f;

	// 内部工具函数
	size_t Hash(const T& Value) const;
	size_t FindBucket(const T& Value) const;
	void RobinHoodInsert(T&& Value);
	void ResizeIfNeeded();
	void Resize(size_t NewCapacity);
	void InitializeBuckets();
	void DeallocateBuckets();
	void MoveFrom(CSet&& Other) noexcept;
	void CopyFrom(const CSet& Other);

	// Robin Hood 相关
	size_t CalculateDistance(size_t HashValue, size_t ActualPos) const;
	void ShiftBackward(size_t Pos);
};

// 模板实现
template <typename TType>
CSet<T>::CSet()
    : Buckets(nullptr),
      Size(0),
      Capacity(0),
      MaxLoadFactor(DEFAULT_MAX_LOAD_FACTOR)
{}

template <typename TType>
CSet<T>::CSet(size_t InitialCapacity)
    : Buckets(nullptr),
      Size(0),
      Capacity(0),
      MaxLoadFactor(DEFAULT_MAX_LOAD_FACTOR)
{
	if (InitialCapacity > 0)
	{
		// 确保容量是2的幂次
		size_t ActualCapacity = 1;
		while (ActualCapacity < InitialCapacity)
		{
			ActualCapacity <<= 1;
		}

		Capacity = ActualCapacity;
		InitializeBuckets();
	}
}

template <typename TType>
CSet<T>::CSet(std::initializer_list<T> InitList)
    : Buckets(nullptr),
      Size(0),
      Capacity(0),
      MaxLoadFactor(DEFAULT_MAX_LOAD_FACTOR)
{
	if (InitList.size() > 0)
	{
		size_t RequiredCapacity = static_cast<size_t>(InitList.size() / MaxLoadFactor) + 1;
		size_t ActualCapacity = DEFAULT_CAPACITY;
		while (ActualCapacity < RequiredCapacity)
		{
			ActualCapacity <<= 1;
		}

		Capacity = ActualCapacity;
		InitializeBuckets();

		for (const auto& Value : InitList)
		{
			Insert(Value);
		}
	}
}

template <typename TType>
template <typename TInputIterator>
CSet<T>::CSet(InputIterator First, InputIterator Last)
    : Buckets(nullptr),
      Size(0),
      Capacity(0),
      MaxLoadFactor(DEFAULT_MAX_LOAD_FACTOR)
{
	size_t Count = std::distance(First, Last);
	if (Count > 0)
	{
		size_t RequiredCapacity = static_cast<size_t>(Count / MaxLoadFactor) + 1;
		size_t ActualCapacity = DEFAULT_CAPACITY;
		while (ActualCapacity < RequiredCapacity)
		{
			ActualCapacity <<= 1;
		}

		Capacity = ActualCapacity;
		InitializeBuckets();

		for (auto It = First; It != Last; ++It)
		{
			Insert(*It);
		}
	}
}

template <typename TType>
CSet<T>::CSet(const CSet& Other)
    : Buckets(nullptr),
      Size(0),
      Capacity(0),
      MaxLoadFactor(DEFAULT_MAX_LOAD_FACTOR)
{
	CopyFrom(Other);
}

template <typename TType>
CSet<T>::CSet(CSet&& Other) noexcept
    : Buckets(nullptr),
      Size(0),
      Capacity(0),
      MaxLoadFactor(DEFAULT_MAX_LOAD_FACTOR)
{
	MoveFrom(std::move(Other));
}

template <typename TType>
CSet<T>::~CSet()
{
	DeallocateBuckets();
}

template <typename TType>
CSet<T>& CSet<T>::operator=(const CSet& Other)
{
	if (this != &Other)
	{
		CopyFrom(Other);
	}
	return *this;
}

template <typename TType>
CSet<T>& CSet<T>::operator=(CSet&& Other) noexcept
{
	if (this != &Other)
	{
		DeallocateBuckets();
		MoveFrom(std::move(Other));
	}
	return *this;
}

template <typename TType>
CSet<T>& CSet<T>::operator=(std::initializer_list<T> InitList)
{
	Clear();
	for (const auto& Value : InitList)
	{
		Insert(Value);
	}
	return *this;
}

template <typename TType>
void CSet<T>::Clear()
{
	if (Buckets)
	{
		for (size_t i = 0; i < Capacity; ++i)
		{
			if (Buckets[i].bOccupied)
			{
				// GC注销将在.cpp文件中实现以避免循环依赖

				Buckets[i].Value.~T();
				Buckets[i].bOccupied = false;
				Buckets[i].Distance = 0;
			}
		}
		Size = 0;
	}
}

template <typename TType>
std::pair<typename CSet<T>::Iterator, bool> CSet<T>::Insert(const T& Value)
{
	return Insert(T(Value));
}

template <typename TType>
std::pair<typename CSet<T>::Iterator, bool> CSet<T>::Insert(T&& Value)
{
	ResizeIfNeeded();

	size_t pos = FindBucket(Value);

	if (pos != Capacity && Buckets[pos].bOccupied && NEqual<T>{}(Buckets[pos].Value, Value))
	{
		// 值已存在
		return std::make_pair(Iterator(Buckets, pos, Capacity), false);
	}

	// 插入新元素
	T ValueForSearch = Value; // 保存副本用于后续查找
	RobinHoodInsert(std::move(Value));
	++Size;

	// 查找插入后的位置
	pos = FindBucket(ValueForSearch);
	return std::make_pair(Iterator(Buckets, pos, Capacity), true);
}

template <typename TType>
template <typename... Args>
std::pair<typename CSet<T>::Iterator, bool> CSet<T>::Emplace(Args&&... args)
{
	T Value(std::forward<Args>(args)...);
	return Insert(std::move(Value));
}

template <typename TType>
bool CSet<T>::Erase(const T& Value)
{
	size_t pos = FindBucket(Value);

	if (pos == Capacity || !Buckets[pos].bOccupied || !NEqual<T>{}(Buckets[pos].Value, Value))
	{
		return false;
	}

	// GC注销将在.cpp文件中实现以避免循环依赖

	Buckets[pos].Value.~T();
	Buckets[pos].bOccupied = false;
	Buckets[pos].Distance = 0;

	// Robin Hood 回溯
	ShiftBackward(pos);

	--Size;
	return true;
}

template <typename TType>
typename CSet<T>::Iterator CSet<T>::Erase(Iterator Pos)
{
	if (Pos.Index >= Capacity || !Buckets[Pos.Index].bOccupied)
	{
		return End();
	}

	// GC注销将在.cpp文件中实现以避免循环依赖

	Buckets[Pos.Index].Value.~T();
	Buckets[Pos.Index].bOccupied = false;
	Buckets[Pos.Index].Distance = 0;

	// Robin Hood 回溯
	ShiftBackward(Pos.Index);

	--Size;

	// 返回下一个有效位置的迭代器
	return Iterator(Buckets, Pos.Index, Capacity);
}

template <typename TType>
typename CSet<T>::Iterator CSet<T>::Find(const T& Value)
{
	size_t pos = FindBucket(Value);

	if (pos != Capacity && Buckets[pos].bOccupied && NEqual<T>{}(Buckets[pos].Value, Value))
	{
		return Iterator(Buckets, pos, Capacity);
	}

	return End();
}

template <typename TType>
typename CSet<T>::ConstIterator CSet<T>::Find(const T& Value) const
{
	size_t pos = FindBucket(Value);

	if (pos != Capacity && Buckets[pos].bOccupied && NEqual<T>{}(Buckets[pos].Value, Value))
	{
		return ConstIterator(Buckets, pos, Capacity);
	}

	return End();
}

template <typename TType>
bool CSet<T>::Contains(const T& Value) const
{
	return Find(Value) != End();
}

template <typename TType>
size_t CSet<T>::Count(const T& Value) const
{
	return Contains(Value) ? 1 : 0;
}

template <typename TType>
CSet<T> CSet<T>::Union(const CSet& Other) const
{
	CSet Result(*this);
	Result.Merge(Other);
	return Result;
}

template <typename TType>
CSet<T> CSet<T>::Intersection(const CSet& Other) const
{
	CSet Result;
	for (auto it = Begin(); it != End(); ++it)
	{
		if (Other.Contains(*it))
		{
			Result.Insert(*it);
		}
	}
	return Result;
}

template <typename TType>
CSet<T> CSet<T>::Difference(const CSet& Other) const
{
	CSet Result;
	for (auto it = Begin(); it != End(); ++it)
	{
		if (!Other.Contains(*it))
		{
			Result.Insert(*it);
		}
	}
	return Result;
}

template <typename TType>
bool CSet<T>::IsSubsetOf(const CSet& Other) const
{
	if (Size > Other.Size)
		return false;

	for (auto it = Begin(); it != End(); ++it)
	{
		if (!Other.Contains(*it))
		{
			return false;
		}
	}
	return true;
}

template <typename TType>
CSet<T> CSet<T>::SymmetricDifference(const CSet& Other) const
{
	CSet Result;

	// 添加在this中但不在Other中的元素
	for (auto it = Begin(); it != End(); ++it)
	{
		if (!Other.Contains(*it))
		{
			Result.Insert(*it);
		}
	}

	// 添加在Other中但不在this中的元素
	for (auto it = Other.Begin(); it != Other.End(); ++it)
	{
		if (!Contains(*it))
		{
			Result.Insert(*it);
		}
	}

	return Result;
}

template <typename TType>
bool CSet<T>::IsSupersetOf(const CSet& Other) const
{
	return Other.IsSubsetOf(*this);
}

template <typename TType>
bool CSet<T>::IsDisjointWith(const CSet& Other) const
{
	// 检查两个集合是否没有交集
	for (auto it = Begin(); it != End(); ++it)
	{
		if (Other.Contains(*it))
		{
			return false;
		}
	}
	return true;
}

template <typename TType>
void CSet<T>::Reserve(size_t NewCapacity)
{
	if (NewCapacity > Capacity)
	{
		// 确保容量是2的幂次
		size_t ActualCapacity = 1;
		while (ActualCapacity < NewCapacity)
		{
			ActualCapacity <<= 1;
		}

		Resize(ActualCapacity);
	}
}

template <typename TType>
void CSet<T>::Rehash(size_t BucketCount)
{
	if (BucketCount > 0)
	{
		// 确保容量是2的幂次
		size_t ActualCapacity = 1;
		while (ActualCapacity < BucketCount)
		{
			ActualCapacity <<= 1;
		}

		Resize(ActualCapacity);
	}
}

template <typename TType>
void CSet<T>::Merge(const CSet& Other)
{
	for (auto it = Other.Begin(); it != Other.End(); ++it)
	{
		Insert(*it);
	}
}

template <typename TType>
void CSet<T>::Merge(CSet&& Other)
{
	for (auto it = Other.Begin(); it != Other.End(); ++it)
	{
		Insert(std::move(const_cast<T&>(*it)));
	}
	Other.Clear();
}

template <typename TType>
template <typename TInputIterator>
void CSet<T>::Insert(InputIterator First, InputIterator Last)
{
	for (auto It = First; It != Last; ++It)
	{
		Insert(*It);
	}
}

template <typename TType>
CArray<T> CSet<T>::ToArray() const
{
	CArray<T> Result;
	Result.Reserve(Size);

	for (auto it = Begin(); it != End(); ++it)
	{
		Result.PushBack(*it);
	}

	return Result;
}

template <typename TType>
bool CSet<T>::operator==(const CSet& Other) const
{
	if (Size != Other.Size)
		return false;

	for (auto it = Begin(); it != End(); ++it)
	{
		if (!Other.Contains(*it))
		{
			return false;
		}
	}

	return true;
}

template <typename TType>
bool CSet<T>::operator!=(const CSet& Other) const
{
	return !(*this == Other);
}

template <typename TType>
bool CSet<T>::Equals(const CObject* Other) const
{
	const CSet<T>* OtherSet = dynamic_cast<const CSet<T>*>(Other);
	return OtherSet && (*this == *OtherSet);
}

template <typename TType>
size_t CSet<T>::GetHashCode() const
{
	size_t Hash = 0;
	for (auto it = Begin(); it != End(); ++it)
	{
		Hash ^= NHash<T>{}(*it) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
	}
	return Hash;
}

// 内部实现
template <typename TType>
size_t CSet<T>::Hash(const T& Value) const
{
	return NHash<T>{}(Value);
}

template <typename TType>
size_t CSet<T>::FindBucket(const T& Value) const
{
	if (Capacity == 0)
		return Capacity;

	size_t HashValue = Hash(Value);
	size_t pos = HashValue & (Capacity - 1);

	while (pos < Capacity && Buckets[pos].bOccupied)
	{
		if (NEqual<T>{}(Buckets[pos].Value, Value))
		{
			return pos;
		}

		pos = (pos + 1) & (Capacity - 1);
	}

	return pos;
}

template <typename TType>
void CSet<T>::RobinHoodInsert(T&& Value)
{
	size_t HashValue = Hash(Value);
	size_t pos = HashValue & (Capacity - 1);
	size_t distance = 0;

	while (true)
	{
		if (!Buckets[pos].bOccupied)
		{
			// 找到空位，插入
			new (&Buckets[pos].Value) T(std::move(Value));
			Buckets[pos].Distance = distance;
			Buckets[pos].bOccupied = true;

			// GC注册将在.cpp文件中实现以避免循环依赖

			break;
		}

		// Robin Hood: 如果当前元素距离理想位置更远，则替换
		if (distance > Buckets[pos].Distance)
		{
			std::swap(Value, Buckets[pos].Value);
			std::swap(distance, Buckets[pos].Distance);
		}

		pos = (pos + 1) & (Capacity - 1);
		++distance;
	}
}

template <typename TType>
void CSet<T>::ResizeIfNeeded()
{
	if (Capacity == 0)
	{
		Capacity = DEFAULT_CAPACITY;
		InitializeBuckets();
	}
	else if (GetLoadFactor() >= MaxLoadFactor)
	{
		Resize(Capacity * 2);
	}
}

template <typename TType>
void CSet<T>::Resize(size_t NewCapacity)
{
	CSet temp(NewCapacity);

	for (size_t i = 0; i < Capacity; ++i)
	{
		if (Buckets[i].bOccupied)
		{
			temp.RobinHoodInsert(std::move(Buckets[i].Value));
			++temp.Size;
		}
	}

	*this = std::move(temp);
}

template <typename TType>
void CSet<T>::InitializeBuckets()
{
	if (Capacity > 0)
	{
		Buckets = AllocateElements<Bucket>(Capacity);
		for (size_t i = 0; i < Capacity; ++i)
		{
			new (Buckets + i) Bucket();
		}
	}
}

template <typename TType>
void CSet<T>::DeallocateBuckets()
{
	if (Buckets)
	{
		Clear();
		for (size_t i = 0; i < Capacity; ++i)
		{
			Buckets[i].~Bucket();
		}
		DeallocateElements(Buckets, Capacity);

		Buckets = nullptr;
		Capacity = 0;
		Size = 0;
	}
}

template <typename TType>
void CSet<T>::MoveFrom(CSet&& Other) noexcept
{
	Buckets = Other.Buckets;
	Size = Other.Size;
	Capacity = Other.Capacity;
	MaxLoadFactor = Other.MaxLoadFactor;

	Other.Buckets = nullptr;
	Other.Size = 0;
	Other.Capacity = 0;
}

template <typename TType>
void CSet<T>::CopyFrom(const CSet& Other)
{
	if (Other.Size == 0)
	{
		Clear();
		return;
	}

	DeallocateBuckets();

	Capacity = Other.Capacity;
	MaxLoadFactor = Other.MaxLoadFactor;
	InitializeBuckets();

	for (size_t i = 0; i < Other.Capacity; ++i)
	{
		if (Other.Buckets[i].bOccupied)
		{
			Insert(Other.Buckets[i].Value);
		}
	}
}

template <typename TType>
void CSet<T>::ShiftBackward(size_t pos)
{
	size_t NextPos = (pos + 1) & (Capacity - 1);

	while (NextPos != pos && Buckets[NextPos].bOccupied && Buckets[NextPos].Distance > 0)
	{
		// 将下一个元素向前移动
		Buckets[pos] = std::move(Buckets[NextPos]);
		--Buckets[pos].Distance;

		Buckets[NextPos].bOccupied = false;
		Buckets[NextPos].Distance = 0;

		pos = NextPos;
		NextPos = (NextPos + 1) & (Capacity - 1);
	}
}

} // namespace NLib
