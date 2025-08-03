#pragma once

#include "Logging/CLogger.h"
#include "CArray.h"
#include "CContainer.h"
#include "CHashMap.generate.h"
#include "NLibMacros.h"

namespace NLib
{

template <typename TKey, typename TValue>
struct NKeyValuePair
{
	K Key;
	V Value;

	NKeyValuePair() = default;
	NKeyValuePair(const K& Key, const V& Value)
	    : Key(Key),
	      Value(Value)
	{}
	NKeyValuePair(K&& Key, V&& Value)
	    : Key(std::move(Key)),
	      Value(std::move(Value))
	{}

	bool operator==(const NKeyValuePair& other) const
	{
		return NLib::NEqual<K>{}(Key, other.Key) && NLib::NEqual<V>{}(Value, other.Value);
	}
};

template <typename TKey, typename TValue>
NCLASS()
class LIBNLIB_API CHashMap : public CContainer
{
	GENERATED_BODY()

public:
	using KeyType = K;
	using ValueType = V;
	using PairType = NKeyValuePair<K, V>;
	using Reference = V&;
	using ConstReference = const V&;
	using KeyReference = const K&;

	// Robin Hood 哈希表的桶结构
	struct Bucket
	{
		PairType Pair;
		size_t Distance; // 距离理想位置的距离
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

		PairType& operator*()
		{
			return Buckets[Index].Pair;
		}
		const PairType& operator*() const
		{
			return Buckets[Index].Pair;
		}
		PairType* operator->()
		{
			return &Buckets[Index].Pair;
		}
		const PairType* operator->() const
		{
			return &Buckets[Index].Pair;
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
	};

	class ConstIterator
	{
	public:
		ConstIterator(const Bucket* Buckets, size_t Index, size_t Capacity)
		    : Buckets(Buckets),
		      Index(Index),
		      Capacity(Capacity)
		{
			while (Index < Capacity && !Buckets[Index].bOccupied)
			{
				++Index;
			}
		}

		const PairType& operator*() const
		{
			return Buckets[Index].Pair;
		}
		const PairType* operator->() const
		{
			return &Buckets[Index].Pair;
		}

		ConstIterator& operator++()
		{
			++Index;
			while (Index < Capacity && !Buckets[Index].bOccupied)
			{
				++Index;
			}
			return *this;
		}

		ConstIterator operator++(int)
		{
			ConstIterator temp(*this);
			++(*this);
			return temp;
		}

		bool operator==(const ConstIterator& Other) const
		{
			return Index == Other.Index;
		}

		bool operator!=(const ConstIterator& Other) const
		{
			return Index != Other.Index;
		}

	private:
		const Bucket* Buckets;
		size_t Index;
		size_t Capacity;
	};

	// 构造函数
	CHashMap();
	explicit CHashMap(size_t InitialCapacity);
	CHashMap(std::initializer_list<PairType> InitList);
	CHashMap(const CHashMap& other);
	CHashMap(CHashMap&& other) noexcept;

	// 析构函数
	virtual ~CHashMap();

	// 赋值操作符
	CHashMap& operator=(const CHashMap& other);
	CHashMap& operator=(CHashMap&& other) noexcept;
	CHashMap& operator=(std::initializer_list<PairType> InitList);

	// 基础访问
	Reference operator[](const K& Key);
	Reference operator[](K&& Key);
	ConstReference At(const K& Key) const;
	Reference At(const K& Key);

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
		return Capacity > 0 ? static_cast<float>(Size) / Capacity : 0.0f;
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
	std::pair<Iterator, bool> Insert(const PairType& pair);
	std::pair<Iterator, bool> Insert(PairType&& pair);
	std::pair<Iterator, bool> Insert(const K& key, const V& value);
	std::pair<Iterator, bool> Insert(K&& key, V&& value);

	template <typename... Args>
	std::pair<Iterator, bool> Emplace(const K& key, Args&&... args);
	template <typename... Args>
	std::pair<Iterator, bool> Emplace(K&& key, Args&&... args);

