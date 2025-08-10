#pragma once

#include "Core/SmartPointers.h"
#include "Logging/LogCategory.h"
#include "TContainer.h"

#include <algorithm>
#include <cstdarg>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>

namespace NLib
{
/**
 * @brief TString - 高性能字符串容器
 *
 * 专为Nut引擎优化的字符串类，支持UTF-8编码
 * 特性：
 * - Small String Optimization (SSO)
 * - Copy-on-Write (COW) 语义
 * - 高效的字符串操作
 * - 与std::string兼容的接口
 * - 自定义内存管理
 */
template <typename TCharType = char, typename TAllocator = CMemoryManager>
class TString : public TSequenceContainer<TCharType, TAllocator>
{
public:
	using CharType = TCharType;
	using BaseType = TSequenceContainer<TCharType, TAllocator>;
	using typename BaseType::AllocatorType;
	using typename BaseType::ConstPointer;
	using typename BaseType::ConstReference;
	using typename BaseType::DifferenceType;
	using typename BaseType::ElementType;
	using typename BaseType::Pointer;
	using typename BaseType::Reference;
	using typename BaseType::SizeType;

	// 字符串特有的类型
	using StringView = std::basic_string_view<CharType>;
	using StdString = std::basic_string<CharType>;
	using Iterator = Pointer;
	using ConstIterator = ConstPointer;
	using ReverseIterator = std::reverse_iterator<Iterator>;
	using ConstReverseIterator = std::reverse_iterator<ConstIterator>;

	static constexpr SizeType NoPosition = static_cast<SizeType>(-1);

private:
	// Small String Optimization 的阈值
	static constexpr SizeType SSOThreshold = (sizeof(void*) + sizeof(SizeType) * 2) / sizeof(CharType) - 1;

	/**
	 * @brief 字符串数据结构
	 */
	struct alignas(alignof(std::max_align_t)) SStringData
	{
		SizeType Size;                 // 字符串长度（不包含null terminator）
		SizeType Capacity;             // 容量
		std::atomic<int32_t> RefCount; // 引用计数（COW）
		
		// 确保Data数组有正确的对齐
		alignas(alignof(CharType)) CharType Data[1]; // 柔性数组

		SStringData(SizeType InSize, SizeType InCapacity)
		    : Size(InSize),
		      Capacity(InCapacity),
		      RefCount(1)
		{
			Data[Size] = CharType(0); // null terminator
		}
	};

	union UStringStorage
	{
		// 小字符串优化
		struct
		{
			CharType Buffer[SSOThreshold + 1];
			uint8_t Size; // 实际大小，最高位表示是否使用SSO
		} SSO;

		// 大字符串
		struct
		{
			SStringData* Data;
			SizeType Size;
			SizeType Capacity;
		} Heap;

		UStringStorage()
		{
			SSO.Size = 0x80; // 设置SSO标志位
			SSO.Buffer[0] = CharType(0);
		}
	};

	UStringStorage Storage;
	AllocatorType* Allocator;

public:
	// === 构造函数和析构函数 ===

	/**
	 * @brief 默认构造函数
	 */
	TString()
	    : Storage(),
	      Allocator(&GetDefaultAllocator())
	{
		NLOG(LogCore, Debug, "TString default constructed");
	}

	/**
	 * @brief C字符串构造函数
	 * @param CStr C字符串
	 */
	TString(const CharType* CStr)
	    : TString()
	{
		if (CStr)
		{
			SizeType Len = StringLength(CStr);
			Assign(CStr, Len);
		}
		NLOG(LogCore, Debug, "TString constructed from C string of length {}", Size());
	}

	/**
	 * @brief C字符串和长度构造函数
	 * @param CStr C字符串
	 * @param Length 长度
	 */
	TString(const CharType* CStr, SizeType Length)
	    : TString()
	{
		if (CStr && Length > 0)
		{
			Assign(CStr, Length);
		}
		NLOG(LogCore, Debug, "TString constructed from C string with length {}", Length);
	}

	/**
	 * @brief 填充构造函数
	 * @param Count 字符数量
	 * @param Ch 填充字符
	 */
	TString(SizeType Count, CharType Ch)
	    : TString()
	{
		Assign(Count, Ch);
		NLOG(LogCore, Debug, "TString constructed with {} characters", Count);
	}

	/**
	 * @brief std::string构造函数
	 * @param StdStr std::string
	 */
	TString(const StdString& StdStr)
	    : TString(StdStr.c_str(), StdStr.size())
	{
		NLOG(LogCore, Debug, "TString constructed from std::string");
	}

	/**
	 * @brief string_view构造函数
	 * @param View string_view
	 */
	TString(const StringView& View)
	    : TString(View.data(), View.size())
	{
		NLOG(LogCore, Debug, "TString constructed from string_view");
	}

	/**
	 * @brief 拷贝构造函数（COW语义）
	 * @param Other 其他字符串
	 */
	TString(const TString& Other)
	    : Storage(),
	      Allocator(Other.Allocator)
	{
		if (Other.IsSSO())
		{
			Storage = Other.Storage;
		}
		else
		{
			// COW: 增加引用计数
			SStringData* Data = Other.Storage.Heap.Data;
			Data->RefCount.fetch_add(1);
			Storage.Heap = Other.Storage.Heap;
		}
		NLOG(LogCore, Debug, "TString copy constructed");
	}

	/**
	 * @brief 移动构造函数
	 * @param Other 要移动的字符串
	 */
	TString(TString&& Other) noexcept
	    : Storage(Other.Storage),
	      Allocator(Other.Allocator)
	{
		Other.Storage = UStringStorage{};
		NLOG(LogCore, Debug, "TString move constructed");
	}

	/**
	 * @brief 拷贝赋值操作符
	 * @param Other 其他字符串
	 * @return 字符串引用
	 */
	TString& operator=(const TString& Other)
	{
		if (this != &Other)
		{
			Release();

			Allocator = Other.Allocator;
			if (Other.IsSSO())
			{
				Storage = Other.Storage;
			}
			else
			{
				// COW: 增加引用计数
				SStringData* Data = Other.Storage.Heap.Data;
				Data->RefCount.fetch_add(1);
				Storage.Heap = Other.Storage.Heap;
			}

			NLOG(LogCore, Debug, "TString copy assigned");
		}
		return *this;
	}

