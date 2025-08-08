#pragma once

#include "Logging/LogCategory.h"
#include "TArray.h"
#include "TContainer.h"

#include <functional>
#include <utility>

namespace NLib
{
/**
 * @brief THashMap - 哈希映射容器
 *
 * 高性能的键值对容器，基于开放寻址法实现
 * 特性：
 * - Robin Hood哈希算法，减少聚集
 * - 支持自定义哈希函数和比较函数
 * - 自动扩容和收缩
 * - 内存友好的布局
 */
template <typename TKeyType,
          typename TValueType,
          typename THasher = std::hash<TKeyType>,
          typename TKeyEqual = std::equal_to<TKeyType>,
          typename TAllocator = CMemoryManager>
class THashMap : public TAssociativeContainer<TKeyType, TValueType, TAllocator>
{
public:
	using KeyType = TKeyType;
	using ValueType = TValueType;
	using PairType = std::pair<TKeyType, TValueType>;
	using BaseType = TAssociativeContainer<TKeyType, TValueType, TAllocator>;
	using HasherType = THasher;
	using KeyEqualType = TKeyEqual;
	using typename BaseType::AllocatorType;
	using typename BaseType::SizeType;

private:
	/**
	 * @brief 哈希表桶结构
	 */
	struct SBucket
	{
		PairType Data;     // 键值对数据
		size_t Hash;       // 缓存的哈希值
		uint16_t Distance; // 距离理想位置的距离（Robin Hood）
		bool bOccupied;    // 是否被占用

		SBucket()
		    : Hash(0),
		      Distance(0),
		      bOccupied(false)
		{}

		void SetData(const KeyType& Key, const ValueType& Value, size_t HashValue)
		{
			Data.first = Key;
			Data.second = Value;
			Hash = HashValue;
			Distance = 0;
			bOccupied = true;
		}

		void SetData(const KeyType& Key, ValueType&& Value, size_t HashValue)
		{
			Data.first = Key;
			Data.second = std::move(Value);
			Hash = HashValue;
			Distance = 0;
			bOccupied = true;
		}

		template <typename TKeyArg, typename TValueArg>
		void SetData(TKeyArg&& Key, TValueArg&& Value, size_t HashValue)
		{
			Data.first = std::forward<TKeyArg>(Key);
			Data.second = std::forward<TValueArg>(Value);
			Hash = HashValue;
			Distance = 0;
			bOccupied = true;
		}

		void Clear()
		{
			if (bOccupied)
			{
				Data.~PairType();
				bOccupied = false;
				Distance = 0;
				Hash = 0;
			}
		}
	};

	SBucket* Buckets;         // 桶数组
	SizeType BucketCount;     // 桶数量
	SizeType ElementCount;    // 元素数量
	SizeType MaxDistance;     // 最大探测距离
	HasherType Hasher;        // 哈希函数对象
	KeyEqualType KeyEqual;    // 键比较函数对象
	AllocatorType* Allocator; // 内存分配器

	static constexpr SizeType DefaultBucketCount = 16;
	static constexpr double DefaultMaxLoadFactor = 0.75;
	static constexpr SizeType MaxAllowedDistance = 256;

public:
	// === 构造函数和析构函数 ===

	/**
	 * @brief 默认构造函数
	 */
	THashMap()
	    : Buckets(nullptr),
	      BucketCount(0),
	      ElementCount(0),
	      MaxDistance(0),
	      Hasher(),
	      KeyEqual(),
	      Allocator(&GetDefaultAllocator())
	{
		InitializeBuckets(DefaultBucketCount);
		NLOG(LogCore, Debug, "THashMap default constructed with {} buckets", DefaultBucketCount);
	}

	/**
	 * @brief 指定桶数量的构造函数
	 * @param InitialBucketCount 初始桶数量
	 */
	explicit THashMap(SizeType InitialBucketCount)
	    : Buckets(nullptr),
	      BucketCount(0),
	      ElementCount(0),
	      MaxDistance(0),
	      Hasher(),
	      KeyEqual(),
	      Allocator(&GetDefaultAllocator())
	{
		SizeType ActualBucketCount = std::max(InitialBucketCount, DefaultBucketCount);
		ActualBucketCount = NextPowerOfTwo(ActualBucketCount);
		InitializeBuckets(ActualBucketCount);
		NLOG(LogCore, Debug, "THashMap constructed with {} buckets", ActualBucketCount);
	}

