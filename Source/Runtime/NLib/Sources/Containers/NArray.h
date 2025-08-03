#pragma once

#include "Logging/CLogger.h"
#include "CContainer.h"
#include "NLibMacros.h"

#include <algorithm>
#include <initializer_list>

namespace NLib
{

template <typename TType>
NCLASS()
class LIBNLIB_API CArray : public CContainer
{
	GENERATED_BODY()

	friend class CIteratorBase<T, CArray>;

public:
	using ValueType = T;
	using Reference = T&;
	using ConstReference = const T&;
	using Pointer = T*;
	using ConstPointer = const T*;
	using Iterator = CIteratorBase<T, CArray<T>>;
	using ConstIterator = CIteratorBase<const T, CArray<T>>;

	// 构造函数
	CArray();
	explicit CArray(size_t InitialCapacity);
	CArray(size_t Count, const T& Value);
	CArray(std::initializer_list<T> InitList);
	template <typename TInputIterator>
	CArray(InputIterator First, InputIterator Last);
	CArray(const CArray& Other);
	CArray(CArray&& Other) noexcept;

	// 析构函数
	virtual ~CArray();

	// 赋值操作符
	CArray& operator=(const CArray& Other);
	CArray& operator=(CArray&& Other) noexcept;
	CArray& operator=(std::initializer_list<T> InitList);

	// 基础访问
	Reference operator[](size_t Index);
	ConstReference operator[](size_t Index) const;
	Reference At(size_t Index);
	ConstReference At(size_t Index) const;

	Reference Front()
	{
		return Data[0];
	}
	ConstReference Front() const
	{
		return Data[0];
	}
	Reference Back()
	{
		return Data[Size - 1];
	}
	ConstReference Back() const
	{
		return Data[Size - 1];
	}

	Pointer GetData()
	{
		return Data;
	}
	ConstPointer GetData() const
	{
		return Data;
	}

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
	void Resize(size_t NewSize);
	void Resize(size_t NewSize, const T& Value);
	void ShrinkToFit();

	// 元素操作
	void PushBack(const T& Value);
	void PushBack(T&& Value);
	template <typename... TArgs>
	Reference EmplaceBack(TArgs&&... Args);

	void PopBack();

	Iterator Insert(ConstIterator Pos, const T& Value);
	Iterator Insert(ConstIterator Pos, T&& Value);
	Iterator Insert(ConstIterator Pos, size_t Count, const T& Value);
	template <typename TInputIterator>
	Iterator Insert(ConstIterator Pos, InputIterator First, InputIterator Last);
	Iterator Insert(ConstIterator Pos, std::initializer_list<T> InitList);

	template <typename... TArgs>
	Iterator Emplace(ConstIterator Pos, TArgs&&... Args);

	Iterator Erase(ConstIterator Pos);
	Iterator Erase(ConstIterator First, ConstIterator Last);

	// 查找和算法
	Iterator Find(const T& Value);
	ConstIterator Find(const T& Value) const;

	template <typename TPredicate>
	Iterator FindIf(TPredicate Pred);
	template <typename TPredicate>
	ConstIterator FindIf(TPredicate Pred) const;

	bool Contains(const T& Value) const;
	size_t Count(const T& Value) const;

	template <typename TPredicate>
	size_t CountIf(TPredicate Pred) const;

	// 排序和操作
	void Sort();
	template <typename TCompare>
	void Sort(TCompare Comp);

	void Reverse();

	template <typename TPredicate>
	Iterator RemoveIf(TPredicate Pred);

	void RemoveDuplicates();

	// 批量操作
	void Assign(size_t Count, const T& Value);
	template <typename TInputIterator>
	void Assign(InputIterator First, InputIterator Last);
	void Assign(std::initializer_list<T> InitList);

	void Append(const CArray& Other);
	void Append(CArray&& Other);
	void Append(std::initializer_list<T> InitList);

	// 子数组操作
	CArray Slice(size_t Start, size_t Length = static_cast<size_t>(-1)) const;

