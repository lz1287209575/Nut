#pragma once

#include "Core/CObject.h"
#include "Memory/TSharedPtr.h"

namespace NLib
{

/**
 * @brief 共享引用智能指针 - 保证非空的NSharedPtr
 * 
 * 特点：
 * - 保证指针永远不为空
 * - 共享对象所有权
 * - 引用计数管理
 * - 比NSharedPtr更安全，无需空指针检查
 * - 可以转换为NSharedPtr
 */
template<typename TType>
class TSharedRef
{
public:
    using ElementType = T;

    // 构造函数 - 必须提供有效指针
    explicit TSharedRef(T* InPtr) : SharedPtr(InPtr)
    {
        checkf(InPtr != nullptr, TEXT("TSharedRef cannot be constructed with null pointer"));
    }

    explicit TSharedRef(T* InPtr, NSharedPtrControlBlock* InControlBlock) : SharedPtr(InPtr, InControlBlock)
    {
        checkf(InPtr != nullptr, TEXT("TSharedRef cannot be constructed with null pointer"));
    }

    // 从NSharedPtr构造 - 必须非空
    explicit TSharedRef(const TSharedPtr<T>& InSharedPtr) : SharedPtr(InSharedPtr)
    {
        checkf(InSharedPtr.IsValid(), TEXT("TSharedRef cannot be constructed from null TSharedPtr"));
    }

    explicit TSharedRef(TSharedPtr<T>&& InSharedPtr) : SharedPtr(MoveTemp(InSharedPtr))
    {
        checkf(SharedPtr.IsValid(), TEXT("TSharedRef cannot be constructed from null TSharedPtr"));
    }

    // 拷贝构造函数
    TSharedRef(const TSharedRef& Other) : SharedPtr(Other.SharedPtr) {}

    template<typename U>
    TSharedRef(const TSharedRef<U>& Other) : SharedPtr(Other.SharedPtr) {}

    // 移动构造函数
    TSharedRef(TSharedRef&& Other) noexcept : SharedPtr(MoveTemp(Other.SharedPtr)) {}

    template<typename U>
    TSharedRef(TSharedRef<U>&& Other) noexcept : SharedPtr(MoveTemp(Other.SharedPtr)) {}

    // 赋值操作符
    TSharedRef& operator=(const TSharedRef& Other)
    {
        SharedPtr = Other.SharedPtr;
        return *this;
    }

    template<typename U>
    TSharedRef& operator=(const TSharedRef<U>& Other)
    {
        SharedPtr = Other.SharedPtr;
        return *this;
    }

    TSharedRef& operator=(TSharedRef&& Other) noexcept
    {
        if (this != &Other)
        {
            SharedPtr = MoveTemp(Other.SharedPtr);
        }
        return *this;
    }

    template<typename U>
    TSharedRef& operator=(TSharedRef<U>&& Other) noexcept
    {
        SharedPtr = MoveTemp(Other.SharedPtr);
        return *this;
    }

    // 访问操作符 - 无需空指针检查
    T& operator*() const noexcept
    {
        return *SharedPtr;
    }

    T* operator->() const noexcept
    {
        return SharedPtr.Get();
    }

    // 获取原始指针 - 保证非空
    T* Get() const noexcept
    {
        return SharedPtr.Get();
    }

    // 引用计数
    int32_t GetSharedReferenceCount() const
    {
        return SharedPtr.GetSharedReferenceCount();
    }

    bool IsUnique() const
    {
        return SharedPtr.IsUnique();
    }

    // 转换为NSharedPtr
    TSharedPtr<T> ToSharedPtr() const
    {
        return SharedPtr;
    }

    operator TSharedPtr<T>() const
    {
        return SharedPtr;
    }

    // 获取控制块（内部使用）
    NSharedPtrControlBlock* GetControlBlock() const
    {
        return SharedPtr.GetControlBlock();
    }

