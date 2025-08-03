#pragma once

#include "Core/CObject.h"
#include "Memory/CAllocator.h"

namespace NLib
{

/**
 * @brief 独占式智能指针 - 类似std::unique_ptr
 * 
 * 特点：
 * - 独占所有权，不能复制，只能移动
 * - 自动析构管理的对象
 * - 使用NAllocator内存管理系统
 * - 支持自定义删除器
 * - 零开销抽象
 */
template<typename TType, typename TDeleter = NAllocatorDeleter<T>>
class CUniquePtr
{
public:
    using ElementType = T;
    using DeleterType = TDeleter;
    using PointerType = T*;

    // 构造函数
    constexpr CUniquePtr() noexcept : Ptr(nullptr), Deleter() {}
    constexpr CUniquePtr(NullPtr) noexcept : Ptr(nullptr), Deleter() {}
    
    explicit CUniquePtr(PointerType InPtr) noexcept : Ptr(InPtr), Deleter() {}
    
    CUniquePtr(PointerType InPtr, const DeleterType& InDeleter) noexcept 
        : Ptr(InPtr), Deleter(InDeleter) {}
    
    CUniquePtr(PointerType InPtr, DeleterType&& InDeleter) noexcept 
        : Ptr(InPtr), Deleter(MoveTemp(InDeleter)) {}

    // 移动构造函数
    CUniquePtr(CUniquePtr&& Other) noexcept 
        : Ptr(Other.Release()), Deleter(MoveTemp(Other.Deleter)) {}

    template<typename U, typename TType>
    CUniquePtr(CUniquePtr<U, E>&& Other) noexcept 
        : Ptr(Other.Release()), Deleter(MoveTemp(Other.GetDeleter())) {}

    // 析构函数
    ~CUniquePtr()
    {
        if (Ptr)
        {
            Deleter(Ptr);
        }
    }

    // 禁用拷贝构造和拷贝赋值
    CUniquePtr(const CUniquePtr&) = delete;
    CUniquePtr& operator=(const CUniquePtr&) = delete;

    // 移动赋值
    CUniquePtr& operator=(CUniquePtr&& Other) noexcept
    {
        if (this != &Other)
        {
            Reset(Other.Release());
            Deleter = MoveTemp(Other.Deleter);
        }
        return *this;
    }

    template<typename U, typename TType>
    CUniquePtr& operator=(CUniquePtr<U, E>&& Other) noexcept
    {
        Reset(Other.Release());
        Deleter = MoveTemp(Other.GetDeleter());
        return *this;
    }

    CUniquePtr& operator=(NullPtr) noexcept
    {
        Reset();
        return *this;
    }

    // 访问操作符
    T& operator*() const noexcept
    {
        checkSlow(Ptr != nullptr);
        return *Ptr;
    }

    PointerType operator->() const noexcept
    {
        checkSlow(Ptr != nullptr);
        return Ptr;
    }

    // 获取原始指针
    PointerType Get() const noexcept { return Ptr; }

    // 检查是否有效
    explicit operator bool() const noexcept { return Ptr != nullptr; }
    bool IsValid() const noexcept { return Ptr != nullptr; }

    // 释放所有权
    PointerType Release() noexcept
    {
        PointerType Result = Ptr;
        Ptr = nullptr;
        return Result;
    }

    // 重置指针
    void Reset(PointerType NewPtr = nullptr) noexcept
    {
        PointerType OldPtr = Ptr;
        Ptr = NewPtr;
        if (OldPtr)
        {
            Deleter(OldPtr);
        }
    }

    // 交换
    void Swap(CUniquePtr& Other) noexcept
    {
        NLib::Swap(Ptr, Other.Ptr);
        NLib::Swap(Deleter, Other.Deleter);
    }

    // 获取删除器
    DeleterType& GetDeleter() noexcept { return Deleter; }
    const DeleterType& GetDeleter() const noexcept { return Deleter; }

private:
    PointerType Ptr;
    DeleterType Deleter;
};

/**
 * @brief 数组版本的独占智能指针
 */
template<typename TType, typename TDeleter>
class CUniquePtr<T[], TDeleter>
{
public:
    using ElementType = T;
    using DeleterType = TDeleter;
    using PointerType = T*;