	std::pair<Iterator, bool> InsertOrAssign(const K& key, const V& value);
	std::pair<Iterator, bool> InsertOrAssign(const K& key, V&& value);
	std::pair<Iterator, bool> InsertOrAssign(K&& key, const V& value);
	std::pair<Iterator, bool> InsertOrAssign(K&& key, V&& value);

	template <typename... Args>
	std::pair<Iterator, bool> TryEmplace(const K& key, Args&&... args);
	template <typename... Args>
	std::pair<Iterator, bool> TryEmplace(K&& key, Args&&... args);

	bool Erase(const K& key);
	Iterator Erase(Iterator pos);
	Iterator Erase(ConstIterator pos);

	// 查找操作
	Iterator Find(const K& key);
	ConstIterator Find(const K& key) const;

	bool Contains(const K& key) const;
	size_t Count(const K& key) const; // 对于HashMap总是返回0或1

	// 批量操作
	void Merge(const CHashMap& other);
	void Merge(CHashMap&& other);

	// 键和值的访问
	CArray<K> GetKeys() const;
	CArray<V> GetValues() const;

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
	bool operator==(const CHashMap& other) const;
	bool operator!=(const CHashMap& other) const;

	// CObject 接口重写
	virtual bool Equals(const CObject* other) const override;
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
	size_t Hash(const K& Key) const;
	size_t FindBucket(const K& Key) const;
	size_t FindInsertPosition(const K& Key, size_t HashValue) const;
	void InsertAtPosition(size_t Pos, PairType&& Pair, size_t Distance);
	void RobinHoodInsert(PairType&& Pair);
	void ResizeIfNeeded();
	void Resize(size_t NewCapacity);
	void InitializeBuckets();
	void DeallocateBuckets();
	void MoveFrom(CHashMap&& other) noexcept;
	void CopyFrom(const CHashMap& other);

	// Robin Hood 相关
	size_t CalculateDistance(size_t HashValue, size_t ActualPos) const;
	void ShiftBackward(size_t Pos);
};

// 模板实现
template <typename TKey, typename TValue>
CHashMap<K, V>::CHashMap()
    : Buckets(nullptr),
      Size(0),
      Capacity(0),
      MaxLoadFactor(DEFAULT_MAX_LOAD_FACTOR)
{}

template <typename TKey, typename TValue>
CHashMap<K, V>::CHashMap(size_t InitialCapacity)
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

template <typename TKey, typename TValue>
CHashMap<K, V>::CHashMap(std::initializer_list<PairType> InitList)
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

		for (const auto& pair : InitList)
		{
			Insert(pair);
		}
	}
}

template <typename TKey, typename TValue>
CHashMap<K, V>::CHashMap(const CHashMap& other)
    : Buckets(nullptr),
      Size(0),
      Capacity(0),
      MaxLoadFactor(DEFAULT_MAX_LOAD_FACTOR)
{
	CopyFrom(other);
}

template <typename TKey, typename TValue>
CHashMap<K, V>::CHashMap(CHashMap&& other) noexcept
    : Buckets(nullptr),
      Size(0),
      Capacity(0),
      MaxLoadFactor(DEFAULT_MAX_LOAD_FACTOR)
{
	MoveFrom(std::move(other));
}

template <typename TKey, typename TValue>
CHashMap<K, V>::~CHashMap()
{
	DeallocateBuckets();
}

template <typename TKey, typename TValue>
CHashMap<K, V>& CHashMap<K, V>::operator=(const CHashMap& other)
{
	if (this != &other)
	{
		CopyFrom(other);
	}
	return *this;
}

template <typename TKey, typename TValue>
CHashMap<K, V>& CHashMap<K, V>::operator=(CHashMap&& other) noexcept
{
	if (this != &other)
	{
		DeallocateBuckets();
		MoveFrom(std::move(other));
	}
	return *this;
}