	/**
	 * @brief 移动赋值操作符
	 * @param Other 要移动的字符串
	 * @return 字符串引用
	 */
	TString& operator=(TString&& Other) noexcept
	{
		if (this != &Other)
		{
			Release();

			Storage = Other.Storage;
			Allocator = Other.Allocator;

			Other.Storage = UStringStorage{};

			NLOG(LogCore, Debug, "TString move assigned");
		}
		return *this;
	}

	/**
	 * @brief C字符串赋值操作符
	 * @param CStr C字符串
	 * @return 字符串引用
	 */
	TString& operator=(const CharType* CStr)
	{
		if (CStr)
		{
			SizeType Len = StringLength(CStr);
			Assign(CStr, Len);
		}
		else
		{
			Clear();
		}
		return *this;
	}

	/**
	 * @brief std::string赋值操作符
	 * @param StdStr std::string
	 * @return 字符串引用
	 */
	TString& operator=(const StdString& StdStr)
	{
		Assign(StdStr.c_str(), StdStr.size());
		return *this;
	}

	/**
	 * @brief 析构函数
	 */
	~TString()
	{
		Release();
		NLOG(LogCore, Debug, "TString destroyed");
	}

public:
	// === TContainer接口实现 ===

	SizeType Size() const override
	{
		return IsSSO() ? (Storage.SSO.Size & 0x7F) : Storage.Heap.Size;
	}

	SizeType MaxSize() const override
	{
		return std::numeric_limits<SizeType>::max() / sizeof(CharType) - 1;
	}

	void Clear() override
	{
		if (IsSSO())
		{
			Storage.SSO.Size = 0x80;
			Storage.SSO.Buffer[0] = CharType(0);
		}
		else
		{
			Release();
			Storage = UStringStorage{};
		}
		NLOG(LogCore, Debug, "TString cleared");
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
		if (IsSSO())
		{
			return sizeof(UStringStorage);
		}
		else
		{
			return sizeof(SStringData) + Storage.Heap.Capacity * sizeof(CharType);
		}
	}

	void ShrinkToFit() override
	{
		if (!IsSSO() && Storage.Heap.Size < Storage.Heap.Capacity)
		{
			if (Storage.Heap.Size <= SSOThreshold)
			{
				// 转换为SSO
				CharType SSOBuffer[SSOThreshold + 1];
				SizeType SSOSize = Storage.Heap.Size;
				std::memcpy(SSOBuffer, Storage.Heap.Data->Data, SSOSize * sizeof(CharType));
				SSOBuffer[SSOSize] = CharType(0);

				Release();

				Storage.SSO.Size = static_cast<uint8_t>(SSOSize) | 0x80;
				std::memcpy(Storage.SSO.Buffer, SSOBuffer, (SSOSize + 1) * sizeof(CharType));
			}
			else
			{
				// 重新分配合适大小的内存
				EnsureUnique();
				ReallocateHeap(Storage.Heap.Size);
			}
		}
		NLOG(LogCore, Debug, "TString shrunk to fit");
	}

	bool Equals(const TContainer<CharType, TAllocator>& Other) const override
	{
		const TString* OtherString = dynamic_cast<const TString*>(&Other);
		if (!OtherString)
		{
			return false;
		}

		return Compare(*OtherString) == 0;
	}

	size_t GetHashCode() const override
	{
		size_t Hash = 0;
		const CharType* Data = GetData();
		SizeType Length = Size();

		for (SizeType i = 0; i < Length; ++i)
		{
			Hash = Hash * 31 + static_cast<size_t>(Data[i]);
		}

		return Hash;
	}

	bool Validate() const override
	{
		bool bValid = true;

		if (IsSSO())
		{
			SizeType SSOSize = Storage.SSO.Size & 0x7F;
			if (SSOSize > SSOThreshold)
			{
				NLOG(LogCore, Error, "TString validation failed: SSO size {} > threshold {}", SSOSize, SSOThreshold);
				bValid = false;
			}

			if (Storage.SSO.Buffer[SSOSize] != CharType(0))
			{
				NLOG(LogCore, Error, "TString validation failed: SSO buffer not null-terminated");
				bValid = false;
			}
		}
		else
		{
			if (!Storage.Heap.Data)
			{
				NLOG(LogCore, Error, "TString validation failed: heap data is null");
				bValid = false;
			}
			else
			{
				if (Storage.Heap.Size > Storage.Heap.Capacity)
				{
					NLOG(LogCore,
					     Error,
					     "TString validation failed: heap size {} > capacity {}",
					     Storage.Heap.Size,
					     Storage.Heap.Capacity);
					bValid = false;
				}

				if (Storage.Heap.Data->Size != Storage.Heap.Size)
				{
					NLOG(LogCore,
					     Error,
					     "TString validation failed: inconsistent sizes {} != {}",
					     Storage.Heap.Data->Size,
					     Storage.Heap.Size);
					bValid = false;
				}

				if (Storage.Heap.Data->Data[Storage.Heap.Size] != CharType(0))
				{
					NLOG(LogCore, Error, "TString validation failed: heap buffer not null-terminated");
					bValid = false;
				}
			}
		}

		return bValid;
	}

	typename BaseType::SContainerStats GetStats() const override
	{
		typename BaseType::SContainerStats Stats;
		Stats.ElementCount = Size();
		Stats.Capacity = Capacity();
		Stats.MemoryUsage = GetMemoryUsage();
		Stats.WastedMemory = (Stats.Capacity - Stats.ElementCount) * sizeof(CharType);
		Stats.LoadFactor = Stats.Capacity > 0 ? static_cast<double>(Stats.ElementCount) / Stats.Capacity : 0.0;
		return Stats;
	}

public:
	// === TSequenceContainer接口实现 ===

	Reference At(SizeType Index) override
	{
		BaseType::CheckIndex(Index, Size());
		EnsureUnique();
		return GetMutableData()[Index];
	}

	ConstReference At(SizeType Index) const override
	{
		BaseType::CheckIndex(Index, Size());
		return GetData()[Index];
	}

	Reference operator[](SizeType Index) override
	{
		EnsureUnique();
		return GetMutableData()[Index];
	}

	ConstReference operator[](SizeType Index) const override
	{
		return GetData()[Index];
	}

