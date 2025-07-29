#pragma once

#include "NContainer.h"
#include <initializer_list>
#include <algorithm>

template<typename T>
NCLASS()
class LIBNLIB_API NArray : public NContainer
{
    GENERATED_BODY()

public:
    using ValueType = T;
    using Reference = T&;
    using ConstReference = const T&;
    using Pointer = T*;
    using ConstPointer = const T*;
    using Iterator = NIteratorBase<T, NArray<T>>;
    using ConstIterator = NIteratorBase<const T, NArray<T>>;

    // 构造函数
    NArray();
    explicit NArray(size_t initial_capacity);
    NArray(size_t count, const T& value);
    NArray(std::initializer_list<T> init_list);
    template<typename InputIterator>
    NArray(InputIterator first, InputIterator last);
    NArray(const NArray& other);
    NArray(NArray&& other) noexcept;
    
    // 析构函数
    virtual ~NArray();
    
    // 赋值操作符
    NArray& operator=(const NArray& other);
    NArray& operator=(NArray&& other) noexcept;
    NArray& operator=(std::initializer_list<T> init_list);
    
    // 基础访问
    Reference operator[](size_t index);
    ConstReference operator[](size_t index) const;
    Reference At(size_t index);
    ConstReference At(size_t index) const;
    
    Reference Front() { return data_[0]; }
    ConstReference Front() const { return data_[0]; }
    Reference Back() { return data_[size_ - 1]; }
    ConstReference Back() const { return data_[size_ - 1]; }
    
    Pointer Data() { return data_; }
    ConstPointer Data() const { return data_; }
    
    // 容器接口实现
    size_t GetSize() const override { return size_; }
    size_t GetCapacity() const override { return capacity_; }
    bool IsEmpty() const override { return size_ == 0; }
    void Clear() override;
    
    // 容量管理
    void Reserve(size_t new_capacity);
    void Resize(size_t new_size);
    void Resize(size_t new_size, const T& value);
    void ShrinkToFit();
    
    // 元素操作
    void PushBack(const T& value);
    void PushBack(T&& value);
    template<typename... Args>
    Reference EmplaceBack(Args&&... args);
    
    void PopBack();
    
    Iterator Insert(ConstIterator pos, const T& value);
    Iterator Insert(ConstIterator pos, T&& value);
    Iterator Insert(ConstIterator pos, size_t count, const T& value);
    template<typename InputIterator>
    Iterator Insert(ConstIterator pos, InputIterator first, InputIterator last);
    Iterator Insert(ConstIterator pos, std::initializer_list<T> init_list);
    
    template<typename... Args>
    Iterator Emplace(ConstIterator pos, Args&&... args);
    
    Iterator Erase(ConstIterator pos);
    Iterator Erase(ConstIterator first, ConstIterator last);
    
    // 查找和算法
    Iterator Find(const T& value);
    ConstIterator Find(const T& value) const;
    
    template<typename Predicate>
    Iterator FindIf(Predicate pred);
    template<typename Predicate>
    ConstIterator FindIf(Predicate pred) const;
    
    bool Contains(const T& value) const;
    size_t Count(const T& value) const;
    
    template<typename Predicate>
    size_t CountIf(Predicate pred) const;
    
    // 排序和操作
    void Sort();
    template<typename Compare>
    void Sort(Compare comp);
    
    void Reverse();
    
    template<typename Predicate>
    Iterator RemoveIf(Predicate pred);
    
    void RemoveDuplicates();
    
    // 批量操作
    void Assign(size_t count, const T& value);
    template<typename InputIterator>
    void Assign(InputIterator first, InputIterator last);
    void Assign(std::initializer_list<T> init_list);
    
    void Append(const NArray& other);
    void Append(NArray&& other);
    void Append(std::initializer_list<T> init_list);
    
    // 子数组操作
    NArray Slice(size_t start, size_t length = static_cast<size_t>(-1)) const;
    