template <typename TKey, typename TValue>
typename CHashMap<K, V>::Reference CHashMap<K, V>::operator[](const K& key)
{
	auto result = TryEmplace(key);
	return result.first->Value;
}

template <typename TKey, typename TValue>
typename CHashMap<K, V>::Reference CHashMap<K, V>::operator[](K&& key)
{
	auto result = TryEmplace(std::move(key));
	return result.first->Value;
}

template <typename TKey, typename TValue>
typename CHashMap<K, V>::ConstReference CHashMap<K, V>::At(const K& key) const
{
	ConstIterator it = Find(key);
	if (it == End())
	{
		NLib::CLogger::Error("CHashMap::At: Key not found");
		static const V dummy{};
		return dummy;
	}
	return it->Value;
}

template <typename TKey, typename TValue>
typename CHashMap<K, V>::Reference CHashMap<K, V>::At(const K& key)
{
	Iterator it = Find(key);
	if (it == End())
	{
		NLib::CLogger::Error("CHashMap::At: Key not found");
		static V dummy{};
		return dummy;
	}
	return it->Value;
}

template <typename TKey, typename TValue>
void CHashMap<K, V>::Clear()
{
	if (Buckets)
	{
		for (size_t i = 0; i < Capacity; ++i)
		{
			if (Buckets[i].bOccupied)
			{
				if constexpr (std::is_base_of_v<CObject, K>)
				{
					NLib::CGarbageCollector::GetInstance().UnregisterObject(&Buckets[i].Pair.Key);
				}
				if constexpr (std::is_base_of_v<CObject, V>)
				{
					NLib::CGarbageCollector::GetInstance().UnregisterObject(&Buckets[i].Pair.Value);
				}

				Buckets[i].Pair.~PairType();
				Buckets[i].bOccupied = false;
				Buckets[i].Distance = 0;
			}
		}
		Size = 0;
	}
}

template <typename TKey, typename TValue>
std::pair<typename CHashMap<K, V>::Iterator, bool> CHashMap<K, V>::Insert(const PairType& pair)
{
	return Insert(PairType(pair));
}

template <typename TKey, typename TValue>
std::pair<typename CHashMap<K, V>::Iterator, bool> CHashMap<K, V>::Insert(PairType&& pair)
{
	ResizeIfNeeded();

	size_t pos = FindBucket(pair.Key);

	if (pos != Capacity && Buckets[pos].bOccupied && NLib::NEqual<K>{}(Buckets[pos].Pair.Key, pair.Key))
	{
		// 键已存在
		return std::make_pair(Iterator(Buckets, pos, Capacity), false);
	}

	// 插入新元素
	RobinHoodInsert(std::move(pair));
	++Size;

	// 查找插入后的位置
	pos = FindBucket(pair.Key);
	return std::make_pair(Iterator(Buckets, pos, Capacity), true);
}

template <typename TKey, typename TValue>
std::pair<typename CHashMap<K, V>::Iterator, bool> CHashMap<K, V>::Insert(const K& key, const V& value)
{
	return Insert(PairType(key, value));
}

template <typename TKey, typename TValue>
template <typename... Args>
std::pair<typename CHashMap<K, V>::Iterator, bool> CHashMap<K, V>::TryEmplace(const K& key, Args&&... args)
{
	ResizeIfNeeded();

	size_t pos = FindBucket(key);

	if (pos != Capacity && Buckets[pos].bOccupied && NLib::NEqual<K>{}(Buckets[pos].Pair.Key, key))
	{
		// 键已存在，不插入
		return std::make_pair(Iterator(Buckets, pos, Capacity), false);
	}

	// 插入新元素
	PairType NewPair(key, V(std::forward<Args>(args)...));
	RobinHoodInsert(std::move(NewPair));
	++Size;

	// 查找插入后的位置
	pos = FindBucket(key);
	return std::make_pair(Iterator(Buckets, pos, Capacity), true);
}