    // 构造函数
    constexpr CUniquePtr() noexcept : Ptr(nullptr), Deleter() {}
    constexpr CUniquePtr(NullPtr) noexcept : Ptr(nullptr), Deleter() {}
    
    explicit CUniquePtr(PointerType InPtr) noexcept : Ptr(InPtr), Deleter() {}
    
    CUniquePtr(PointerType InPtr, const DeleterType& InDeleter) noexcept 
        : Ptr(InPtr), Deleter(InDeleter) {}
    
    CUniquePtr(PointerType InPtr, DeleterType&& InDeleter) noexcept 
        : Ptr(InPtr), Deleter(MoveTemp(InDeleter)) {}

    // 移动构造函数
    CUniquePtr(CUniquePtr&& Other) noexcept 
        : Ptr(Other.Release()), Deleter(MoveTemp(Other.Deleter)) {}

    // 析构函数
    ~CUniquePtr()
    {
        if (Ptr)
        {
            Deleter(Ptr);
        }
    }

    // 禁用拷贝
    CUniquePtr(const CUniquePtr&) = delete;
    CUniquePtr& operator=(const CUniquePtr&) = delete;

    // 移动赋值
    CUniquePtr& operator=(CUniquePtr&& Other) noexcept
    {
        if (this != &Other)
        {
            Reset(Other.Release());
            Deleter = MoveTemp(Other.Deleter);
        }
        return *this;
    }

    CUniquePtr& operator=(NullPtr) noexcept
    {
        Reset();
        return *this;
    }

    // 数组访问
    T& operator[](int32_t Index) const noexcept
    {
        checkSlow(Ptr != nullptr);
        return Ptr[Index];
    }

    // 获取原始指针
    PointerType Get() const noexcept { return Ptr; }

    // 检查是否有效
    explicit operator bool() const noexcept { return Ptr != nullptr; }
    bool IsValid() const noexcept { return Ptr != nullptr; }

    // 释放所有权
    PointerType Release() noexcept
    {
        PointerType Result = Ptr;
        Ptr = nullptr;
        return Result;
    }

    // 重置指针
    void Reset(PointerType NewPtr = nullptr) noexcept
    {
        PointerType OldPtr = Ptr;
        Ptr = NewPtr;
        if (OldPtr)
        {
            Deleter(OldPtr);
        }
    }

    // 交换
    void Swap(CUniquePtr& Other) noexcept
    {
        NLib::Swap(Ptr, Other.Ptr);
        NLib::Swap(Deleter, Other.Deleter);
    }

    // 获取删除器
    DeleterType& GetDeleter() noexcept { return Deleter; }
    const DeleterType& GetDeleter() const noexcept { return Deleter; }

private:
    PointerType Ptr;
    DeleterType Deleter;
};

/**
 * @brief NAllocator删除器 - 使用NAllocator系统进行内存管理
 */
template<typename TType>
struct NAllocatorDeleter
{
    CAllocator* Allocator;

    NAllocatorDeleter() : Allocator(CAllocator::GetDefaultAllocator()) {}
    explicit NAllocatorDeleter(CAllocator* InAllocator) : Allocator(InAllocator) {}

    void operator()(T* Ptr) const noexcept
    {
        if (Ptr && Allocator)
        {
            // 调用析构函数
            Ptr->~T();
            // 释放内存
            Allocator->Free(Ptr);
        }
    }
};

/**
 * @brief 数组版本的NAllocator删除器
 */
template<typename TType>
struct NAllocatorDeleter<T[]>
{
    CAllocator* Allocator;
    int32_t ArraySize;

    NAllocatorDeleter() : Allocator(CAllocator::GetDefaultAllocator()), ArraySize(0) {}
    explicit NAllocatorDeleter(CAllocator* InAllocator, int32_t InArraySize = 0) 
        : Allocator(InAllocator), ArraySize(InArraySize) {}