    // 迭代器
    Iterator Begin() { return Iterator(data_); }
    Iterator End() { return Iterator(data_ + size_); }
    ConstIterator Begin() const { return ConstIterator(data_); }
    ConstIterator End() const { return ConstIterator(data_ + size_); }
    ConstIterator CBegin() const { return ConstIterator(data_); }
    ConstIterator CEnd() const { return ConstIterator(data_ + size_); }
    
    // 比较操作符
    bool operator==(const NArray& other) const;
    bool operator!=(const NArray& other) const;
    bool operator<(const NArray& other) const;
    bool operator<=(const NArray& other) const;
    bool operator>(const NArray& other) const;
    bool operator>=(const NArray& other) const;
    
    // NObject 接口重写
    virtual bool Equals(const NObject* other) const override;
    virtual size_t GetHashCode() const override;
    virtual NString ToString() const override;

private:
    T* data_;
    size_t size_;
    size_t capacity_;
    
    // 内部工具函数
    void EnsureCapacity(size_t required_capacity);
    void ReallocateAndMove(size_t new_capacity);
    void SetSize(size_t new_size);
    void InsertElements(size_t pos, const T* src, size_t count);
    void EraseElements(size_t pos, size_t count);
    void MoveElementsRight(size_t pos, size_t count);
    void MoveElementsLeft(size_t pos, size_t count);
    
    // 内存管理
    void AllocateMemory(size_t capacity);
    void DeallocateMemory();
    void MoveFrom(NArray&& other) noexcept;
    void CopyFrom(const NArray& other);
};

// 模板实现
template<typename T>
NArray<T>::NArray()
    : data_(nullptr), size_(0), capacity_(0)
{
}

template<typename T>
NArray<T>::NArray(size_t initial_capacity)
    : data_(nullptr), size_(0), capacity_(0)
{
    if (initial_capacity > 0)
    {
        AllocateMemory(initial_capacity);
    }
}

template<typename T>
NArray<T>::NArray(size_t count, const T& value)
    : data_(nullptr), size_(0), capacity_(0)
{
    if (count > 0)
    {
        AllocateMemory(count);
        ConstructElements(data_, count, value);
        RegisterWithGC(data_, count);
        size_ = count;
    }
}

template<typename T>
NArray<T>::NArray(std::initializer_list<T> init_list)
    : data_(nullptr), size_(0), capacity_(0)
{
    if (init_list.size() > 0)
    {
        AllocateMemory(init_list.size());
        
        size_t i = 0;
        for (const auto& item : init_list)
        {
            new (data_ + i) T(item);
            ++i;
        }
        
        RegisterWithGC(data_, init_list.size());
        size_ = init_list.size();
    }
}

template<typename T>
template<typename InputIterator>
NArray<T>::NArray(InputIterator first, InputIterator last)
    : data_(nullptr), size_(0), capacity_(0)
{
    size_t count = std::distance(first, last);
    if (count > 0)
    {
        AllocateMemory(count);
        
        size_t i = 0;
        for (auto it = first; it != last; ++it, ++i)
        {
            new (data_ + i) T(*it);
        }
        
        RegisterWithGC(data_, count);
        size_ = count;
    }
}

template<typename T>
NArray<T>::NArray(const NArray& other)
    : data_(nullptr), size_(0), capacity_(0)
{
    CopyFrom(other);
}

template<typename T>
NArray<T>::NArray(NArray&& other) noexcept
    : data_(nullptr), size_(0), capacity_(0)
{
    MoveFrom(std::move(other));
}

template<typename T>
NArray<T>::~NArray()
{
    DeallocateMemory();
}

template<typename T>
NArray<T>& NArray<T>::operator=(const NArray& other)
{
    if (this != &other)
    {
        CopyFrom(other);
    }
    return *this;
}

template<typename T>
NArray<T>& NArray<T>::operator=(NArray&& other) noexcept
{
    if (this != &other)
    {
        DeallocateMemory();
        MoveFrom(std::move(other));
    }
    return *this;
}

