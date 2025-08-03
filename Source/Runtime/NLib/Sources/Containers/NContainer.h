#pragma once

#include "Core/CObject.h"
#include "Memory/CGarbageCollector.h"
#include "Memory/CMemoryManager.h"
#include "CContainer.generate.h"
#include "NLibMacros.h"

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace NLib
{
NCLASS()
class LIBNLIB_API CContainer : public CObject
{
	GENERATED_BODY()

public:
	virtual ~CContainer() = default;

	virtual size_t GetSize() const = 0;
	virtual size_t GetCapacity() const = 0;
	virtual bool IsEmpty() const = 0;
	virtual void Clear() = 0;

protected:
	static constexpr size_t CONTAINER_ALIGNMENT = 8;
	static constexpr size_t MIN_CAPACITY = 4;

	// 容器专用内存分配器
	template <typename TType>
	static T* AllocateElements(size_t Count)
	{
		return static_cast<T*>(CMemoryManager::GetInstance().Allocate(sizeof(T) * Count, alignof(T)));
	}

	template <typename TType>
	static void DeallocateElements(T* Ptr, size_t Count)
	{
		(void)Count; // 参数未使用，但保持接口兼容性
		CMemoryManager::GetInstance().Deallocate(Ptr);
	}

	// 对象类型的GC注册
	template <typename TType>
	static void RegisterWithGC(T* Ptr, size_t Count)
	{
		if constexpr (std::is_base_of_v<CObject, T>)
		{
			for (size_t i = 0; i < Count; ++i)
			{
				CGarbageCollector::GetInstance().RegisterObject(&Ptr[i]);
			}
		}
	}

	template <typename TType>
	static void UnregisterFromGC(T* Ptr, size_t Count)
	{
		if constexpr (std::is_base_of_v<CObject, T>)
		{
			for (size_t i = 0; i < Count; ++i)
			{
				CGarbageCollector::GetInstance().UnregisterObject(&Ptr[i]);
			}
		}
	}

	// 智能构造和析构
	template <typename TType>
	static void ConstructElements(T* Ptr, size_t Count)
	{
		if constexpr (std::is_trivially_constructible_v<T>)
		{
			memset(Ptr, 0, sizeof(T) * Count);
		}
		else
		{
			for (size_t i = 0; i < Count; ++i)
			{
				new (Ptr + i) T();
			}
		}
	}

	template <typename TType>
	static void ConstructElements(T* Ptr, size_t Count, const T& Value)
	{
		for (size_t i = 0; i < Count; ++i)
		{
			new (Ptr + i) T(Value);
		}
	}

	template <typename TType>
	static void DestructElements(T* Ptr, size_t Count)
	{
		if constexpr (!std::is_trivially_destructible_v<T>)
		{
			for (size_t i = 0; i < Count; ++i)
			{
				Ptr[i].~T();
			}
		}
	}

	// 内存移动和复制
	template <typename TType>
	static void MoveElements(T* Dest, T* Src, size_t Count)
	{
		if constexpr (std::is_trivially_move_constructible_v<T>)
		{
			memmove(Dest, Src, sizeof(T) * Count);
		}
		else
		{
			if (Dest < Src)
			{
				for (size_t i = 0; i < Count; ++i)
				{
					new (Dest + i) T(std::move(Src[i]));
					Src[i].~T();
				}
			}
			else
			{
				for (size_t i = Count; i > 0; --i)
				{
					new (Dest + i - 1) T(std::move(Src[i - 1]));
					Src[i - 1].~T();
				}
			}
		}
	}

	template <typename TType>
	static void CopyElements(T* Dest, const T* Src, size_t Count)
	{
		if constexpr (std::is_trivially_copy_constructible_v<T>)
		{
			memcpy(Dest, Src, sizeof(T) * Count);
		}
		else
		{
			for (size_t i = 0; i < Count; ++i)
			{
				new (Dest + i) T(Src[i]);
			}
		}
	}

	// 容量计算工具
	static size_t CalculateGrowth(size_t Current, size_t Requested)
	{
		if (Requested <= Current)
		{
			return Current;
		}

		// 使用1.5倍增长策略
		size_t Growth = Current + (Current / 2);
		return Growth > Requested ? Growth : Requested;
	}
};

// 迭代器基类
template <typename TType, typename TContainerType>
class CIteratorBase
{
public:
	using ValueType = T;
	using Pointer = T*;
	using Reference = T&;
	using ConstReference = const T&;

	CIteratorBase()
	    : Ptr(nullptr)
	{}
	explicit CIteratorBase(Pointer InPtr)
	    : Ptr(InPtr)
	{}

	Reference operator*()
	{
		return *Ptr;
	}
	ConstReference operator*() const
	{
		return *Ptr;
	}
	Pointer operator->()
	{
		return Ptr;
	}

	Pointer operator->() const
	{
		return Ptr;
	}

	bool operator==(const CIteratorBase& Other) const
	{
		return Ptr == Other.Ptr;
	}
	bool operator!=(const CIteratorBase& Other) const
	{
		return Ptr != Other.Ptr;
	}

	CIteratorBase& operator++()
	{
		++Ptr;
		return *this;
	}
	CIteratorBase operator++(int)
	{
		CIteratorBase Temp(*this);
		++Ptr;
		return Temp;
	}

	CIteratorBase& operator--()
	{
		--Ptr;
		return *this;
	}
	CIteratorBase operator--(int)
	{
		CIteratorBase Temp(*this);
		--Ptr;
		return Temp;
	}

	CIteratorBase operator+(ptrdiff_t n) const
	{
		return CIteratorBase(Ptr + n);
	}
	CIteratorBase operator-(ptrdiff_t n) const
	{
		return CIteratorBase(Ptr - n);
	}

	ptrdiff_t operator-(const CIteratorBase& Other) const
	{
		return Ptr - Other.Ptr;
	}

	bool operator<(const CIteratorBase& Other) const
	{
		return Ptr < Other.Ptr;
	}
	bool operator<=(const CIteratorBase& Other) const
	{
		return Ptr <= Other.Ptr;
	}
	bool operator>(const CIteratorBase& Other) const
	{
		return Ptr > Other.Ptr;
	}
	bool operator>=(const CIteratorBase& Other) const
	{
		return Ptr >= Other.Ptr;
	}

	// 提供访问内部指针的方法
	Pointer GetPtr() const
	{
		return Ptr;
	}

protected:
	Pointer Ptr;
};

// 哈希函数特化
template <typename TType>
struct NHash
{
	size_t operator()(const T& Value) const
	{
		if constexpr (std::is_integral_v<T>)
		{
			return static_cast<size_t>(Value);
		}
		else if constexpr (std::is_pointer_v<T>)
		{
			return reinterpret_cast<size_t>(Value);
		}
		else if constexpr (std::is_base_of_v<CObject, T>)
		{
			return Value.GetHashCode();
		}
		else
		{
			// 通用哈希函数
			return DefaultHash(reinterpret_cast<const uint8_t*>(&Value), sizeof(T));
		}
	}

private:
	static size_t DefaultHash(const uint8_t* data, size_t length)
	{
		// FNV-1a 哈希算法
		constexpr size_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
		constexpr size_t FNV_PRIME = 1099511628211ULL;

		size_t Hash = FNV_OFFSET_BASIS;
		for (size_t i = 0; i < length; ++i)
		{
			Hash ^= static_cast<size_t>(data[i]);
			Hash *= FNV_PRIME;
		}
		return Hash;
	}
};

// 相等比较函数特化
template <typename TType>
struct NEqual
{
	bool operator()(const T& left, const T& right) const
	{
		if constexpr (std::is_base_of_v<CObject, T>)
		{
			return left.Equals(&right);
		}
		else
		{
			return left == right;
		}
	}
};
} // namespace NLib