    void operator()(T* Ptr) const noexcept
    {
        if (Ptr && Allocator)
        {
            // 如果知道数组大小，逐个调用析构函数
            if (ArraySize > 0)
            {
                for (int32_t i = ArraySize - 1; i >= 0; --i)
                {
                    Ptr[i].~T();
                }
            }
            // 释放内存
            Allocator->Free(Ptr);
        }
    }
};

/**
 * @brief 简单删除器 - 仅用于调试或特殊情况，不推荐在生产环境使用
 */
template<typename TType>
struct NSimpleDeleter
{
    void operator()(T* Ptr) const noexcept
    {
        delete Ptr;
    }
};

template<typename TType>
struct NSimpleDeleter<T[]>
{
    void operator()(T* Ptr) const noexcept
    {
        delete[] Ptr;
    }
};

// 比较操作符
template<typename T1, typename TDeleter1, typename T2, typename TDeleter2>
bool operator==(const CUniquePtr<T1, D1>& Left, const CUniquePtr<T2, D2>& Right)
{
    return Left.Get() == Right.Get();
}

template<typename T1, typename TDeleter1, typename T2, typename TDeleter2>
bool operator!=(const CUniquePtr<T1, D1>& Left, const CUniquePtr<T2, D2>& Right)
{
    return !(Left == Right);
}

template<typename T1, typename TDeleter1, typename T2, typename TDeleter2>
bool operator<(const CUniquePtr<T1, D1>& Left, const CUniquePtr<T2, D2>& Right)
{
    return Left.Get() < Right.Get();
}

template<typename TType, typename TDeleter>
bool operator==(const CUniquePtr<T, D>& Left, NullPtr)
{
    return !Left;
}

template<typename TType, typename TDeleter>
bool operator==(NullPtr, const CUniquePtr<T, D>& Right)
{
    return !Right;
}

template<typename TType, typename TDeleter>
bool operator!=(const CUniquePtr<T, D>& Left, NullPtr)
{
    return static_cast<bool>(Left);
}

template<typename TType, typename TDeleter>
bool operator!=(NullPtr, const CUniquePtr<T, D>& Right)
{
    return static_cast<bool>(Right);
}

// 工厂函数 - 使用NAllocator分配内存
template<typename TType, typename... TArgs>
CUniquePtr<T> MakeUnique(TArgs&&... Args)
{
    CAllocator* Allocator = CAllocator::GetDefaultAllocator();
    T* Ptr = static_cast<T*>(Allocator->Allocate(sizeof(T), alignof(T)));
    new(Ptr) T(Forward<TArgs>(Args)...);
    return CUniquePtr<T>(Ptr, NAllocatorDeleter<T>(Allocator));
}

template<typename TType, typename... TArgs>
CUniquePtr<T> MakeUniqueWithAllocator(CAllocator* Allocator, TArgs&&... Args)
{
    T* Ptr = static_cast<T*>(Allocator->Allocate(sizeof(T), alignof(T)));
    new(Ptr) T(Forward<TArgs>(Args)...);
    return CUniquePtr<T>(Ptr, NAllocatorDeleter<T>(Allocator));
}

template<typename TType>
CUniquePtr<T[]> MakeUniqueArray(int32_t Size)
{
    using ElementType = typename RemoveExtent<T>::Type;
    CAllocator* Allocator = CAllocator::GetDefaultAllocator();
    ElementType* Ptr = static_cast<ElementType*>(Allocator->Allocate(sizeof(ElementType) * Size, alignof(ElementType)));
    
    // 构造数组元素
    for (int32_t i = 0; i < Size; ++i)
    {
        new(Ptr + i) ElementType();
    }
    
    return CUniquePtr<T[]>(Ptr, NAllocatorDeleter<T[]>(Allocator, Size));
}

template<typename TType>
CUniquePtr<T[]> MakeUniqueArrayWithAllocator(CAllocator* Allocator, int32_t Size)
{
    using ElementType = typename RemoveExtent<T>::Type;
    ElementType* Ptr = static_cast<ElementType*>(Allocator->Allocate(sizeof(ElementType) * Size, alignof(ElementType)));
    
    // 构造数组元素
    for (int32_t i = 0; i < Size; ++i)
    {
        new(Ptr + i) ElementType();
    }
    
    return CUniquePtr<T[]>(Ptr, NAllocatorDeleter<T[]>(Allocator, Size));
}

// 交换函数
template<typename TType, typename TDeleter>
void Swap(CUniquePtr<T, D>& Left, CUniquePtr<T, D>& Right) noexcept
{
    Left.Swap(Right);
}

} // namespace NLib