	Reference Front() override
	{
		if (Size() == 0)
		{
			NLOG(LogCore, Error, "TString::Front() called on empty string");
			throw std::runtime_error("Front() called on empty TString");
		}
		EnsureUnique();
		return GetMutableData()[0];
	}

	ConstReference Front() const override
	{
		if (Size() == 0)
		{
			NLOG(LogCore, Error, "TString::Front() called on empty string");
			throw std::runtime_error("Front() called on empty TString");
		}
		return GetData()[0];
	}

	Reference Back() override
	{
		SizeType Length = Size();
		if (Length == 0)
		{
			NLOG(LogCore, Error, "TString::Back() called on empty string");
			throw std::runtime_error("Back() called on empty TString");
		}
		EnsureUnique();
		return GetMutableData()[Length - 1];
	}

	ConstReference Back() const override
	{
		SizeType Length = Size();
		if (Length == 0)
		{
			NLOG(LogCore, Error, "TString::Back() called on empty string");
			throw std::runtime_error("Back() called on empty TString");
		}
		return GetData()[Length - 1];
	}

	void PushBack(const CharType& Ch) override
	{
		Append(1, Ch);
	}

	void PushBack(CharType&& Ch) override
	{
		Append(1, Ch);
	}

	void PopBack() override
	{
		SizeType Length = Size();
		if (Length == 0)
		{
			NLOG(LogCore, Error, "TString::PopBack() called on empty string");
			throw std::runtime_error("PopBack() called on empty TString");
		}
		Resize(Length - 1);
	}

	void Insert(SizeType Index, const CharType& Ch) override
	{
		Insert(Index, 1, Ch);
	}

	void Insert(SizeType Index, CharType&& Ch) override
	{
		Insert(Index, 1, Ch);
	}

	void RemoveAt(SizeType Index) override
	{
		Erase(Index, 1);
	}

	void RemoveRange(SizeType StartIndex, SizeType Count) override
	{
		Erase(StartIndex, Count);
	}

public:
	// === 字符串特有操作 ===

	/**
	 * @brief 获取数据指针（公有访问）
	 * @return 数据指针
	 */
	const CharType* GetData() const
	{
		return IsSSO() ? Storage.SSO.Buffer : Storage.Heap.Data->Data;
	}

	/**
	 * @brief 获取C字符串指针
	 * @return C字符串指针
	 */
	const CharType* CStr() const
	{
		return GetData();
	}

	/**
	 * @brief 获取字符串容量
	 * @return 容量
	 */
	SizeType Capacity() const
	{
		return IsSSO() ? SSOThreshold : Storage.Heap.Capacity;
	}

	/**
	 * @brief 检查字符串是否为空
	 * @return true if empty, false otherwise
	 */
	bool IsEmpty() const override
	{
		return Size() == 0;
	}

	/**
	 * @brief 预留容量
	 * @param NewCapacity 新容量
	 */
	void Reserve(SizeType NewCapacity)
	{
		if (NewCapacity > Capacity())
		{
			if (NewCapacity <= SSOThreshold && !IsSSO())
			{
				// 不需要分配堆内存
				return;
			}

			if (IsSSO() && NewCapacity > SSOThreshold)
			{
				// 从SSO转换到堆
				ConvertToHeap(NewCapacity);
			}
			else if (!IsSSO())
			{
				// 在堆上扩容
				EnsureUnique();
				ReallocateHeap(NewCapacity);
			}
		}
	}

	/**
	 * @brief 调整字符串大小
	 * @param NewSize 新大小
	 * @param Ch 填充字符
	 */
	void Resize(SizeType NewSize, CharType Ch = CharType(0))
	{
		SizeType CurrentSize = Size();

		if (NewSize > CurrentSize)
		{
			// 扩大字符串
			Reserve(NewSize);
			EnsureUnique();

			CharType* Data = GetMutableData();
			for (SizeType i = CurrentSize; i < NewSize; ++i)
			{
				Data[i] = Ch;
			}
			Data[NewSize] = CharType(0);

			SetSize(NewSize);
		}
		else if (NewSize < CurrentSize)
		{
			// 缩小字符串
			EnsureUnique();
			CharType* Data = GetMutableData();
			Data[NewSize] = CharType(0);
			SetSize(NewSize);
		}
	}

	/**
	 * @brief 赋值操作
	 * @param CStr C字符串
	 * @param Length 长度
	 */
	void Assign(const CharType* CStr, SizeType Length)
	{
		if (Length <= SSOThreshold)
		{
			// 使用SSO
			Release();

			Storage.SSO.Size = static_cast<uint8_t>(Length) | 0x80;
			if (CStr && Length > 0)
			{
				std::memcpy(Storage.SSO.Buffer, CStr, Length * sizeof(CharType));
			}
			Storage.SSO.Buffer[Length] = CharType(0);
		}
		else
		{
			// 使用堆分配
			Release();
			AllocateHeap(Length);

			if (CStr && Length > 0)
			{
				std::memcpy(Storage.Heap.Data->Data, CStr, Length * sizeof(CharType));
			}
			Storage.Heap.Data->Data[Length] = CharType(0);
			Storage.Heap.Data->Size = Length;
			Storage.Heap.Size = Length;
		}
	}

	/**
	 * @brief 填充赋值
	 * @param Count 字符数量
	 * @param Ch 字符
	 */
	void Assign(SizeType Count, CharType Ch)
	{
		if (Count <= SSOThreshold)
		{
			Release();

			Storage.SSO.Size = static_cast<uint8_t>(Count) | 0x80;
			for (SizeType i = 0; i < Count; ++i)
			{
				Storage.SSO.Buffer[i] = Ch;
			}
			Storage.SSO.Buffer[Count] = CharType(0);
		}
		else
		{
			Release();
			AllocateHeap(Count);

			for (SizeType i = 0; i < Count; ++i)
			{
				Storage.Heap.Data->Data[i] = Ch;
			}
			Storage.Heap.Data->Data[Count] = CharType(0);
			Storage.Heap.Data->Size = Count;
			Storage.Heap.Size = Count;
		}
	}

	/**
	 * @brief 追加字符串
	 * @param CStr C字符串
	 * @param Length 长度
	 */
	void Append(const CharType* CStr, SizeType Length)
	{
		if (!CStr || Length == 0)
		{
			return;
		}

		SizeType CurrentSize = Size();
		SizeType NewSize = CurrentSize + Length;

		Reserve(NewSize);
		EnsureUnique();

		CharType* Data = GetMutableData();
		std::memcpy(Data + CurrentSize, CStr, Length * sizeof(CharType));
		Data[NewSize] = CharType(0);

		SetSize(NewSize);
	}

