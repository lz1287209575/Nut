#pragma once

#include "Core/CObject.h"
#include "Memory/TWeakPtr.h"
#include "Memory/TSharedRef.h"

namespace NLib
{

/**
 * @brief 弱引用智能指针的引用版本
 * 
 * 特点：
 * - 基于NWeakPtr，但提供引用语义
 * - 不拥有对象，只是观察对象
 * - 可以检测对象是否还存在
 * - 可以转换为NSharedRef来临时获取所有权
 * - 解决循环引用问题
 * - 提供更安全的接口
 */
template<typename TType>
class TWeakRef
{
public:
    using ElementType = T;

    // 构造函数
    TWeakRef() = default;

    TWeakRef(const TWeakRef& Other) : WeakPtr(Other.WeakPtr) {}

    template<typename U>
    TWeakRef(const TWeakRef<U>& Other) : WeakPtr(Other.WeakPtr) {}

    template<typename U>
    TWeakRef(const TSharedRef<U>& Other) : WeakPtr(Other.ToSharedPtr()) {}

    template<typename U>
    TWeakRef(const TSharedPtr<U>& Other) : WeakPtr(Other) {}

    template<typename U>
    TWeakRef(const TWeakPtr<U>& Other) : WeakPtr(Other) {}

    // 移动构造函数
    TWeakRef(TWeakRef&& Other) noexcept : WeakPtr(MoveTemp(Other.WeakPtr)) {}

    template<typename U>
    TWeakRef(TWeakRef<U>&& Other) noexcept : WeakPtr(MoveTemp(Other.WeakPtr)) {}

    // 赋值操作符
    TWeakRef& operator=(const TWeakRef& Other)
    {
        WeakPtr = Other.WeakPtr;
        return *this;
    }

    template<typename U>
    TWeakRef& operator=(const TWeakRef<U>& Other)
    {
        WeakPtr = Other.WeakPtr;
        return *this;
    }

    template<typename U>
    TWeakRef& operator=(const TSharedRef<U>& Other)
    {
        WeakPtr = Other.ToSharedPtr();
        return *this;
    }

    template<typename U>
    TWeakRef& operator=(const TSharedPtr<U>& Other)
    {
        WeakPtr = Other;
        return *this;
    }

    template<typename U>
    TWeakRef& operator=(const TWeakPtr<U>& Other)
    {
        WeakPtr = Other;
        return *this;
    }

    TWeakRef& operator=(TWeakRef&& Other) noexcept
    {
        if (this != &Other)
        {
            WeakPtr = MoveTemp(Other.WeakPtr);
        }
        return *this;
    }

    template<typename U>
    TWeakRef& operator=(TWeakRef<U>&& Other) noexcept
    {
        WeakPtr = MoveTemp(Other.WeakPtr);
        return *this;
    }

    // 观察操作
    int32_t UseCount() const noexcept
    {
        return WeakPtr.UseCount();
    }

    bool IsExpired() const noexcept
    {
        return WeakPtr.IsExpired();
    }

    bool IsValid() const noexcept
    {
        return WeakPtr.IsValid();
    }

    // 锁定操作 - 尝试获取NSharedRef
    TOptional<TSharedRef<T>> Pin() const noexcept
    {
        TSharedPtr<T> LockedPtr = WeakPtr.Lock();
        if (LockedPtr.IsValid())
        {
            return TSharedRef<T>(LockedPtr);
        }
        return {};
    }

    // 锁定操作 - 尝试获取NSharedPtr
    TSharedPtr<T> Lock() const noexcept
    {
        return WeakPtr.Lock();
    }

    // 重置
    void Reset() noexcept
    {
        WeakPtr.Reset();
    }

    // 交换
    void Swap(TWeakRef& Other) noexcept
    {
        WeakPtr.Swap(Other.WeakPtr);
    }

    // 所有者比较
    template<typename U>
    bool OwnerBefore(const TWeakRef<U>& Other) const noexcept
    {
        return WeakPtr.OwnerBefore(Other.WeakPtr);
    }

    template<typename U>
    bool OwnerBefore(const TSharedRef<U>& Other) const noexcept
    {
        return WeakPtr.OwnerBefore(Other.ToSharedPtr());
    }

    template<typename U>
    bool OwnerBefore(const TSharedPtr<U>& Other) const noexcept
    {
        return WeakPtr.OwnerBefore(Other);
    }

    template<typename U>
    bool OwnerBefore(const TWeakPtr<U>& Other) const noexcept
    {
        return WeakPtr.OwnerBefore(Other);
    }

    // 转换为NWeakPtr
    const TWeakPtr<T>& ToWeakPtr() const noexcept
    {
        return WeakPtr;
    }

    operator TWeakPtr<T>() const noexcept
    {
        return WeakPtr;
    }

    // 获取控制块（内部使用）
    NSharedPtrControlBlock* GetControlBlock() const noexcept
    {
        return WeakPtr.GetControlBlock();
    }

private:
    TWeakPtr<T> WeakPtr;

    // 允许其他模板实例访问私有成员
    template<typename U> friend class TWeakRef;
};

// 比较操作符
template<typename TType, typename U>
bool operator==(const TWeakRef<T>& Left, const TWeakRef<U>& Right) noexcept
{
    return Left.ToWeakPtr() == Right.ToWeakPtr();
}

template<typename TType, typename U>
bool operator!=(const TWeakRef<T>& Left, const TWeakRef<U>& Right) noexcept
{
    return !(Left == Right);
}

template<typename TType, typename U>
bool operator<(const TWeakRef<T>& Left, const TWeakRef<U>& Right) noexcept
{
    return Left.ToWeakPtr() < Right.ToWeakPtr();
}

template<typename TType, typename U>
bool operator<=(const TWeakRef<T>& Left, const TWeakRef<U>& Right) noexcept
{
    return !(Right < Left);
}

template<typename TType, typename U>
bool operator>(const TWeakRef<T>& Left, const TWeakRef<U>& Right) noexcept
{
    return Right < Left;
}

template<typename TType, typename U>
bool operator>=(const TWeakRef<T>& Left, const TWeakRef<U>& Right) noexcept
{
    return !(Left < Right);
}

// 与其他智能指针类型的比较
template<typename TType, typename U>
bool operator==(const TWeakRef<T>& Left, const TWeakPtr<U>& Right) noexcept
{
    return Left.ToWeakPtr() == Right;
}

template<typename TType, typename U>
bool operator==(const TWeakPtr<T>& Left, const TWeakRef<U>& Right) noexcept
{
    return Left == Right.ToWeakPtr();
}

template<typename TType, typename U>
bool operator!=(const TWeakRef<T>& Left, const TWeakPtr<U>& Right) noexcept
{
    return !(Left == Right);
}

template<typename TType, typename U>
bool operator!=(const TWeakPtr<T>& Left, const TWeakRef<U>& Right) noexcept
{
    return !(Left == Right);
}

// 交换函数
template<typename TType>
void Swap(TWeakRef<T>& Left, TWeakRef<T>& Right) noexcept
{
    Left.Swap(Right);
}

// 便利函数 - 创建NWeakRef
template<typename TType>
TWeakRef<T> MakeWeakRef(const TSharedRef<T>& InSharedRef)
{
    return TWeakRef<T>(InSharedRef);
}

template<typename TType>
TWeakRef<T> MakeWeakRef(const TSharedPtr<T>& InSharedPtr)
{
    return TWeakRef<T>(InSharedPtr);
}

} // namespace NLib