	/**
	 * @brief 初始化列表构造函数
	 * @param InitList 初始化列表
	 */
	THashMap(std::initializer_list<PairType> InitList)
	    : THashMap(InitList.size() * 2)
	{
		for (const auto& Pair : InitList)
		{
			Insert(Pair.first, Pair.second);
		}
		NLOG(LogCore, Debug, "THashMap constructed from initializer list with {} elements", InitList.size());
	}

	/**
	 * @brief 移动构造函数
	 * @param Other 要移动的哈希映射
	 */
	THashMap(THashMap&& Other) noexcept
	    : Buckets(Other.Buckets),
	      BucketCount(Other.BucketCount),
	      ElementCount(Other.ElementCount),
	      MaxDistance(Other.MaxDistance),
	      Hasher(std::move(Other.Hasher)),
	      KeyEqual(std::move(Other.KeyEqual)),
	      Allocator(Other.Allocator)
	{
		Other.Buckets = nullptr;
		Other.BucketCount = 0;
		Other.ElementCount = 0;
		Other.MaxDistance = 0;
		NLOG(LogCore, Debug, "THashMap move constructed");
	}

	/**
	 * @brief 移动赋值操作符
	 * @param Other 要移动的哈希映射
	 * @return 哈希映射引用
	 */
	THashMap& operator=(THashMap&& Other) noexcept
	{
		if (this != &Other)
		{
			Clear();
			DeallocateBuckets();

			Buckets = Other.Buckets;
			BucketCount = Other.BucketCount;
			ElementCount = Other.ElementCount;
			MaxDistance = Other.MaxDistance;
			Hasher = std::move(Other.Hasher);
			KeyEqual = std::move(Other.KeyEqual);
			Allocator = Other.Allocator;

			Other.Buckets = nullptr;
			Other.BucketCount = 0;
			Other.ElementCount = 0;
			Other.MaxDistance = 0;

			NLOG(LogCore, Debug, "THashMap move assigned");
		}
		return *this;
	}

	/**
	 * @brief 析构函数
	 */
	~THashMap()
	{
		Clear();
		DeallocateBuckets();
		NLOG(LogCore, Debug, "THashMap destroyed");
	}

public:
	// === TContainer接口实现 ===

	SizeType Size() const override
	{
		return ElementCount;
	}

	SizeType MaxSize() const override
	{
		return std::numeric_limits<SizeType>::max() / sizeof(SBucket);
	}

	void Clear() override
	{
		for (SizeType i = 0; i < BucketCount; ++i)
		{
			Buckets[i].Clear();
		}
		ElementCount = 0;
		MaxDistance = 0;
		NLOG(LogCore, Debug, "THashMap cleared");
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
		return BucketCount * sizeof(SBucket);
	}

	void ShrinkToFit() override
	{
		if (ElementCount == 0)
		{
			Rehash(DefaultBucketCount);
		}
		else
		{
			SizeType MinBuckets = std::max(NextPowerOfTwo(ElementCount * 2), DefaultBucketCount);
			if (MinBuckets < BucketCount)
			{
				Rehash(MinBuckets);
			}
		}
		NLOG(LogCore, Debug, "THashMap shrunk to fit");
	}

	bool Equals(const TContainer<PairType, TAllocator>& Other) const override
	{
		const THashMap* OtherMap = dynamic_cast<const THashMap*>(&Other);
		if (!OtherMap || OtherMap->Size() != Size())
		{
			return false;
		}

		// 检查每个键值对是否相等
		for (SizeType i = 0; i < BucketCount; ++i)
		{
			if (Buckets[i].bOccupied)
			{
				const ValueType* OtherValue = OtherMap->Find(Buckets[i].Data.first);
				if (!OtherValue || !(*OtherValue == Buckets[i].Data.second))
				{
					return false;
				}
			}
		}

		return true;
	}

	size_t GetHashCode() const override
	{
		size_t Hash = 0;
		for (SizeType i = 0; i < BucketCount; ++i)
		{
			if (Buckets[i].bOccupied)
			{
				// 对键值对的哈希值进行组合
				size_t KeyHash = Hasher(Buckets[i].Data.first);
				size_t ValueHash = std::hash<ValueType>{}(Buckets[i].Data.second);
				Hash ^= KeyHash + ValueHash + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
			}
		}
		return Hash;
	}