	/**
	 * @brief 追加字符
	 * @param Count 字符数量
	 * @param Ch 字符
	 */
	void Append(SizeType Count, CharType Ch)
	{
		if (Count == 0)
		{
			return;
		}

		SizeType CurrentSize = Size();
		SizeType NewSize = CurrentSize + Count;

		Reserve(NewSize);
		EnsureUnique();

		CharType* Data = GetMutableData();
		for (SizeType i = 0; i < Count; ++i)
		{
			Data[CurrentSize + i] = Ch;
		}
		Data[NewSize] = CharType(0);

		SetSize(NewSize);
	}

	/**
	 * @brief 插入字符串
	 * @param Index 插入位置
	 * @param CStr C字符串
	 * @param Length 长度
	 */
	void Insert(SizeType Index, const CharType* CStr, SizeType Length)
	{
		if (!CStr || Length == 0)
		{
			return;
		}

		SizeType CurrentSize = Size();
		if (Index > CurrentSize)
		{
			BaseType::CheckIndex(Index, CurrentSize + 1);
		}

		SizeType NewSize = CurrentSize + Length;
		Reserve(NewSize);
		EnsureUnique();

		CharType* Data = GetMutableData();

		// 移动后续字符
		if (Index < CurrentSize)
		{
			std::memmove(Data + Index + Length, Data + Index, (CurrentSize - Index) * sizeof(CharType));
		}

		// 插入新字符
		std::memcpy(Data + Index, CStr, Length * sizeof(CharType));
		Data[NewSize] = CharType(0);

		SetSize(NewSize);
	}

	/**
	 * @brief 插入字符
	 * @param Index 插入位置
	 * @param Count 字符数量
	 * @param Ch 字符
	 */
	void Insert(SizeType Index, SizeType Count, CharType Ch)
	{
		if (Count == 0)
		{
			return;
		}

		SizeType CurrentSize = Size();
		if (Index > CurrentSize)
		{
			BaseType::CheckIndex(Index, CurrentSize + 1);
		}

		SizeType NewSize = CurrentSize + Count;
		Reserve(NewSize);
		EnsureUnique();

		CharType* Data = GetMutableData();

		// 移动后续字符
		if (Index < CurrentSize)
		{
			std::memmove(Data + Index + Count, Data + Index, (CurrentSize - Index) * sizeof(CharType));
		}

		// 插入新字符
		for (SizeType i = 0; i < Count; ++i)
		{
			Data[Index + i] = Ch;
		}
		Data[NewSize] = CharType(0);

		SetSize(NewSize);
	}

	/**
	 * @brief 删除子字符串
	 * @param Index 开始位置
	 * @param Count 删除数量
	 */
	void Erase(SizeType Index, SizeType Count = NoPosition)
	{
		SizeType CurrentSize = Size();
		BaseType::CheckIndex(Index, CurrentSize);

		if (Count == NoPosition || Index + Count > CurrentSize)
		{
			Count = CurrentSize - Index;
		}

		if (Count == 0)
		{
			return;
		}

		EnsureUnique();

		CharType* Data = GetMutableData();
		SizeType NewSize = CurrentSize - Count;

		// 移动后续字符
		if (Index + Count < CurrentSize)
		{
			std::memmove(Data + Index, Data + Index + Count, (CurrentSize - Index - Count) * sizeof(CharType));
		}

		Data[NewSize] = CharType(0);
		SetSize(NewSize);
	}

	/**
	 * @brief 查找子字符串
	 * @param SubStr 子字符串
	 * @param StartIndex 开始位置
	 * @return 找到的位置，未找到返回NoPosition
	 */
	SizeType Find(const TString& SubStr, SizeType StartIndex = 0) const
	{
		return Find(SubStr.CStr(), StartIndex, SubStr.Size());
	}

	/**
	 * @brief 查找C字符串
	 * @param CStr C字符串
	 * @param StartIndex 开始位置
	 * @param Length 长度
	 * @return 找到的位置，未找到返回NoPosition
	 */
	SizeType Find(const CharType* CStr, SizeType StartIndex = 0, SizeType Length = NoPosition) const
	{
		if (!CStr)
		{
			return NoPosition;
		}

		if (Length == NoPosition)
		{
			Length = StringLength(CStr);
		}

		SizeType CurrentSize = Size();
		if (StartIndex >= CurrentSize || Length == 0)
		{
			return NoPosition;
		}

		const CharType* Data = GetData();
		const CharType* Found = std::search(Data + StartIndex, Data + CurrentSize, CStr, CStr + Length);

		return (Found != Data + CurrentSize) ? static_cast<SizeType>(Found - Data) : NoPosition;
	}

	/**
	 * @brief 查找字符
	 * @param Ch 字符
	 * @param StartIndex 开始位置
	 * @return 找到的位置，未找到返回NoPosition
	 */
	SizeType Find(CharType Ch, SizeType StartIndex = 0) const
	{
		SizeType CurrentSize = Size();
		if (StartIndex >= CurrentSize)
		{
			return NoPosition;
		}

		const CharType* Data = GetData();
		const CharType* Found = std::find(Data + StartIndex, Data + CurrentSize, Ch);

		return (Found != Data + CurrentSize) ? static_cast<SizeType>(Found - Data) : NoPosition;
	}

	/**
	 * @brief 反向查找
	 * @param SubStr 子字符串
	 * @param StartIndex 开始位置
	 * @return 找到的位置，未找到返回NoPosition
	 */
	SizeType RFind(const TString& SubStr, SizeType StartIndex = NoPosition) const
	{
		return RFind(SubStr.CStr(), StartIndex, SubStr.Size());
	}