template <typename TKey, typename TValue>
template <typename... Args>
std::pair<typename CHashMap<K, V>::Iterator, bool> CHashMap<K, V>::TryEmplace(K&& key, Args&&... args)
{
	ResizeIfNeeded();

	size_t pos = FindBucket(key);

	if (pos != Capacity && Buckets[pos].bOccupied && NLib::NEqual<K>{}(Buckets[pos].Pair.Key, key))
	{
		// 键已存在，不插入
		return std::make_pair(Iterator(Buckets, pos, Capacity), false);
	}

	// 插入新元素
	PairType NewPair(std::move(key), V(std::forward<Args>(args)...));
	RobinHoodInsert(std::move(NewPair));
	++Size;

	// 查找插入后的位置 (注意：key已被移动，需要用NewPair.Key)
	pos = FindBucket(NewPair.Key);
	return std::make_pair(Iterator(Buckets, pos, Capacity), true);
}

template <typename TKey, typename TValue>
bool CHashMap<K, V>::Erase(const K& key)
{
	size_t pos = FindBucket(key);

	if (pos == Capacity || !Buckets[pos].bOccupied || !NLib::NEqual<K>{}(Buckets[pos].Pair.Key, key))
	{
		return false;
	}

	// 注销GC
	if constexpr (std::is_base_of_v<CObject, K>)
	{
		NLib::CGarbageCollector::GetInstance().UnregisterObject(&Buckets[pos].Pair.Key);
	}
	if constexpr (std::is_base_of_v<CObject, V>)
	{
		NLib::CGarbageCollector::GetInstance().UnregisterObject(&Buckets[pos].Pair.Value);
	}

	Buckets[pos].Pair.~PairType();
	Buckets[pos].bOccupied = false;
	Buckets[pos].Distance = 0;

	// Robin Hood 回溯
	ShiftBackward(pos);

	--Size;
	return true;
}

template <typename TKey, typename TValue>
typename CHashMap<K, V>::Iterator CHashMap<K, V>::Find(const K& key)
{
	size_t pos = FindBucket(key);

	if (pos != Capacity && Buckets[pos].bOccupied && NLib::NEqual<K>{}(Buckets[pos].Pair.Key, key))
	{
		return Iterator(Buckets, pos, Capacity);
	}

	return End();
}

template <typename TKey, typename TValue>
typename CHashMap<K, V>::ConstIterator CHashMap<K, V>::Find(const K& key) const
{
	size_t pos = FindBucket(key);

	if (pos != Capacity && Buckets[pos].bOccupied && NLib::NEqual<K>{}(Buckets[pos].Pair.Key, key))
	{
		return ConstIterator(Buckets, pos, Capacity);
	}

	return End();
}

template <typename TKey, typename TValue>
bool CHashMap<K, V>::Contains(const K& key) const
{
	return Find(key) != End();
}

template <typename TKey, typename TValue>
bool CHashMap<K, V>::operator==(const CHashMap& other) const
{
	if (Size != other.Size)
		return false;

	for (auto it = Begin(); it != End(); ++it)
	{
		auto other_it = other.Find(it->Key);
		if (other_it == other.End() || !NLib::NEqual<V>{}(it->Value, other_it->Value))
		{
			return false;
		}
	}

	return true;
}

template <typename TKey, typename TValue>
bool CHashMap<K, V>::operator!=(const CHashMap& other) const
{
	return !(*this == other);
}

template <typename TKey, typename TValue>
bool CHashMap<K, V>::Equals(const CObject* other) const
{
	const CHashMap<K, V>* other_map = dynamic_cast<const CHashMap<K, V>*>(other);
	return other_map && (*this == *other_map);
}

template <typename TKey, typename TValue>
size_t CHashMap<K, V>::GetHashCode() const
{
	size_t hash = 0;
	for (auto it = Begin(); it != End(); ++it)
	{
		size_t PairHash = NLib::NHash<K>{}(it->Key) ^ (NLib::NHash<V>{}(it->Value) << 1);
		hash ^= PairHash + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	}
	return hash;
}