	bool Validate() const override
	{
		bool bValid = true;

		if (ElementCount > BucketCount)
		{
			NLOG(LogCore,
			     Error,
			     "THashMap validation failed: element count {} > bucket count {}",
			     ElementCount,
			     BucketCount);
			bValid = false;
		}

		if (BucketCount > 0 && Buckets == nullptr)
		{
			NLOG(LogCore, Error, "THashMap validation failed: bucket count {} but buckets is null", BucketCount);
			bValid = false;
		}

		// 验证元素数量
		SizeType ActualElementCount = 0;
		for (SizeType i = 0; i < BucketCount; ++i)
		{
			if (Buckets[i].bOccupied)
			{
				++ActualElementCount;

				// 验证距离计算
				SizeType IdealIndex = Buckets[i].Hash & (BucketCount - 1);
				SizeType ActualDistance = (i - IdealIndex) & (BucketCount - 1);
				if (Buckets[i].Distance != ActualDistance)
				{
					NLOG(LogCore, Error, "THashMap validation failed: incorrect distance at bucket {}", i);
					bValid = false;
				}
			}
		}

		if (ActualElementCount != ElementCount)
		{
			NLOG(LogCore,
			     Error,
			     "THashMap validation failed: actual element count {} != recorded count {}",
			     ActualElementCount,
			     ElementCount);
			bValid = false;
		}

		return bValid;
	}

	typename BaseType::SContainerStats GetStats() const override
	{
		typename BaseType::SContainerStats Stats;
		Stats.ElementCount = ElementCount;
		Stats.Capacity = BucketCount;
		Stats.MemoryUsage = GetMemoryUsage();
		Stats.WastedMemory = (BucketCount - ElementCount) * sizeof(SBucket);
		Stats.LoadFactor = BucketCount > 0 ? static_cast<double>(ElementCount) / BucketCount : 0.0;
		return Stats;
	}

public:
	// === TAssociativeContainer接口实现 ===

	bool Contains(const KeyType& Key) const override
	{
		return Find(Key) != nullptr;
	}

	ValueType* Find(const KeyType& Key) override
	{
		SizeType Index = FindBucketIndex(Key);
		return (Index != SIZE_MAX) ? &Buckets[Index].Data.second : nullptr;
	}

	const ValueType* Find(const KeyType& Key) const override
	{
		SizeType Index = FindBucketIndex(Key);
		return (Index != SIZE_MAX) ? &Buckets[Index].Data.second : nullptr;
	}

	SizeType Count(const KeyType& Key) const override
	{
		return Contains(Key) ? 1 : 0;
	}

	bool Insert(const KeyType& Key, const ValueType& Value) override
	{
		return InsertInternal(Key, Value);
	}

	bool Insert(const KeyType& Key, ValueType&& Value) override
	{
		return InsertInternal(Key, std::move(Value));
	}

	SizeType Remove(const KeyType& Key) override
	{
		SizeType Index = FindBucketIndex(Key);
		if (Index == SIZE_MAX)
		{
			return 0;
		}

		RemoveBucket(Index);
		return 1;
	}

public:
	// === 访问操作符 ===

	/**
	 * @brief 访问或插入元素
	 * @param Key 键
	 * @return 值的引用
	 */
	ValueType& operator[](const KeyType& Key)
	{
		SizeType Index = FindBucketIndex(Key);
		if (Index != SIZE_MAX)
		{
			return Buckets[Index].Data.second;
		}

		// 插入新元素
		Index = InsertNewElement(Key, ValueType{});
		return Buckets[Index].Data.second;
	}

	/**
	 * @brief 访问元素（常量版本）
	 * @param Key 键
	 * @return 值的常量引用
	 */
	const ValueType& At(const KeyType& Key) const
	{
		const ValueType* Value = Find(Key);
		if (!Value)
		{
			NLOG(LogCore, Error, "THashMap::At() key not found");
			throw std::out_of_range("Key not found in THashMap");
		}
		return *Value;
	}

public:
	// === 容量管理 ===

	/**
	 * @brief 获取桶数量
	 * @return 桶数量
	 */
	SizeType BucketSize() const
	{
		return BucketCount;
	}

	/**
	 * @brief 获取负载因子
	 * @return 负载因子
	 */
	double LoadFactor() const
	{
		return BucketCount > 0 ? static_cast<double>(ElementCount) / BucketCount : 0.0;
	}