	/**
	 * @brief 反向查找C字符串
	 * @param CStr C字符串
	 * @param StartIndex 开始位置
	 * @param Length 长度
	 * @return 找到的位置，未找到返回NoPosition
	 */
	SizeType RFind(const CharType* CStr, SizeType StartIndex = NoPosition, SizeType Length = NoPosition) const
	{
		if (!CStr)
		{
			return NoPosition;
		}

		if (Length == NoPosition)
		{
			Length = StringLength(CStr);
		}

		SizeType CurrentSize = Size();
		if (Length == 0 || Length > CurrentSize)
		{
			return NoPosition;
		}

		if (StartIndex == NoPosition || StartIndex >= CurrentSize)
		{
			StartIndex = CurrentSize - 1;
		}

		const CharType* Data = GetData();

		// 从StartIndex开始向前搜索
		for (SizeType i = std::min(StartIndex, CurrentSize - Length); i != NoPosition; --i)
		{
			if (std::memcmp(Data + i, CStr, Length * sizeof(CharType)) == 0)
			{
				return i;
			}

			if (i == 0)
				break; // 防止无符号整数下溢
		}

		return NoPosition;
	}

	/**
	 * @brief 获取子字符串
	 * @param StartIndex 开始位置
	 * @param Count 字符数量
	 * @return 子字符串
	 */
	TString SubString(SizeType StartIndex, SizeType Count = NoPosition) const
	{
		SizeType CurrentSize = Size();
		BaseType::CheckIndex(StartIndex, CurrentSize + 1);

		if (Count == NoPosition || StartIndex + Count > CurrentSize)
		{
			Count = CurrentSize - StartIndex;
		}

		return TString(GetData() + StartIndex, Count);
	}

	/**
	 * @brief 比较字符串
	 * @param Other 其他字符串
	 * @return 比较结果（-1: 小于, 0: 等于, 1: 大于）
	 */
	int Compare(const TString& Other) const
	{
		SizeType ThisSize = Size();
		SizeType OtherSize = Other.Size();
		SizeType MinSize = std::min(ThisSize, OtherSize);

		int Result = std::memcmp(GetData(), Other.GetData(), MinSize * sizeof(CharType));
		if (Result == 0)
		{
			if (ThisSize < OtherSize)
				return -1;
			if (ThisSize > OtherSize)
				return 1;
			return 0;
		}

		return Result < 0 ? -1 : 1;
	}

	/**
	 * @brief 比较C字符串
	 * @param CStr C字符串
	 * @return 比较结果
	 */
	int Compare(const CharType* CStr) const
	{
		if (!CStr)
		{
			return Size() == 0 ? 0 : 1;
		}

		SizeType CStrLength = StringLength(CStr);
		SizeType ThisSize = Size();
		SizeType MinSize = std::min(ThisSize, CStrLength);

		int Result = std::memcmp(GetData(), CStr, MinSize * sizeof(CharType));
		if (Result == 0)
		{
			if (ThisSize < CStrLength)
				return -1;
			if (ThisSize > CStrLength)
				return 1;
			return 0;
		}

		return Result < 0 ? -1 : 1;
	}

	/**
	 * @brief 检查字符串是否包含子字符串
	 * @param SubStr 要查找的子字符串
	 * @return 是否包含
	 */
	bool Contains(const TString& SubStr) const
	{
		return Find(SubStr) != NoPosition;
	}

	/**
	 * @brief 检查字符串是否包含C字符串
	 * @param CStr 要查找的C字符串
	 * @return 是否包含
	 */
	bool Contains(const CharType* CStr) const
	{
		return Find(CStr) != NoPosition;
	}

	/**
	 * @brief 检查字符串是否包含字符
	 * @param Ch 要查找的字符
	 * @return 是否包含
	 */
	bool Contains(CharType Ch) const
	{
		return Find(Ch) != NoPosition;
	}

	/**
	 * @brief 转换为小写字符串
	 * @return 小写字符串
	 */
	TString ToLower() const
	{
		TString Result;
		Result.Reserve(Size());
		
		const CharType* Data = GetData();
		SizeType CurrentSize = Size();
		
		for (SizeType i = 0; i < CurrentSize; ++i)
		{
			CharType Ch = Data[i];
			if (Ch >= 'A' && Ch <= 'Z')
			{
				Ch = Ch - 'A' + 'a';
			}
			Result.PushBack(Ch);
		}
		
		return Result;
	}

	/**
	 * @brief 转换为大写字符串
	 * @return 大写字符串
	 */
	TString ToUpper() const
	{
		TString Result;
		Result.Reserve(Size());
		
		const CharType* Data = GetData();
		SizeType CurrentSize = Size();
		
		for (SizeType i = 0; i < CurrentSize; ++i)
		{
			CharType Ch = Data[i];
			if (Ch >= 'a' && Ch <= 'z')
			{
				Ch = Ch - 'a' + 'A';
			}
			Result.PushBack(Ch);
		}
		
		return Result;
	}

	/**
	 * @brief 检查字符串是否以指定子字符串开始
	 * @param Prefix 前缀字符串
	 * @return 是否以该前缀开始
	 */
	bool StartsWith(const TString& Prefix) const
	{
		return StartsWith(Prefix.CStr());
	}

	/**
	 * @brief 检查字符串是否以指定C字符串开始
	 * @param Prefix 前缀C字符串
	 * @return 是否以该前缀开始
	 */
	bool StartsWith(const CharType* Prefix) const
	{
		if (!Prefix)
		{
			return true;
		}
		
		SizeType PrefixLength = StringLength(Prefix);
		if (PrefixLength > Size())
		{
			return false;
		}
		
		return std::memcmp(GetData(), Prefix, PrefixLength * sizeof(CharType)) == 0;
	}

	/**
	 * @brief 检查字符串是否以指定字符开始
	 * @param Ch 前缀字符
	 * @return 是否以该字符开始
	 */
	bool StartsWith(CharType Ch) const
	{
		return Size() > 0 && GetData()[0] == Ch;
	}

	/**
	 * @brief 检查字符串是否以指定子字符串结束
	 * @param Suffix 后缀字符串
	 * @return 是否以该后缀结束
	 */
	bool EndsWith(const TString& Suffix) const
	{
		return EndsWith(Suffix.CStr());
	}

	/**
	 * @brief 检查字符串是否以指定C字符串结束
	 * @param Suffix 后缀C字符串
	 * @return 是否以该后缀结束
	 */
	bool EndsWith(const CharType* Suffix) const
	{
		if (!Suffix)
		{
			return true;
		}
		
		SizeType SuffixLength = StringLength(Suffix);
		SizeType CurrentSize = Size();
		
		if (SuffixLength > CurrentSize)
		{
			return false;
		}
		
		return std::memcmp(GetData() + CurrentSize - SuffixLength, Suffix, SuffixLength * sizeof(CharType)) == 0;
	}

