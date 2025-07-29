#pragma once

#include "Core/NObject.h"
#include "Memory/NMemoryManager.h"
#include "Memory/NGarbageCollector.h"
#include <cstdint>
#include <cstring>
#include <type_traits>

NCLASS()
class LIBNLIB_API NContainer : public NObject
{
    GENERATED_BODY()

public:
    virtual ~NContainer() = default;

    virtual size_t GetSize() const = 0;
    virtual size_t GetCapacity() const = 0;
    virtual bool IsEmpty() const = 0;
    virtual void Clear() = 0;

protected:
    static constexpr size_t CONTAINER_ALIGNMENT = 8;
    static constexpr size_t MIN_CAPACITY = 4;
    
    // 容器专用内存分配器
    template<typename T>
    static T* AllocateElements(size_t count)
    {
        return static_cast<T*>(NMemoryManager::GetInstance().AllocateAligned(
            sizeof(T) * count, alignof(T)));
    }
    
    template<typename T>
    static void DeallocateElements(T* ptr, size_t count)
    {
        NMemoryManager::GetInstance().Deallocate(ptr, sizeof(T) * count);
    }
    
    // 对象类型的GC注册
    template<typename T>
    static void RegisterWithGC(T* ptr, size_t count)
    {
        if constexpr (std::is_base_of_v<NObject, T>)
        {
            for (size_t i = 0; i < count; ++i)
            {
                NGarbageCollector::GetInstance().RegisterObject(&ptr[i]);
            }
        }
    }
    
    template<typename T>
    static void UnregisterFromGC(T* ptr, size_t count)
    {
        if constexpr (std::is_base_of_v<NObject, T>)
        {
            for (size_t i = 0; i < count; ++i)
            {
                NGarbageCollector::GetInstance().UnregisterObject(&ptr[i]);
            }
        }
    }
    
    // 智能构造和析构
    template<typename T>
    static void ConstructElements(T* ptr, size_t count)
    {
        if constexpr (std::is_trivially_constructible_v<T>)
        {
            memset(ptr, 0, sizeof(T) * count);
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
            {
                new (ptr + i) T();
            }
        }
    }
    
    template<typename T>
    static void ConstructElements(T* ptr, size_t count, const T& value)
    {
        for (size_t i = 0; i < count; ++i)
        {
            new (ptr + i) T(value);
        }
    }
    
    template<typename T>
    static void DestructElements(T* ptr, size_t count)
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
        {
            for (size_t i = 0; i < count; ++i)
            {
                ptr[i].~T();
            }
        }
    }
    
    // 内存移动和复制
    template<typename T>
    static void MoveElements(T* dest, T* src, size_t count)
    {
        if constexpr (std::is_trivially_move_constructible_v<T>)
        {
            memmove(dest, src, sizeof(T) * count);
        }
        else
        {
            if (dest < src)
            {
                for (size_t i = 0; i < count; ++i)
                {
                    new (dest + i) T(std::move(src[i]));
                    src[i].~T();
                }
            }
            else
            {
                for (size_t i = count; i > 0; --i)
                {
                    new (dest + i - 1) T(std::move(src[i - 1]));
                    src[i - 1].~T();
                }
            }
        }
    }
    
    template<typename T>
    static void CopyElements(T* dest, const T* src, size_t count)
    {
        if constexpr (std::is_trivially_copy_constructible_v<T>)
        {
            memcpy(dest, src, sizeof(T) * count);
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
            {
                new (dest + i) T(src[i]);
            }
        }
    }
    
    // 容量计算工具
    static size_t CalculateGrowth(size_t current, size_t requested)
    {
        if (requested <= current)
            return current;
            
        // 使用1.5倍增长策略
        size_t growth = current + current / 2;
        return growth > requested ? growth : requested;
    }
};

// 迭代器基类
template<typename T, typename ContainerType>
class NIteratorBase
{
public:
    using ValueType = T;
    using Pointer = T*;
    using Reference = T&;
    using ConstReference = const T&;
    
    NIteratorBase() : ptr_(nullptr) {}
    explicit NIteratorBase(Pointer ptr) : ptr_(ptr) {}
    
    Reference operator*() { return *ptr_; }
    ConstReference operator*() const { return *ptr_; }
    Pointer operator->() { return ptr_; }
    const Pointer operator->() const { return ptr_; }
    
    bool operator==(const NIteratorBase& other) const { return ptr_ == other.ptr_; }
    bool operator!=(const NIteratorBase& other) const { return ptr_ != other.ptr_; }
    
    NIteratorBase& operator++() { ++ptr_; return *this; }
    NIteratorBase operator++(int) { NIteratorBase temp(*this); ++ptr_; return temp; }
    
    NIteratorBase& operator--() { --ptr_; return *this; }
    NIteratorBase operator--(int) { NIteratorBase temp(*this); --ptr_; return temp; }
    
    NIteratorBase operator+(ptrdiff_t n) const { return NIteratorBase(ptr_ + n); }
    NIteratorBase operator-(ptrdiff_t n) const { return NIteratorBase(ptr_ - n); }
    
    ptrdiff_t operator-(const NIteratorBase& other) const { return ptr_ - other.ptr_; }
    
    bool operator<(const NIteratorBase& other) const { return ptr_ < other.ptr_; }
    bool operator<=(const NIteratorBase& other) const { return ptr_ <= other.ptr_; }
    bool operator>(const NIteratorBase& other) const { return ptr_ > other.ptr_; }
    bool operator>=(const NIteratorBase& other) const { return ptr_ >= other.ptr_; }

protected:
    Pointer ptr_;
};

// 哈希函数特化
template<typename T>
struct NHash
{
    size_t operator()(const T& value) const
    {
        if constexpr (std::is_integral_v<T>)
        {
            return static_cast<size_t>(value);
        }
        else if constexpr (std::is_pointer_v<T>)
        {
            return reinterpret_cast<size_t>(value);
        }
        else if constexpr (std::is_base_of_v<NObject, T>)
        {
            return value.GetHashCode();
        }
        else
        {
            // 通用哈希函数
            return DefaultHash(reinterpret_cast<const uint8_t*>(&value), sizeof(T));
        }
    }

private:
    static size_t DefaultHash(const uint8_t* data, size_t length)
    {
        // FNV-1a 哈希算法
        constexpr size_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
        constexpr size_t FNV_PRIME = 1099511628211ULL;
        
        size_t hash = FNV_OFFSET_BASIS;
        for (size_t i = 0; i < length; ++i)
        {
            hash ^= static_cast<size_t>(data[i]);
            hash *= FNV_PRIME;
        }
        return hash;
    }
};

// 相等比较函数特化
template<typename T>
struct NEqual
{
    bool operator()(const T& left, const T& right) const
    {
        if constexpr (std::is_base_of_v<NObject, T>)
        {
            return left.Equals(&right);
        }
        else
        {
            return left == right;
        }
    }
};