	/**
	 * @brief 重新哈希
	 * @param NewBucketCount 新的桶数量
	 */
	void Rehash(SizeType NewBucketCount)
	{
		NewBucketCount = std::max(NewBucketCount, DefaultBucketCount);
		NewBucketCount = NextPowerOfTwo(NewBucketCount);

		if (NewBucketCount == BucketCount)
		{
			return;
		}

		// 保存旧数据
		SBucket* OldBuckets = Buckets;
		SizeType OldBucketCount = BucketCount;

		// 分配新桶
		InitializeBuckets(NewBucketCount);
		ElementCount = 0;
		MaxDistance = 0;

		// 重新插入所有元素
		for (SizeType i = 0; i < OldBucketCount; ++i)
		{
			if (OldBuckets[i].bOccupied)
			{
				InsertInternal(std::move(OldBuckets[i].Data.first), std::move(OldBuckets[i].Data.second));
				OldBuckets[i].Clear();
			}
		}

		// 释放旧桶
		if (OldBuckets)
		{
			Allocator->DeallocateObject(OldBuckets);
		}

		NLOG(LogCore, Debug, "THashMap rehashed to {} buckets", NewBucketCount);
	}

	/**
	 * @brief 预留容量
	 * @param Count 预期元素数量
	 */
	void Reserve(SizeType Count)
	{
		SizeType RequiredBuckets = static_cast<SizeType>(Count / DefaultMaxLoadFactor) + 1;
		RequiredBuckets = NextPowerOfTwo(RequiredBuckets);
		if (RequiredBuckets > BucketCount)
		{
			Rehash(RequiredBuckets);
		}
	}

public:
	// === 迭代器支持 ===
	
	/**
	 * @brief 简单的迭代器实现
	 */
	class Iterator
	{
	public:
		Iterator(SBucket* InBuckets, SizeType InBucketCount, SizeType InIndex)
			: Buckets(InBuckets), BucketCount(InBucketCount), Index(InIndex)
		{
			// 跳到第一个有效元素
			while (Index < BucketCount && !Buckets[Index].bOccupied)
			{
				++Index;
			}
		}
		
		PairType& operator*() { return Buckets[Index].Data; }
		const PairType& operator*() const { return Buckets[Index].Data; }
		PairType* operator->() { return &Buckets[Index].Data; }
		const PairType* operator->() const { return &Buckets[Index].Data; }
		
		Iterator& operator++()
		{
			if (Index < BucketCount)
			{
				++Index;
				// 跳到下一个有效元素
				while (Index < BucketCount && !Buckets[Index].bOccupied)
				{
					++Index;
				}
			}
			return *this;
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
		SBucket* Buckets;
		SizeType BucketCount;
		SizeType Index;
	};
	
	/**
	 * @brief 获取开始迭代器
	 */
	Iterator begin()
	{
		return Iterator(Buckets, BucketCount, 0);
	}
	
	/**
	 * @brief 获取结束迭代器
	 */
	Iterator end()
	{
		return Iterator(Buckets, BucketCount, BucketCount);
	}
	
	/**
	 * @brief 常量迭代器
	 */
	class ConstIterator
	{
	public:
		ConstIterator(const SBucket* InBuckets, SizeType InBucketCount, SizeType InIndex)
			: Buckets(InBuckets), BucketCount(InBucketCount), Index(InIndex)
		{
			// 跳到第一个有效元素
			while (Index < BucketCount && !Buckets[Index].bOccupied)
			{
				++Index;
			}
		}
		
		const PairType& operator*() const { return Buckets[Index].Data; }
		const PairType* operator->() const { return &Buckets[Index].Data; }
		
		ConstIterator& operator++()
		{
			if (Index < BucketCount)
			{
				++Index;
				// 跳到下一个有效元素
				while (Index < BucketCount && !Buckets[Index].bOccupied)
				{
					++Index;
				}
			}
			return *this;
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
		const SBucket* Buckets;
		SizeType BucketCount;
		SizeType Index;
	};

	/**
	 * @brief 获取开始迭代器（常量版本）
	 */
	ConstIterator begin() const
	{
		return ConstIterator(Buckets, BucketCount, 0);
	}
	
	/**
	 * @brief 获取结束迭代器（常量版本）
	 */
	ConstIterator end() const
	{
		return ConstIterator(Buckets, BucketCount, BucketCount);
	}

public:
	// === 高级操作 ===