// ToString方法声明移至实现文件以避免循环依赖
// 这里只保留声明，实现将在NHashMap.cpp中通过显式模板实例化提供

// 内部实现
template <typename TKey, typename TValue>
size_t CHashMap<K, V>::Hash(const K& key) const
{
	return NLib::NHash<K>{}(key);
}

template <typename TKey, typename TValue>
size_t CHashMap<K, V>::FindBucket(const K& key) const
{
	if (Capacity == 0)
		return Capacity;

	size_t HashValue = Hash(key);
	size_t pos = HashValue & (Capacity - 1); // 假设capacity是2的幂次

	while (pos < Capacity && Buckets[pos].bOccupied)
	{
		if (NLib::NEqual<K>{}(Buckets[pos].Pair.Key, key))
		{
			return pos;
		}

		pos = (pos + 1) & (Capacity - 1);
	}

	return pos;
}

template <typename TKey, typename TValue>
void CHashMap<K, V>::RobinHoodInsert(PairType&& pair)
{
	size_t HashValue = Hash(pair.Key);
	size_t pos = HashValue & (Capacity - 1);
	size_t distance = 0;

	while (true)
	{
		if (!Buckets[pos].bOccupied)
		{
			// 找到空位，插入
			new (&Buckets[pos].Pair) PairType(std::move(pair));
			Buckets[pos].Distance = distance;
			Buckets[pos].bOccupied = true;

			if constexpr (std::is_base_of_v<CObject, K>)
			{
				NLib::CGarbageCollector::GetInstance().RegisterObject(&Buckets[pos].Pair.Key);
			}
			if constexpr (std::is_base_of_v<CObject, V>)
			{
				NLib::CGarbageCollector::GetInstance().RegisterObject(&Buckets[pos].Pair.Value);
			}

			break;
		}

		// Robin Hood: 如果当前元素距离理想位置更远，则替换
		if (distance > Buckets[pos].Distance)
		{
			std::swap(pair, Buckets[pos].Pair);
			std::swap(distance, Buckets[pos].Distance);
		}

		pos = (pos + 1) & (Capacity - 1);
		++distance;
	}
}

template <typename TKey, typename TValue>
void CHashMap<K, V>::ResizeIfNeeded()
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

template <typename TKey, typename TValue>
void CHashMap<K, V>::Resize(size_t NewCapacity)
{
	CHashMap temp(NewCapacity);

	for (size_t i = 0; i < Capacity; ++i)
	{
		if (Buckets[i].bOccupied)
		{
			temp.RobinHoodInsert(std::move(Buckets[i].Pair));
			++temp.Size;
		}
	}

	*this = std::move(temp);
}

template <typename TKey, typename TValue>
void CHashMap<K, V>::InitializeBuckets()
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

template <typename TKey, typename TValue>
void CHashMap<K, V>::DeallocateBuckets()
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

template <typename TKey, typename TValue>
void CHashMap<K, V>::MoveFrom(CHashMap&& other) noexcept
{
	Buckets = other.Buckets;
	Size = other.Size;
	Capacity = other.Capacity;
	MaxLoadFactor = other.MaxLoadFactor;

	other.Buckets = nullptr;
	other.Size = 0;
	other.Capacity = 0;
}

template <typename TKey, typename TValue>
void CHashMap<K, V>::CopyFrom(const CHashMap& other)
{
	if (other.Size == 0)
	{
		Clear();
		return;
	}

	DeallocateBuckets();

	Capacity = other.Capacity;
	MaxLoadFactor = other.MaxLoadFactor;
	InitializeBuckets();

	for (size_t i = 0; i < other.Capacity; ++i)
	{
		if (other.Buckets[i].bOccupied)
		{
			Insert(other.Buckets[i].Pair);
		}
	}
}

template <typename TKey, typename TValue>
void CHashMap<K, V>::ShiftBackward(size_t pos)
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