	// 迭代器
	Iterator Begin()
	{
		return Iterator(Data);
	}
	Iterator End()
	{
		return Iterator(Data + Size);
	}
	ConstIterator Begin() const
	{
		return ConstIterator(Data);
	}
	ConstIterator End() const
	{
		return ConstIterator(Data + Size);
	}
	ConstIterator CBegin() const
	{
		return ConstIterator(Data);
	}
	ConstIterator CEnd() const
	{
		return ConstIterator(Data + Size);
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
	bool operator==(const CArray& Other) const;
	bool operator!=(const CArray& Other) const;
	bool operator<(const CArray& Other) const;
	bool operator<=(const CArray& Other) const;
	bool operator>(const CArray& Other) const;
	bool operator>=(const CArray& Other) const;

	// CObject 接口重写
	virtual bool Equals(const CObject* Other) const override;
	virtual size_t GetHashCode() const override;
	virtual CString ToString() const override;

private:
	T* Data;
	size_t Size;
	size_t Capacity;

	// 内部工具函数
	void EnsureCapacity(size_t RequiredCapacity);
	void ReallocateAndMove(size_t NewCapacity);
	void SetSize(size_t NewSize);
	void InsertElements(size_t Pos, const T* Src, size_t Count);
	void EraseElements(size_t Pos, size_t Count);
	void MoveElementsRight(size_t Pos, size_t Count);
	void MoveElementsLeft(size_t Pos, size_t Count);

	// 内存管理
	void AllocateMemory(size_t Capacity);
	void DeallocateMemory();
	void MoveFrom(CArray&& Other) noexcept;
	void CopyFrom(const CArray& Other);
};

// 模板实现
template <typename TType>
CArray<T>::CArray()
    : Data(nullptr),
      Size(0),
      Capacity(0)
{}

template <typename TType>
CArray<T>::CArray(size_t InitialCapacity)
    : Data(nullptr),
      Size(0),
      Capacity(0)
{
	if (InitialCapacity > 0)
	{
		AllocateMemory(InitialCapacity);
	}
}

template <typename TType>
CArray<T>::CArray(size_t Count, const T& Value)
    : Data(nullptr),
      Size(0),
      Capacity(0)
{
	if (Count > 0)
	{
		AllocateMemory(Count);
		ConstructElements(Data, Count, Value);
		RegisterWithGC(Data, Count);
		Size = Count;
	}
}

template <typename TType>
CArray<T>::CArray(std::initializer_list<T> InitList)
    : Data(nullptr),
      Size(0),
      Capacity(0)
{
	if (InitList.size() > 0)
	{
		AllocateMemory(InitList.size());

		size_t i = 0;
		for (const auto& Item : InitList)
		{
			new (Data + i) T(Item);
			++i;
		}

		RegisterWithGC(Data, InitList.size());
		Size = InitList.size();
	}
}

template <typename TType>
template <typename TInputIterator>
CArray<T>::CArray(InputIterator First, InputIterator Last)
    : Data(nullptr),
      Size(0),
      Capacity(0)
{
	size_t Count = std::distance(First, Last);
	if (Count > 0)
	{
		AllocateMemory(Count);

		size_t i = 0;
		for (auto It = First; It != Last; ++It, ++i)
		{
			new (Data + i) T(*It);
		}

		RegisterWithGC(Data, Count);
		Size = Count;
	}
}

template <typename TType>
CArray<T>::CArray(const CArray& Other)
    : Data(nullptr),
      Size(0),
      Capacity(0)
{
	CopyFrom(Other);
}

template <typename TType>
CArray<T>::CArray(CArray&& Other) noexcept
    : Data(nullptr),
      Size(0),
      Capacity(0)
{
	MoveFrom(std::move(Other));
}

template <typename TType>
CArray<T>::~CArray()
{
	DeallocateMemory();
}

template <typename TType>
CArray<T>& CArray<T>::operator=(const CArray& Other)
{
	if (this != &Other)
	{
		CopyFrom(Other);
	}
	return *this;
}

template <typename TType>
CArray<T>& CArray<T>::operator=(CArray&& Other) noexcept
{
	if (this != &Other)
	{
		DeallocateMemory();
		MoveFrom(std::move(Other));
	}
	return *this;
}

template <typename TType>
CArray<T>& CArray<T>::operator=(std::initializer_list<T> InitList)
{
	Clear();
	if (InitList.size() > 0)
	{
		EnsureCapacity(InitList.size());

		size_t i = 0;
		for (const auto& Item : InitList)
		{
			new (Data + i) T(Item);
			++i;
		}

		RegisterWithGC(Data, InitList.size());
		Size = InitList.size();
	}
	return *this;
}

template <typename TType>
typename CArray<T>::Reference CArray<T>::operator[](size_t Index)
{
	return Data[Index];
}

template <typename TType>
typename CArray<T>::ConstReference CArray<T>::operator[](size_t Index) const
{
	return Data[Index];
}

template <typename TType>
typename CArray<T>::Reference CArray<T>::At(size_t Index)
{
	if (Index >= Size)
	{
		CLogger::Error("CArray::At: Index {} out of bounds (size: {})", Index, Size);
		static T dummy{};
		return dummy;
	}
	return Data[Index];
}

template <typename TType>
typename CArray<T>::ConstReference CArray<T>::At(size_t Index) const
{
	if (Index >= Size)
	{
		CLogger::Error("CArray::At: Index {} out of bounds (size: {})", Index, Size);
		static const T dummy{};
		return dummy;
	}
	return Data[Index];
}

template <typename TType>
void CArray<T>::Clear()
{
	if (Data)
	{
		UnregisterFromGC(Data, Size);
		DestructElements(Data, Size);
		Size = 0;
	}
}

template <typename TType>
void CArray<T>::Reserve(size_t NewCapacity)
{
	if (NewCapacity > Capacity)
	{
		ReallocateAndMove(NewCapacity);
	}
}

template <typename TType>
void CArray<T>::Resize(size_t NewSize)
{
	if (NewSize > Size)
	{
		EnsureCapacity(NewSize);
		ConstructElements(Data + Size, NewSize - Size);
		RegisterWithGC(Data + Size, NewSize - Size);
	}
	else if (NewSize < Size)
	{
		UnregisterFromGC(Data + NewSize, Size - NewSize);
		DestructElements(Data + NewSize, Size - NewSize);
	}

	Size = NewSize;
}

template <typename TType>
void CArray<T>::Resize(size_t NewSize, const T& Value)
{
	if (NewSize > Size)
	{
		EnsureCapacity(NewSize);
		ConstructElements(Data + Size, NewSize - Size, Value);
		RegisterWithGC(Data + Size, NewSize - Size);
	}
	else if (NewSize < Size)
	{
		UnregisterFromGC(Data + NewSize, Size - NewSize);
		DestructElements(Data + NewSize, Size - NewSize);
	}

	Size = NewSize;
}

template <typename TType>
void CArray<T>::PushBack(const T& Value)
{
	EnsureCapacity(Size + 1);
	new (Data + Size) T(Value);
	RegisterWithGC(Data + Size, 1);
	++Size;
}

template <typename TType>
void CArray<T>::PushBack(T&& Value)
{
	EnsureCapacity(Size + 1);
	new (Data + Size) T(std::move(Value));
	RegisterWithGC(Data + Size, 1);
	++Size;
}

template <typename TType>
template <typename... Args>
typename CArray<T>::Reference CArray<T>::EmplaceBack(Args&&... args)
{
	EnsureCapacity(Size + 1);
	new (Data + Size) T(std::forward<Args>(args)...);
	RegisterWithGC(Data + Size, 1);
	++Size;
	return Data[Size - 1];
}

template <typename TType>
void CArray<T>::PopBack()
{
	if (Size > 0)
	{
		--Size;
		UnregisterFromGC(Data + Size, 1);
		DestructElements(Data + Size, 1);
	}
}

template <typename TType>
typename CArray<T>::Iterator CArray<T>::Find(const T& Value)
{
	for (size_t i = 0; i < Size; ++i)
	{
		if (NLib::NEqual<T>{}(Data[i], Value))
		{
			return Iterator(Data + i);
		}
	}
	return End();
}

template <typename TType>
typename CArray<T>::ConstIterator CArray<T>::Find(const T& Value) const
{
	for (size_t i = 0; i < Size; ++i)
	{
		if (NLib::NEqual<T>{}(Data[i], Value))
		{
			return ConstIterator(Data + i);
		}
	}
	return End();
}

template <typename TType>
bool CArray<T>::Contains(const T& Value) const
{
	return Find(Value) != End();
}

template <typename TType>
void CArray<T>::Sort()
{
	if (Size > 1)
	{
		std::sort(Data, Data + Size);
	}
}

template <typename TType>
template <typename TCompare>
void CArray<T>::Sort(Compare comp)
{
	if (Size > 1)
	{
		std::sort(Data, Data + Size, comp);
	}
}

template <typename TType>
void CArray<T>::Reverse()
{
	if (Size > 1)
	{
		std::reverse(Data, Data + Size);
	}
}

template <typename TType>
bool CArray<T>::operator==(const CArray& Other) const
{
	if (Size != Other.Size)
		return false;

	for (size_t i = 0; i < Size; ++i)
	{
		if (!NLib::NEqual<T>{}(Data[i], Other.Data[i]))
			return false;
	}

	return true;
}

template <typename TType>
bool CArray<T>::operator!=(const CArray& Other) const
{
	return !(*this == Other);
}

template <typename TType>
bool CArray<T>::Equals(const CObject* Other) const
{
	const CArray<T>* OtherArray = dynamic_cast<const CArray<T>*>(Other);
	return OtherArray && (*this == *OtherArray);
}

template <typename TType>
size_t CArray<T>::GetHashCode() const
{
	size_t Hash = 0;
	for (size_t i = 0; i < Size; ++i)
	{
		Hash ^= NLib::NHash<T>{}(Data[i]) + 0x9e3779b9 + (Hash << 6) + (Hash >> 2);
	}
	return Hash;
}

// ToString方法声明移至实现文件以避免循环依赖
// 这里只保留声明，实现将在NArray.cpp中通过显式模板实例化提供

// 内部实现
template <typename TType>
void CArray<T>::EnsureCapacity(size_t RequiredCapacity)
{
	if (RequiredCapacity <= Capacity)
		return;

	size_t NewCapacity = CalculateGrowth(Capacity, RequiredCapacity);
	ReallocateAndMove(NewCapacity);
}

template <typename TType>
void CArray<T>::ReallocateAndMove(size_t NewCapacity)
{
	T* NewData = AllocateElements<T>(NewCapacity);

	if (Data)
	{
		MoveElements(NewData, Data, Size);
		DeallocateElements(Data, Capacity);
	}

	Data = NewData;
	Capacity = NewCapacity;
}

template <typename TType>
void CArray<T>::AllocateMemory(size_t Capacity)
{
	if (Capacity > 0)
	{
		Data = AllocateElements<T>(Capacity);
		this->Capacity = Capacity;
	}
}

template <typename TType>
void CArray<T>::DeallocateMemory()
{
	if (Data)
	{
		UnregisterFromGC(Data, Size);
		DestructElements(Data, Size);
		DeallocateElements(Data, Capacity);

		Data = nullptr;
		Size = 0;
		Capacity = 0;
	}
}

template <typename TType>
void CArray<T>::MoveFrom(CArray&& Other) noexcept
{
	Data = Other.Data;
	Size = Other.Size;
	Capacity = Other.Capacity;

	Other.Data = nullptr;
	Other.Size = 0;
	Other.Capacity = 0;
}

template <typename TType>
void CArray<T>::CopyFrom(const CArray& Other)
{
	if (Other.Size == 0)
	{
		Clear();
		return;
	}

	EnsureCapacity(Other.Size);

	Clear();
	CopyElements(Data, Other.Data, Other.Size);
	RegisterWithGC(Data, Other.Size);
	Size = Other.Size;
}

// 缺失的方法实现
template <typename TType>
void CArray<T>::ShrinkToFit()
{
	if (Capacity > Size)
	{
		ReallocateAndMove(Size);
	}
}

template <typename TType>
typename CArray<T>::Iterator CArray<T>::Insert(ConstIterator Pos, const T& Value)
{
	size_t index = Pos.GetPtr() - Data;
	EnsureCapacity(Size + 1);

	MoveElementsRight(index, 1);
	new (Data + index) T(Value);
	RegisterWithGC(Data + index, 1);
	++Size;

	return Iterator(Data + index);
}

template <typename TType>
typename CArray<T>::Iterator CArray<T>::Insert(ConstIterator Pos, T&& Value)
{
	size_t index = Pos.GetPtr() - Data;
	EnsureCapacity(Size + 1);

	MoveElementsRight(index, 1);
	new (Data + index) T(std::move(Value));
	RegisterWithGC(Data + index, 1);
	++Size;

	return Iterator(Data + index);
}

template <typename TType>
typename CArray<T>::Iterator CArray<T>::Erase(ConstIterator Pos)
{
	size_t index = Pos.GetPtr() - Data;
	if (index >= Size)
	{
		return End();
	}

	UnregisterFromGC(Data + index, 1);
	DestructElements(Data + index, 1);

	MoveElementsLeft(index, 1);
	--Size;

	return Iterator(Data + index);
}

template <typename TType>
typename CArray<T>::Iterator CArray<T>::Erase(ConstIterator First, ConstIterator Last)
{
	size_t Start = First.GetPtr() - Data;
	size_t EndPos = Last.GetPtr() - Data;
	if (Start >= Size || EndPos > Size || Start >= EndPos)
	{
		return End();
	}
	size_t Count = EndPos - Start;

	UnregisterFromGC(Data + Start, Count);
	DestructElements(Data + Start, Count);

	if (EndPos < Size)
	{
		MoveElements(Data + Start, Data + EndPos, Size - EndPos);
	}

	Size -= Count;
	return Iterator(Data + Start);
}

template <typename TType>
template <typename TPredicate>
typename CArray<T>::Iterator CArray<T>::FindIf(Predicate pred)
{
	for (size_t i = 0; i < Size; ++i)
	{
		if (pred(Data[i]))
		{
			return Iterator(Data + i);
		}
	}
	return End();
}

template <typename TType>
template <typename TPredicate>
typename CArray<T>::ConstIterator CArray<T>::FindIf(Predicate pred) const
{
	for (size_t i = 0; i < Size; ++i)
	{
		if (pred(Data[i]))
		{
			return ConstIterator(Data + i);
		}
	}
	return End();
}

template <typename TType>
size_t CArray<T>::Count(const T& Value) const
{
	size_t Count = 0;
	for (size_t i = 0; i < Size; ++i)
	{
		if (NEqual<T>{}(Data[i], Value))
		{
			++Count;
		}
	}
	return Count;
}

template <typename TType>
template <typename TPredicate>
size_t CArray<T>::CountIf(Predicate Pred) const
{
	size_t Count = 0;
	for (size_t i = 0; i < Size; ++i)
	{
		if (Pred(Data[i]))
		{
			++Count;
		}
	}
	return Count;
}

template <typename TType>
CArray<T> CArray<T>::Slice(size_t Start, size_t Length) const
{
	if (Start >= Size)
	{
		return CArray<T>();
	}

	if (Length == static_cast<size_t>(-1) || Start + Length > Size)
	{
		Length = Size - Start;
	}

	CArray<T> Result;
	if (Length > 0)
	{
		Result.AllocateMemory(Length);
		CopyElements(Result.Data, Data + Start, Length);
		RegisterWithGC(Result.Data, Length);
		Result.Size = Length;
	}

	return Result;
}

template <typename TType>
void CArray<T>::Assign(size_t Count, const T& Value)
{
	Clear();
	if (Count > 0)
	{
		EnsureCapacity(Count);
		ConstructElements(Data, Count, Value);
		RegisterWithGC(Data, Count);
		Size = Count;
	}
}

template <typename TType>
template <typename TInputIterator>
void CArray<T>::Assign(InputIterator First, InputIterator Last)
{
	Clear();
	size_t Count = std::distance(First, Last);
	if (Count > 0)
	{
		EnsureCapacity(Count);

		size_t i = 0;
		for (auto It = First; It != Last; ++It, ++i)
		{
			new (Data + i) T(*It);
		}

		RegisterWithGC(Data, Count);
		Size = Count;
	}
}

template <typename TType>
void CArray<T>::Assign(std::initializer_list<T> InitList)
{
	Clear();
	if (InitList.size() > 0)
	{
		EnsureCapacity(InitList.size());

		size_t i = 0;
		for (const auto& Item : InitList)
		{
			new (Data + i) T(Item);
			++i;
		}

		RegisterWithGC(Data, InitList.size());
		Size = InitList.size();
	}
}

template <typename TType>
void CArray<T>::Append(const CArray& Other)
{
	if (Other.Size > 0)
	{
		EnsureCapacity(Size + Other.Size);
		CopyElements(Data + Size, Other.Data, Other.Size);
		RegisterWithGC(Data + Size, Other.Size);
		Size += Other.Size;
	}
}

template <typename TType>
void CArray<T>::MoveElementsRight(size_t Pos, size_t Count)
{
	if (Pos < Size && Count > 0)
	{
		MoveElements(Data + Pos + Count, Data + Pos, Size - Pos);
	}
}

template <typename TType>
void CArray<T>::MoveElementsLeft(size_t Pos, size_t Count)
{
	if (Pos + Count < Size)
	{
		MoveElements(Data + Pos, Data + Pos + Count, Size - Pos - Count);
	}
}

template <typename TType>
void CArray<T>::RemoveDuplicates()
{
	if (Size <= 1)
	{
		return;
	}

	size_t WriteIndex = 0;
	for (size_t ReadIndex = 0; ReadIndex < Size; ++ReadIndex)
	{
		bool duplicate = false;
		for (size_t CheckIndex = 0; CheckIndex < WriteIndex; ++CheckIndex)
		{
			if (NEqual<T>{}(Data[ReadIndex], Data[CheckIndex]))
			{
				duplicate = true;
				break;
			}
		}

		if (!duplicate)
		{
			if (WriteIndex != ReadIndex)
			{
				Data[WriteIndex] = std::move(Data[ReadIndex]);
			}
			++WriteIndex;
		}
	}

	// 销毁多余的元素
	if (WriteIndex < Size)
	{
		UnregisterFromGC(Data + WriteIndex, Size - WriteIndex);
		DestructElements(Data + WriteIndex, Size - WriteIndex);
		Size = WriteIndex;
	}
}

} // namespace NLib