    // 交换
    void Swap(TSharedRef& Other) noexcept
    {
        SharedPtr.Swap(Other.SharedPtr);
    }

private:
    TSharedPtr<T> SharedPtr;

    // 允许其他模板实例访问私有成员
    template<typename U> friend class TSharedRef;

    // 内部构造函数，用于类型转换
    template<typename U>
    TSharedRef(const TSharedPtr<U>& InSharedPtr, typename EnableIf<IsConvertible<U*, T*>::Value>::Type* = nullptr)
        : SharedPtr(InSharedPtr)
    {
        checkf(InSharedPtr.IsValid(), TEXT("TSharedRef cannot be constructed from null TSharedPtr"));
    }
};

// 比较操作符
template<typename TType, typename U>
bool operator==(const TSharedRef<T>& Left, const TSharedRef<U>& Right)
{
    return Left.Get() == Right.Get();
}

template<typename TType, typename U>
bool operator!=(const TSharedRef<T>& Left, const TSharedRef<U>& Right)
{
    return !(Left == Right);
}

template<typename TType, typename U>
bool operator<(const TSharedRef<T>& Left, const TSharedRef<U>& Right)
{
    return Left.Get() < Right.Get();
}

template<typename TType, typename U>
bool operator<=(const TSharedRef<T>& Left, const TSharedRef<U>& Right)
{
    return !(Right < Left);
}

template<typename TType, typename U>
bool operator>(const TSharedRef<T>& Left, const TSharedRef<U>& Right)
{
    return Right < Left;
}

template<typename TType, typename U>
bool operator>=(const TSharedRef<T>& Left, const TSharedRef<U>& Right)
{
    return !(Left < Right);
}

// 与NSharedPtr的比较
template<typename TType, typename U>
bool operator==(const TSharedRef<T>& Left, const TSharedPtr<U>& Right)
{
    return Left.Get() == Right.Get();
}

template<typename TType, typename U>
bool operator==(const TSharedPtr<T>& Left, const TSharedRef<U>& Right)
{
    return Left.Get() == Right.Get();
}

template<typename TType, typename U>
bool operator!=(const TSharedRef<T>& Left, const TSharedPtr<U>& Right)
{
    return !(Left == Right);
}

template<typename TType, typename U>
bool operator!=(const TSharedPtr<T>& Left, const TSharedRef<U>& Right)
{
    return !(Left == Right);
}

// 工厂函数
template<typename TType, typename... TArgs>
TSharedRef<T> MakeSharedRef(TArgs&&... Args)
{
    return TSharedRef<T>(MakeShared<T>(Forward<TArgs>(Args)...));
}

template<typename TType, typename... TArgs>
TSharedRef<T> MakeSharedRefWithAllocator(CAllocator* Allocator, TArgs&&... Args)
{
    return TSharedRef<T>(MakeSharedWithAllocator<T>(Allocator, Forward<TArgs>(Args)...));
}

// 类型转换函数
template<typename TType, typename U>
TSharedRef<T> StaticCastSharedRef(const TSharedRef<U>& InSharedRef)
{
    return TSharedRef<T>(StaticCastSharedPtr<T>(InSharedRef.ToSharedPtr()));
}

template<typename TType, typename U>
TSharedRef<T> DynamicCastSharedRef(const TSharedRef<U>& InSharedRef)
{
    TSharedPtr<T> CastedPtr = DynamicCastSharedPtr<T>(InSharedRef.ToSharedPtr());
    checkf(CastedPtr.IsValid(), TEXT("DynamicCastSharedRef failed - result would be null"));
    return TSharedRef<T>(CastedPtr);
}

template<typename TType, typename U>
TSharedRef<T> ConstCastSharedRef(const TSharedRef<U>& InSharedRef)
{
    return TSharedRef<T>(ConstCastSharedPtr<T>(InSharedRef.ToSharedPtr()));
}

// 交换函数
template<typename TType>
void Swap(TSharedRef<T>& Left, TSharedRef<T>& Right) noexcept
{
    Left.Swap(Right);
}

} // namespace NLib