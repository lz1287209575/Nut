#pragma once

#include "Core/CObject.h"
#include "Memory/TSharedPtr.h"

namespace NLib
{

// 前向声明
template<typename TType> class TSharedPtr;
template<typename TType> class TWeakPtr;

/**
 * @brief 弱引用智能指针 - 类似std::weak_ptr
 * 
 * 特点：
 * - 不拥有对象，只是观察对象
 * - 不影响对象的生命周期
 * - 可以检测对象是否还存在
 * - 可以转换为NSharedPtr来临时获取所有权
 * - 解决循环引用问题
 */
template<typename TType>
class TWeakPtr
{
public:
    using ElementType = T;

    // 构造函数
    constexpr TWeakPtr() noexcept : WeakPtr(nullptr) {}
    
    TWeakPtr(const TWeakPtr& Other) noexcept : WeakPtr(Other.WeakPtr)
    {
        if (WeakPtr)
        {
            WeakPtr->AddWeakRef();
        }
    }

    template<typename U>
    TWeakPtr(const TWeakPtr<U>& Other) noexcept : WeakPtr(Other.WeakPtr)
    {
        if (WeakPtr)
        {
            WeakPtr->AddWeakRef();
        }
    }

    template<typename U>
    TWeakPtr(const TSharedPtr<U>& Other) noexcept : WeakPtr(Other.GetControlBlock())
    {
        if (WeakPtr)
        {
            WeakPtr->AddWeakRef();
        }
    }

    // 移动构造函数
    TWeakPtr(TWeakPtr&& Other) noexcept : WeakPtr(Other.WeakPtr)
    {
        Other.WeakPtr = nullptr;
    }

    template<typename U>
    TWeakPtr(TWeakPtr<U>&& Other) noexcept : WeakPtr(Other.WeakPtr)
    {
        Other.WeakPtr = nullptr;
    }

    // 析构函数
    ~TWeakPtr()
    {
        if (WeakPtr)
        {
            WeakPtr->ReleaseWeakRef();
        }
    }

    // 赋值操作符
    TWeakPtr& operator=(const TWeakPtr& Other) noexcept
    {
        if (this != &Other)
        {
            if (WeakPtr)
            {
                WeakPtr->ReleaseWeakRef();
            }
            WeakPtr = Other.WeakPtr;
            if (WeakPtr)
            {
                WeakPtr->AddWeakRef();
            }
        }
        return *this;
    }

    template<typename U>
    TWeakPtr& operator=(const TWeakPtr<U>& Other) noexcept
    {
        if (WeakPtr)
        {
            WeakPtr->ReleaseWeakRef();
        }
        WeakPtr = Other.WeakPtr;
        if (WeakPtr)
        {
            WeakPtr->AddWeakRef();
        }
        return *this;
    }

    template<typename U>
    TWeakPtr& operator=(const TSharedPtr<U>& Other) noexcept
    {
        if (WeakPtr)
        {
            WeakPtr->ReleaseWeakRef();
        }
        WeakPtr = Other.GetControlBlock();
        if (WeakPtr)
        {
            WeakPtr->AddWeakRef();
        }
        return *this;
    }

    TWeakPtr& operator=(TWeakPtr&& Other) noexcept
    {
        if (this != &Other)
        {
            if (WeakPtr)
            {
                WeakPtr->ReleaseWeakRef();
            }
            WeakPtr = Other.WeakPtr;
            Other.WeakPtr = nullptr;
        }
        return *this;
    }

    template<typename U>
    TWeakPtr& operator=(TWeakPtr<U>&& Other) noexcept
    {
        if (WeakPtr)
        {
            WeakPtr->ReleaseWeakRef();
        }
        WeakPtr = Other.WeakPtr;
        Other.WeakPtr = nullptr;
        return *this;
    }

    // 观察操作
    int32_t UseCount() const noexcept
    {
        return WeakPtr ? WeakPtr->GetSharedRefCount() : 0;
    }

    bool IsExpired() const noexcept
    {
        return UseCount() == 0;
    }

    bool IsValid() const noexcept
    {
        return !IsExpired();
    }

    // 锁定操作 - 尝试获取NSharedPtr
    TSharedPtr<T> Lock() const noexcept
    {
        if (WeakPtr && WeakPtr->TryAddSharedRef())
        {
            return TSharedPtr<T>(WeakPtr->GetObjectPtr(), WeakPtr);
        }
        return TSharedPtr<T>();
    }

    // 重置
    void Reset() noexcept
    {
        if (WeakPtr)
        {
            WeakPtr->ReleaseWeakRef();
            WeakPtr = nullptr;
        }
    }

    // 交换
    void Swap(TWeakPtr& Other) noexcept
    {
        NLib::Swap(WeakPtr, Other.WeakPtr);
    }

    // 所有者比较（基于控制块地址）
    template<typename U>
    bool OwnerBefore(const TWeakPtr<U>& Other) const noexcept
    {
        return WeakPtr < Other.WeakPtr;
    }

    template<typename U>
    bool OwnerBefore(const TSharedPtr<U>& Other) const noexcept
    {
        return WeakPtr < Other.GetControlBlock();
    }

    // 获取控制块（内部使用）
    NSharedPtrControlBlock* GetControlBlock() const noexcept
    {
        return WeakPtr;
    }

private:
    NSharedPtrControlBlock* WeakPtr;

    // 允许其他模板实例访问私有成员
    template<typename U> friend class TWeakPtr;
    template<typename U> friend class TSharedPtr;
};

// 比较操作符
template<typename TType, typename U>
bool operator==(const TWeakPtr<T>& Left, const TWeakPtr<U>& Right) noexcept
{
    return Left.GetControlBlock() == Right.GetControlBlock();
}

template<typename TType, typename U>
bool operator!=(const TWeakPtr<T>& Left, const TWeakPtr<U>& Right) noexcept
{
    return !(Left == Right);
}

template<typename TType, typename U>
bool operator<(const TWeakPtr<T>& Left, const TWeakPtr<U>& Right) noexcept
{
    return Left.GetControlBlock() < Right.GetControlBlock();
}

template<typename TType, typename U>
bool operator<=(const TWeakPtr<T>& Left, const TWeakPtr<U>& Right) noexcept
{
    return !(Right < Left);
}

template<typename TType, typename U>
bool operator>(const TWeakPtr<T>& Left, const TWeakPtr<U>& Right) noexcept
{
    return Right < Left;
}

template<typename TType, typename U>
bool operator>=(const TWeakPtr<T>& Left, const TWeakPtr<U>& Right) noexcept
{
    return !(Left < Right);
}

// 交换函数
template<typename TType>
void Swap(TWeakPtr<T>& Left, TWeakPtr<T>& Right) noexcept
{
    Left.Swap(Right);
}

} // namespace NLib