	/**
	 * @brief 检查字符串是否以指定字符结束
	 * @param Ch 后缀字符
	 * @return 是否以该字符结束
	 */
	bool EndsWith(CharType Ch) const
	{
		SizeType CurrentSize = Size();
		return CurrentSize > 0 && GetData()[CurrentSize - 1] == Ch;
	}

	/**
	 * @brief 获取字符串长度（兼容方法）
	 * @return 字符串长度
	 */
	SizeType Length() const
	{
		return Size();
	}

	/**
	 * @brief 查找字符的索引位置
	 * @param Ch 要查找的字符
	 * @param StartIndex 开始位置
	 * @return 字符的索引位置，未找到返回NoPosition
	 */
	SizeType IndexOf(CharType Ch, SizeType StartIndex = 0) const
	{
		return Find(Ch, StartIndex);
	}

	/**
	 * @brief 查找子字符串的索引位置
	 * @param SubStr 要查找的子字符串
	 * @param StartIndex 开始位置
	 * @return 子字符串的索引位置，未找到返回NoPosition
	 */
	SizeType IndexOf(const TString& SubStr, SizeType StartIndex = 0) const
	{
		return Find(SubStr, StartIndex);
	}

	/**
	 * @brief 查找C字符串的索引位置
	 * @param CStr 要查找的C字符串
	 * @param StartIndex 开始位置
	 * @return C字符串的索引位置，未找到返回NoPosition
	 */
	SizeType IndexOf(const CharType* CStr, SizeType StartIndex = 0) const
	{
		return Find(CStr, StartIndex);
	}

	/**
	 * @brief 替换字符串中的所有匹配项
	 * @param OldValue 要替换的子字符串
	 * @param NewValue 替换为的字符串
	 * @return 替换后的字符串
	 */
	TString Replace(const TString& OldValue, const TString& NewValue) const
	{
		return Replace(OldValue.CStr(), NewValue.CStr());
	}

	/**
	 * @brief 替换字符串中的所有匹配项
	 * @param OldValue 要替换的C字符串
	 * @param NewValue 替换为的C字符串
	 * @return 替换后的字符串
	 */
	TString Replace(const CharType* OldValue, const CharType* NewValue) const
	{
		if (!OldValue || !NewValue)
		{
			return *this;
		}
		
		SizeType OldLength = StringLength(OldValue);
		SizeType NewLength = StringLength(NewValue);
		
		if (OldLength == 0)
		{
			return *this;
		}
		
		TString Result;
		SizeType CurrentPos = 0;
		SizeType FoundPos;
		
		while ((FoundPos = Find(OldValue, CurrentPos)) != NoPosition)
		{
			// 添加匹配前的部分
			Result.Append(GetData() + CurrentPos, FoundPos - CurrentPos);
			// 添加新值
			Result.Append(NewValue, NewLength);
			// 移动位置
			CurrentPos = FoundPos + OldLength;
		}
		
		// 添加剩余部分
		Result.Append(GetData() + CurrentPos, Size() - CurrentPos);
		return Result;
	}

	/**
	 * @brief 替换字符串中的所有字符
	 * @param OldChar 要替换的字符
	 * @param NewChar 替换为的字符
	 * @return 替换后的字符串
	 */
	TString Replace(CharType OldChar, CharType NewChar) const
	{
		TString Result = *this;
		Result.EnsureUnique();
		CharType* Data = Result.GetMutableData();
		SizeType CurrentSize = Result.Size();
		
		for (SizeType i = 0; i < CurrentSize; ++i)
		{
			if (Data[i] == OldChar)
			{
				Data[i] = NewChar;
			}
		}
		
		return Result;
	}

	/**
	 * @brief 转换为std::string
	 * @return std::string
	 */
	StdString ToStdString() const
	{
		return StdString(GetData(), Size());
	}

	/**
	 * @brief 转换为string_view
	 * @return string_view
	 */
	StringView ToStringView() const
	{
		return StringView(GetData(), Size());
	}

public:
	// === 操作符重载 ===

	TString operator+(const TString& Other) const
	{
		TString Result;
		Result.Reserve(Size() + Other.Size());
		Result.Append(GetData(), Size());
		Result.Append(Other.GetData(), Other.Size());
		return Result;
	}

	TString operator+(const CharType* CStr) const
	{
		if (!CStr)
		{
			return *this;
		}

		SizeType CStrLength = StringLength(CStr);
		TString Result;
		Result.Reserve(Size() + CStrLength);
		Result.Append(GetData(), Size());
		Result.Append(CStr, CStrLength);
		return Result;
	}

	TString& operator+=(const TString& Other)
	{
		Append(Other.GetData(), Other.Size());
		return *this;
	}

	TString& operator+=(const CharType* CStr)
	{
		if (CStr)
		{
			Append(CStr, StringLength(CStr));
		}
		return *this;
	}

	TString& operator+=(CharType Ch)
	{
		PushBack(Ch);
		return *this;
	}

	bool operator==(const TString& Other) const
	{
		return Compare(Other) == 0;
	}

	bool operator!=(const TString& Other) const
	{
		return Compare(Other) != 0;
	}

	bool operator<(const TString& Other) const
	{
		return Compare(Other) < 0;
	}

	bool operator<=(const TString& Other) const
	{
		return Compare(Other) <= 0;
	}

	bool operator>(const TString& Other) const
	{
		return Compare(Other) > 0;
	}

	bool operator>=(const TString& Other) const
	{
		return Compare(Other) >= 0;
	}

	bool operator==(const CharType* CStr) const
	{
		return Compare(CStr) == 0;
	}

	bool operator!=(const CharType* CStr) const
	{
		return Compare(CStr) != 0;
	}

public:
	// === 迭代器支持 ===

	Iterator begin()
	{
		EnsureUnique();
		return GetMutableData();
	}
	ConstIterator begin() const
	{
		return GetData();
	}
	ConstIterator cbegin() const
	{
		return GetData();
	}