template<typename T>
NArray<T>& NArray<T>::operator=(std::initializer_list<T> init_list)
{
    Clear();
    if (init_list.size() > 0)
    {
        EnsureCapacity(init_list.size());
        
        size_t i = 0;
        for (const auto& item : init_list)
        {
            new (data_ + i) T(item);
            ++i;
        }
        
        RegisterWithGC(data_, init_list.size());
        size_ = init_list.size();
    }
    return *this;
}

template<typename T>
typename NArray<T>::Reference NArray<T>::operator[](size_t index)
{
    return data_[index];
}

template<typename T>
typename NArray<T>::ConstReference NArray<T>::operator[](size_t index) const
{
    return data_[index];
}

template<typename T>
typename NArray<T>::Reference NArray<T>::At(size_t index)
{
    if (index >= size_)
    {
        NLogger::GetLogger().Error("NArray::At: Index {} out of bounds (size: {})", index, size_);
        static T dummy{};
        return dummy;
    }
    return data_[index];
}

template<typename T>
typename NArray<T>::ConstReference NArray<T>::At(size_t index) const
{
    if (index >= size_)
    {
        NLogger::GetLogger().Error("NArray::At: Index {} out of bounds (size: {})", index, size_);
        static const T dummy{};
        return dummy;
    }
    return data_[index];
}

template<typename T>
void NArray<T>::Clear()
{
    if (data_)
    {
        UnregisterFromGC(data_, size_);
        DestructElements(data_, size_);
        size_ = 0;
    }
}

template<typename T>
void NArray<T>::Reserve(size_t new_capacity)
{
    if (new_capacity > capacity_)
    {
        ReallocateAndMove(new_capacity);
    }
}

template<typename T>
void NArray<T>::Resize(size_t new_size)
{
    if (new_size > size_)
    {
        EnsureCapacity(new_size);
        ConstructElements(data_ + size_, new_size - size_);
        RegisterWithGC(data_ + size_, new_size - size_);
    }
    else if (new_size < size_)
    {
        UnregisterFromGC(data_ + new_size, size_ - new_size);
        DestructElements(data_ + new_size, size_ - new_size);
    }
    
    size_ = new_size;
}

template<typename T>
void NArray<T>::Resize(size_t new_size, const T& value)
{
    if (new_size > size_)
    {
        EnsureCapacity(new_size);
        ConstructElements(data_ + size_, new_size - size_, value);
        RegisterWithGC(data_ + size_, new_size - size_);
    }
    else if (new_size < size_)
    {
        UnregisterFromGC(data_ + new_size, size_ - new_size);
        DestructElements(data_ + new_size, size_ - new_size);
    }
    
    size_ = new_size;
}

template<typename T>
void NArray<T>::PushBack(const T& value)
{
    EnsureCapacity(size_ + 1);
    new (data_ + size_) T(value);
    RegisterWithGC(data_ + size_, 1);
    ++size_;
}

template<typename T>
void NArray<T>::PushBack(T&& value)
{
    EnsureCapacity(size_ + 1);
    new (data_ + size_) T(std::move(value));
    RegisterWithGC(data_ + size_, 1);
    ++size_;
}

template<typename T>
template<typename... Args>
typename NArray<T>::Reference NArray<T>::EmplaceBack(Args&&... args)
{
    EnsureCapacity(size_ + 1);
    new (data_ + size_) T(std::forward<Args>(args)...);
    RegisterWithGC(data_ + size_, 1);
    ++size_;
    return data_[size_ - 1];
}

template<typename T>
void NArray<T>::PopBack()
{
    if (size_ > 0)
    {
        --size_;
        UnregisterFromGC(data_ + size_, 1);
        DestructElements(data_ + size_, 1);
    }
}

template<typename T>
typename NArray<T>::Iterator NArray<T>::Find(const T& value)
{
    for (size_t i = 0; i < size_; ++i)
    {
        if (NEqual<T>{}(data_[i], value))
        {
            return Iterator(data_ + i);
        }
    }
    return End();
}