	/**
	 * @brief 尝试插入元素
	 * @param Key 键
	 * @param Value 值
	 * @return std::pair<迭代器, 是否插入成功>
	 */
	bool TryInsert(const KeyType& Key, const ValueType& Value)
	{
		if (Contains(Key))
		{
			return false;
		}
		return Insert(Key, Value);
	}

	/**
	 * @brief 插入或更新元素
	 * @param Key 键
	 * @param Value 值
	 * @return 是否是新插入的元素
	 */
	bool InsertOrAssign(const KeyType& Key, const ValueType& Value)
	{
		SizeType Index = FindBucketIndex(Key);
		if (Index != SIZE_MAX)
		{
			Buckets[Index].Data.second = Value;
			return false;
		}

		InsertNewElement(Key, Value);
		return true;
	}

	/**
	 * @brief 就地构造元素
	 * @tparam TArgs 构造参数类型
	 * @param Key 键
	 * @param Args 值的构造参数
	 * @return 是否成功插入
	 */
	template <typename... TArgs>
	bool Emplace(const KeyType& Key, TArgs&&... Args)
	{
		if (Contains(Key))
		{
			return false;
		}

		EnsureCapacity();

		size_t Hash = Hasher(Key);
		SizeType Index = Hash & (BucketCount - 1);
		uint16_t Distance = 0;

		while (Distance < MaxAllowedDistance)
		{
			if (!Buckets[Index].bOccupied)
			{
				new (&Buckets[Index].Data.second) ValueType(std::forward<TArgs>(Args)...);
				Buckets[Index].Data.first = Key;
				Buckets[Index].Hash = Hash;
				Buckets[Index].Distance = Distance;
				Buckets[Index].bOccupied = true;

				++ElementCount;
				MaxDistance = std::max(MaxDistance, static_cast<SizeType>(Distance));
				return true;
			}

			if (Buckets[Index].Distance < Distance)
			{
				// Robin Hood: 交换位置
				SwapBuckets(Index, Key, ValueType(std::forward<TArgs>(Args)...), Hash, Distance);
				++ElementCount;
				MaxDistance = std::max(MaxDistance, static_cast<SizeType>(Distance));
				return true;
			}

			Index = (Index + 1) & (BucketCount - 1);
			++Distance;
		}

		// 如果达到最大距离，需要重新哈希
		Rehash(BucketCount * 2);
		return Emplace(Key, std::forward<TArgs>(Args)...);
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
	 * @brief 计算下一个2的幂
	 * @param Value 输入值
	 * @return 下一个2的幂
	 */
	static SizeType NextPowerOfTwo(SizeType Value)
	{
		if (Value <= 1)
			return 1;

		Value--;
		Value |= Value >> 1;
		Value |= Value >> 2;
		Value |= Value >> 4;
		Value |= Value >> 8;
		Value |= Value >> 16;
		if constexpr (sizeof(SizeType) > 4)
		{
			Value |= Value >> 32;
		}
		Value++;

		return Value;
	}

	/**
	 * @brief 初始化桶数组
	 * @param Count 桶数量
	 */
	void InitializeBuckets(SizeType Count)
	{
		Buckets = static_cast<SBucket*>(Allocator->AllocateObject(Count * sizeof(SBucket)));
		if (!Buckets)
		{
			NLOG(LogCore, Error, "THashMap failed to allocate {} buckets", Count);
			throw std::bad_alloc();
		}

		// 初始化所有桶
		for (SizeType i = 0; i < Count; ++i)
		{
			new (Buckets + i) SBucket();
		}

		BucketCount = Count;
	}

	/**
	 * @brief 释放桶数组
	 */
	void DeallocateBuckets()
	{
		if (Buckets)
		{
			for (SizeType i = 0; i < BucketCount; ++i)
			{
				Buckets[i].~SBucket();
			}
			Allocator->DeallocateObject(Buckets);
			Buckets = nullptr;
			BucketCount = 0;
		}
	}

	/**
	 * @brief 查找桶索引
	 * @param Key 键
	 * @return 桶索引，未找到返回SIZE_MAX
	 */
	SizeType FindBucketIndex(const KeyType& Key) const
	{
		if (BucketCount == 0)
		{
			return SIZE_MAX;
		}

		size_t Hash = Hasher(Key);
		SizeType Index = Hash & (BucketCount - 1);
		uint16_t Distance = 0;

		while (Distance <= MaxDistance && Distance < MaxAllowedDistance)
		{
			if (!Buckets[Index].bOccupied)
			{
				return SIZE_MAX;
			}

			if (Buckets[Index].Hash == Hash && KeyEqual(Buckets[Index].Data.first, Key))
			{
				return Index;
			}

			if (Buckets[Index].Distance < Distance)
			{
				return SIZE_MAX;
			}

			Index = (Index + 1) & (BucketCount - 1);
			++Distance;
		}

		return SIZE_MAX;
	}

	/**
	 * @brief 确保容量足够
	 */
	void EnsureCapacity()
	{
		if (LoadFactor() >= DefaultMaxLoadFactor)
		{
			Rehash(BucketCount * 2);
		}
	}

	/**
	 * @brief 插入元素的内部实现
	 * @tparam TValueArg 值参数类型
	 * @param Key 键
	 * @param Value 值
	 * @return 是否成功插入
	 */
	template <typename TValueArg>
	bool InsertInternal(const KeyType& Key, TValueArg&& Value)
	{
		if (Contains(Key))
		{
			return false;
		}

		InsertNewElement(Key, std::forward<TValueArg>(Value));
		return true;
	}

	/**
	 * @brief 插入新元素
	 * @tparam TValueArg 值参数类型
	 * @param Key 键
	 * @param Value 值
	 * @return 插入的桶索引
	 */
	template <typename TValueArg>
	SizeType InsertNewElement(const KeyType& Key, TValueArg&& Value)
	{
		EnsureCapacity();

		size_t Hash = Hasher(Key);
		SizeType Index = Hash & (BucketCount - 1);
		uint16_t Distance = 0;

		KeyType KeyToInsert = Key;
		ValueType ValueToInsert = std::forward<TValueArg>(Value);
		size_t HashToInsert = Hash;

		while (Distance < MaxAllowedDistance)
		{
			if (!Buckets[Index].bOccupied)
			{
				Buckets[Index].SetData(std::move(KeyToInsert), std::move(ValueToInsert), HashToInsert);
				Buckets[Index].Distance = Distance;
				++ElementCount;
				MaxDistance = std::max(MaxDistance, static_cast<SizeType>(Distance));
				return Index;
			}

			if (Buckets[Index].Distance < Distance)
			{
				// Robin Hood: 交换位置
				std::swap(KeyToInsert, Buckets[Index].Data.first);
				std::swap(ValueToInsert, Buckets[Index].Data.second);
				std::swap(HashToInsert, Buckets[Index].Hash);
				std::swap(Distance, Buckets[Index].Distance);
			}

			Index = (Index + 1) & (BucketCount - 1);
			++Distance;
		}

		// 如果达到最大距离，需要重新哈希
		Rehash(BucketCount * 2);
		return InsertNewElement(Key, std::forward<TValueArg>(Value));
	}

	/**
	 * @brief 移除桶中的元素
	 * @param Index 桶索引
	 */
	void RemoveBucket(SizeType Index)
	{
		Buckets[Index].Clear();
		--ElementCount;

		// 重新排列后续元素（向前移动）
		SizeType NextIndex = (Index + 1) & (BucketCount - 1);
		while (Buckets[NextIndex].bOccupied && Buckets[NextIndex].Distance > 0)
		{
			// 移动元素到前一个位置
			Buckets[Index] = std::move(Buckets[NextIndex]);
			--Buckets[Index].Distance;

			Buckets[NextIndex].Clear();
			Index = NextIndex;
			NextIndex = (NextIndex + 1) & (BucketCount - 1);
		}

		// 重新计算最大距离
		RecalculateMaxDistance();
	}

	/**
	 * @brief 重新计算最大距离
	 */
	void RecalculateMaxDistance()
	{
		MaxDistance = 0;
		for (SizeType i = 0; i < BucketCount; ++i)
		{
			if (Buckets[i].bOccupied)
			{
				MaxDistance = std::max(MaxDistance, static_cast<SizeType>(Buckets[i].Distance));
			}
		}
	}
};

// === 类型别名 ===

template <typename TKeyType, typename TValueType>
using THashMapPtr = TSharedPtr<THashMap<TKeyType, TValueType>>;

// 常用类型的别名
using StringToIntMap = THashMap<CString, int32_t>;
using StringToFloatMap = THashMap<CString, float>;
using StringToStringMap = THashMap<CString, CString>;
using StringToObjectMap = THashMap<CString, NObjectPtr>;
using IntToStringMap = THashMap<int32_t, CString>;

} // namespace NLib