	Iterator end()
	{
		EnsureUnique();
		return GetMutableData() + Size();
	}
	ConstIterator end() const
	{
		return GetData() + Size();
	}
	ConstIterator cend() const
	{
		return GetData() + Size();
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
	// === 静态工厂方法 ===
	
	/**
	 * @brief 从整数创建字符串
	 */
	static TString FromInt(int32_t Value)
	{
		if (Value == 0)
			return TString("0");
		
		char Buffer[12]; // 足够存储 int32 最大值
		int Index = 0;
		bool IsNegative = Value < 0;
		
		// 处理 INT32_MIN 的特殊情况，避免溢出
		uint32_t UnsignedValue;
		if (IsNegative)
		{
			if (Value == INT32_MIN)
			{
				UnsignedValue = static_cast<uint32_t>(INT32_MAX) + 1;
			}
			else
			{
				UnsignedValue = static_cast<uint32_t>(-Value);
			}
		}
		else
		{
			UnsignedValue = static_cast<uint32_t>(Value);
		}
		
		// 逆序生成数字
		while (UnsignedValue > 0)
		{
			Buffer[Index++] = '0' + (UnsignedValue % 10);
			UnsignedValue /= 10;
		}
		
		if (IsNegative)
			Buffer[Index++] = '-';
		
		// 反转字符串
		TString Result;
		Result.Reserve(Index);
		for (int i = Index - 1; i >= 0; --i)
		{
			Result.PushBack(Buffer[i]);
		}
		
		return Result;
	}
	
	/**
	 * @brief 从64位整数创建字符串
	 */
	static TString FromInt64(int64_t Value)
	{
		if (Value == 0)
			return TString("0");
		
		char Buffer[21]; // 足够存储 int64 最大值
		int Index = 0;
		bool IsNegative = Value < 0;
		
		// 处理 INT64_MIN 的特殊情况，避免溢出
		uint64_t UnsignedValue;
		if (IsNegative)
		{
			if (Value == INT64_MIN)
			{
				UnsignedValue = static_cast<uint64_t>(INT64_MAX) + 1;
			}
			else
			{
				UnsignedValue = static_cast<uint64_t>(-Value);
			}
		}
		else
		{
			UnsignedValue = static_cast<uint64_t>(Value);
		}
		
		// 逆序生成数字
		while (UnsignedValue > 0)
		{
			Buffer[Index++] = '0' + (UnsignedValue % 10);
			UnsignedValue /= 10;
		}
		
		if (IsNegative)
			Buffer[Index++] = '-';
		
		// 反转字符串
		TString Result;
		Result.Reserve(Index);
		for (int i = Index - 1; i >= 0; --i)
		{
			Result.PushBack(Buffer[i]);
		}
		
		return Result;
	}
	
	/**
	 * @brief 从单精度浮点数创建字符串
	 */
	static TString FromFloat(float Value)
	{
		// 简化实现：转为double处理
		return FromDouble(static_cast<double>(Value));
	}
	
	/**
	 * @brief 从双精度浮点数创建字符串
	 */
	static TString FromDouble(double Value)
	{
		// 处理特殊值
		if (Value != Value) // NaN check
			return TString("nan");
		if (Value > 1e308)
			return TString("inf");
		if (Value < -1e308)
			return TString("-inf");
		
		if (Value == 0.0)
			return TString("0.0");
		
		bool IsNegative = Value < 0;
		if (IsNegative)
			Value = -Value;
		
		// 分离整数部分和小数部分
		int64_t IntegerPart = static_cast<int64_t>(Value);
		double FractionalPart = Value - IntegerPart;
		
		TString Result;
		if (IsNegative)
			Result += "-";
		
		// 添加整数部分
		Result += FromInt64(IntegerPart);
		Result += ".";
		
		// 添加小数部分（保留6位精度）
		for (int i = 0; i < 6; ++i)
		{
			FractionalPart *= 10;
			int Digit = static_cast<int>(FractionalPart);
			Result.PushBack('0' + Digit);
			FractionalPart -= Digit;
		}
		
		return Result;
	}

	/**
	 * @brief 格式化字符串（简单实现，支持{}占位符）
	 * @param FormatStr 格式字符串，使用{}作为占位符
	 * @param Args 参数包
	 * @return 格式化后的字符串
	 */
	template<typename... TArgs>
	static TString Format(const CharType* FormatStr, TArgs&&... Args)
	{
		if (!FormatStr)
			return TString();
		
		TString Result;
		const CharType* Current = FormatStr;
		size_t ArgIndex = 0;
		constexpr size_t ArgCount = sizeof...(Args);
		
		// 将参数转换为字符串数组
		TString ArgStrings[ArgCount > 0 ? ArgCount : 1];
		if constexpr (ArgCount > 0)
		{
			size_t Index = 0;
			((ArgStrings[Index++] = ToStringHelper(std::forward<TArgs>(Args))), ...);
		}
		
		while (*Current)
		{
			if (*Current == CharType('{') && *(Current + 1) == CharType('}'))
			{
				// 找到占位符 {}
				if (ArgIndex < ArgCount)
				{
					Result += ArgStrings[ArgIndex++];
				}
				Current += 2; // 跳过 {}
			}
			else
			{
				Result.PushBack(*Current);
				Current++;
			}
		}
		
		return Result;
	}
	
private:
	/**
	 * @brief 转换参数为字符串的辅助函数
	 */
	template<typename T>
	static TString ToStringHelper(T&& Value)
	{
		if constexpr (std::is_same_v<std::decay_t<T>, TString>)
		{
			return std::forward<T>(Value);
		}
		else if constexpr (std::is_same_v<std::decay_t<T>, const CharType*>)
		{
			return TString(Value);
		}
		else if constexpr (std::is_same_v<std::decay_t<T>, CharType*>)
		{
			return TString(Value);
		}
		else if constexpr (std::is_integral_v<std::decay_t<T>>)
		{
			if constexpr (std::is_same_v<std::decay_t<T>, int32_t>)
				return FromInt(static_cast<int32_t>(Value));
			else if constexpr (std::is_same_v<std::decay_t<T>, int64_t>)
				return FromInt64(static_cast<int64_t>(Value));
			else
				return FromInt(static_cast<int32_t>(Value));
		}
		else if constexpr (std::is_floating_point_v<std::decay_t<T>>)
		{
			if constexpr (std::is_same_v<std::decay_t<T>, float>)
				return FromFloat(static_cast<float>(Value));
			else
				return FromDouble(static_cast<double>(Value));
		}
		else
		{
			// 对于其他类型，尝试使用流操作符或返回类型名
			return TString("<unknown>");
		}
	}

public:
	
	/**
	 * @brief 计算C字符串长度
	 * @param CStr C字符串
	 * @return 长度
	 */
	static SizeType StringLength(const CharType* CStr)
	{
		if (!CStr)
		{
			return 0;
		}

		SizeType Length = 0;
		while (CStr[Length] != CharType(0))
		{
			++Length;
		}
		return Length;
	}

protected:
	// === BaseType虚函数实现 ===

	Reference DoEmplaceBack() override
	{
		PushBack(CharType{});
		return Back();
	}

	// 模板版本的EmplaceBack支持
	template <typename... TArgs>
	Reference DoEmplaceBackWithArgs(TArgs&&... Args)
	{
		static_assert(std::is_constructible_v<CharType, TArgs...>, "CharType must be constructible from Args");
		PushBack(CharType(std::forward<TArgs>(Args)...));
		return Back();
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
	 * @brief 检查是否使用SSO
	 * @return true if SSO, false otherwise
	 */
	bool IsSSO() const
	{
		return (Storage.SSO.Size & 0x80) != 0;
	}


	/**
	 * @brief 获取可变数据指针
	 * @return 可变数据指针
	 */
	CharType* GetMutableData()
	{
		return IsSSO() ? Storage.SSO.Buffer : Storage.Heap.Data->Data;
	}

	/**
	 * @brief 设置字符串大小
	 * @param NewSize 新大小
	 */
	void SetSize(SizeType NewSize)
	{
		if (IsSSO())
		{
			Storage.SSO.Size = static_cast<uint8_t>(NewSize) | 0x80;
		}
		else
		{
			Storage.Heap.Size = NewSize;
			Storage.Heap.Data->Size = NewSize;
		}
	}

	/**
	 * @brief 确保字符串是唯一的（COW语义）
	 */
	void EnsureUnique()
	{
		if (!IsSSO() && Storage.Heap.Data->RefCount.load() > 1)
		{
			// 需要拷贝数据
			SStringData* OldData = Storage.Heap.Data;
			SizeType CurrentSize = Storage.Heap.Size;
			SizeType CurrentCapacity = Storage.Heap.Capacity;

			AllocateHeap(CurrentCapacity);
			std::memcpy(Storage.Heap.Data->Data, OldData->Data, (CurrentSize + 1) * sizeof(CharType));
			Storage.Heap.Data->Size = CurrentSize;
			Storage.Heap.Size = CurrentSize;

			// 减少旧数据的引用计数
			if (OldData->RefCount.fetch_sub(1) == 1)
			{
				Allocator->DeallocateObject(OldData);
			}
		}
	}

	/**
	 * @brief 释放字符串数据
	 */
	void Release()
	{
		if (!IsSSO())
		{
			if (Storage.Heap.Data->RefCount.fetch_sub(1) == 1)
			{
				Allocator->DeallocateObject(Storage.Heap.Data);
			}
		}
	}

	/**
	 * @brief 分配堆内存
	 * @param Capacity 容量
	 */
	void AllocateHeap(SizeType Capacity)
	{
		SizeType AllocSize = sizeof(SStringData) + Capacity * sizeof(CharType);
		SStringData* Data = static_cast<SStringData*>(Allocator->AllocateObject(AllocSize));

		if (!Data)
		{
			NLOG(LogCore, Error, "TString failed to allocate {} bytes", AllocSize);
			throw std::bad_alloc();
		}

		new (Data) SStringData(0, Capacity);

		Storage.Heap.Data = Data;
		Storage.Heap.Size = 0;
		Storage.Heap.Capacity = Capacity;
	}

	/**
	 * @brief 重新分配堆内存
	 * @param NewCapacity 新容量
	 */
	void ReallocateHeap(SizeType NewCapacity)
	{
		SStringData* OldData = Storage.Heap.Data;
		SizeType OldSize = Storage.Heap.Size;

		AllocateHeap(NewCapacity);

		// 拷贝旧数据
		std::memcpy(Storage.Heap.Data->Data, OldData->Data, (OldSize + 1) * sizeof(CharType));
		Storage.Heap.Data->Size = OldSize;
		Storage.Heap.Size = OldSize;

		// 释放旧数据
		if (OldData->RefCount.fetch_sub(1) == 1)
		{
			Allocator->DeallocateObject(OldData);
		}
	}

	/**
	 * @brief 从SSO转换到堆
	 * @param NewCapacity 新容量
	 */
	void ConvertToHeap(SizeType NewCapacity)
	{
		SizeType SSOSize = Storage.SSO.Size & 0x7F;
		CharType SSOBuffer[SSOThreshold + 1];
		std::memcpy(SSOBuffer, Storage.SSO.Buffer, (SSOSize + 1) * sizeof(CharType));

		AllocateHeap(NewCapacity);
		std::memcpy(Storage.Heap.Data->Data, SSOBuffer, (SSOSize + 1) * sizeof(CharType));
		Storage.Heap.Data->Size = SSOSize;
		Storage.Heap.Size = SSOSize;
	}
};

// === 类型别名 ===
// 注意：CString 已在 Core/Object.h 中定义为 TString<char, CMemoryManager>

using WString = TString<wchar_t>;
using U8String = TString<char8_t>;
using U16String = TString<char16_t>;
using U32String = TString<char32_t>;

template <typename TCharType>
using TStringPtr = TSharedPtr<TString<TCharType>>;

using CStringPtr = TStringPtr<char>;
using WStringPtr = TStringPtr<wchar_t>;

// === 全局操作符 ===

template <typename TCharType>
TString<TCharType> operator+(const TCharType* CStr, const TString<TCharType>& Str)
{
	if (!CStr)
	{
		return Str;
	}

	typename TString<TCharType>::SizeType CStrLength = TString<TCharType>::StringLength(CStr);
	TString<TCharType> Result;
	Result.Reserve(CStrLength + Str.Size());
	Result.Append(CStr, CStrLength);
	Result.Append(Str.GetData(), Str.Size());
	return Result;
}

} // namespace NLib

// === 标准库哈希特化 ===

namespace std
{
template <typename TCharType, typename TAllocator>
struct hash<NLib::TString<TCharType, TAllocator>>
{
	size_t operator()(const NLib::TString<TCharType, TAllocator>& Str) const noexcept
	{
		return Str.GetHashCode();
	}
};
} // namespace std