template<typename T>
typename NArray<T>::ConstIterator NArray<T>::Find(const T& value) const
{
    for (size_t i = 0; i < size_; ++i)
    {
        if (NEqual<T>{}(data_[i], value))
        {
            return ConstIterator(data_ + i);
        }
    }
    return End();
}

template<typename T>
bool NArray<T>::Contains(const T& value) const
{
    return Find(value) != End();
}

template<typename T>
void NArray<T>::Sort()
{
    if (size_ > 1)
    {
        std::sort(data_, data_ + size_);
    }
}

template<typename T>
template<typename Compare>
void NArray<T>::Sort(Compare comp)
{
    if (size_ > 1)
    {
        std::sort(data_, data_ + size_, comp);
    }
}

template<typename T>
void NArray<T>::Reverse()
{
    if (size_ > 1)
    {
        std::reverse(data_, data_ + size_);
    }
}

template<typename T>
bool NArray<T>::operator==(const NArray& other) const
{
    if (size_ != other.size_) return false;
    
    for (size_t i = 0; i < size_; ++i)
    {
        if (!NEqual<T>{}(data_[i], other.data_[i]))
            return false;
    }
    
    return true;
}

template<typename T>
bool NArray<T>::operator!=(const NArray& other) const
{
    return !(*this == other);
}

template<typename T>
bool NArray<T>::Equals(const NObject* other) const
{
    const NArray<T>* other_array = dynamic_cast<const NArray<T>*>(other);
    return other_array && (*this == *other_array);
}

template<typename T>
size_t NArray<T>::GetHashCode() const
{
    size_t hash = 0;
    for (size_t i = 0; i < size_; ++i)
    {
        hash ^= NHash<T>{}(data_[i]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash;
}

template<typename T>
NString NArray<T>::ToString() const
{
    NString result = "[";
    for (size_t i = 0; i < size_; ++i)
    {
        if (i > 0) result += ", ";
        if constexpr (std::is_base_of_v<NObject, T>)
        {
            result += data_[i].ToString();
        }
        else
        {
            result += NString::Format("{}", data_[i]);
        }
    }
    result += "]";
    return result;
}

// 内部实现
template<typename T>
void NArray<T>::EnsureCapacity(size_t required_capacity)
{
    if (required_capacity <= capacity_)
        return;
    
    size_t new_capacity = CalculateGrowth(capacity_, required_capacity);
    ReallocateAndMove(new_capacity);
}

template<typename T>
void NArray<T>::ReallocateAndMove(size_t new_capacity)
{
    T* new_data = AllocateElements<T>(new_capacity);
    
    if (data_)
    {
        MoveElements(new_data, data_, size_);
        DeallocateElements(data_, capacity_);
    }
    
    data_ = new_data;
    capacity_ = new_capacity;
}

template<typename T>
void NArray<T>::AllocateMemory(size_t capacity)
{
    if (capacity > 0)
    {
        data_ = AllocateElements<T>(capacity);
        capacity_ = capacity;
    }
}

template<typename T>
void NArray<T>::DeallocateMemory()
{
    if (data_)
    {
        UnregisterFromGC(data_, size_);
        DestructElements(data_, size_);
        DeallocateElements(data_, capacity_);
        
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
    }
}

template<typename T>
void NArray<T>::MoveFrom(NArray&& other) noexcept
{
    data_ = other.data_;
    size_ = other.size_;
    capacity_ = other.capacity_;
    
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
}

template<typename T>
void NArray<T>::CopyFrom(const NArray& other)
{
    if (other.size_ == 0)
    {
        Clear();
        return;
    }
    
    EnsureCapacity(other.size_);
    
    Clear();
    CopyElements(data_, other.data_, other.size_);
    RegisterWithGC(data_, other.size_);
    size_ = other.size_;
}