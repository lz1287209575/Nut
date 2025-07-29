#pragma once

#include <cstddef>
#include <deque>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <new>
#include <set>
#include <string>
#include <vector>

#include "NMemoryManager.h"

namespace Nut
{

/**
 * @brief 基于tcmalloc的STL兼容分配器
 *
 * 为STL容器（如vector、map等）提供tcmalloc内存分配支持
 * 遵循C++标准分配器要求
 */
template <typename T> class NAllocator
{
  public:
    // STL分配器要求的类型定义
    using value_type = T;
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T &;
    using const_reference = const T &;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    // 重绑定模板，用于分配器转换
    template <typename U> struct rebind
    {
        using other = NAllocator<U>;
    };

    // 构造函数
    NAllocator() noexcept = default;

    // 拷贝构造函数
    NAllocator(const NAllocator &) noexcept = default;

    // 从其他类型的分配器构造
    template <typename U> NAllocator(const NAllocator<U> &) noexcept
    {
    }

    // 析构函数
    ~NAllocator() = default;

    /**
     * @brief 分配内存
     * @param Count 需要分配的对象数量
     * @return 分配的内存指针
     */
    pointer allocate(size_type Count);

    /**
     * @brief 释放内存
     * @param Ptr 要释放的内存指针
     * @param Count 对象数量（在C++17后可以忽略）
     */
    void deallocate(pointer Ptr, size_type Count) noexcept;

    /**
     * @brief 获取最大可分配对象数量
     * @return 最大对象数量
     */
    size_type max_size() const noexcept
    {
        return std::numeric_limits<size_type>::max() / sizeof(T);
    }

    /**
     * @brief 构造对象（C++17前需要）
     * @tparam Args 构造函数参数类型
     * @param Ptr 构造位置
     * @param Args 构造函数参数
     */
    template <typename... Args> void construct(pointer Ptr, Args &&...Arguments)
    {
        ::new (static_cast<void *>(Ptr)) T(std::forward<Args>(Arguments)...);
    }

    /**
     * @brief 销毁对象（C++17前需要）
     * @param Ptr 要销毁的对象指针
     */
    void destroy(pointer Ptr)
    {
        Ptr->~T();
    }

    // 比较操作符
    template <typename U> bool operator==(const NAllocator<U> &) const noexcept
    {
        return true; // 所有NAllocator实例都是相等的
    }

    template <typename U> bool operator!=(const NAllocator<U> &) const noexcept
    {
        return false;
    }
};

// === 模板特化实现 ===

template <typename T> typename NAllocator<T>::pointer NAllocator<T>::allocate(size_type Count)
{
    if (Count == 0)
    {
        return nullptr;
    }

    if (Count > max_size())
    {
        throw std::bad_array_new_length();
    }

    size_type Size = Count * sizeof(T);

    // 获取NMemoryManager实例并分配内存
    auto &MemMgr = NMemoryManager::GetInstance();
    void *Ptr = MemMgr.Allocate(Size);

    if (!Ptr)
    {
        throw std::bad_alloc();
    }

    return static_cast<pointer>(Ptr);
}

template <typename T> void NAllocator<T>::deallocate(pointer Ptr, size_type Count) noexcept
{
    (void)Count; // 避免未使用参数警告

    if (!Ptr)
    {
        return;
    }

    // 获取NMemoryManager实例并释放内存
    auto &MemMgr = NMemoryManager::GetInstance();
    MemMgr.Deallocate(static_cast<void *>(Ptr));
}

// === 常用容器类型别名 ===

template <typename TValue> using NVector = std::vector<TValue, NAllocator<TValue>>;

template <typename TKey, typename TValue>
using NMap = std::map<TKey, TValue, std::less<TKey>, NAllocator<std::pair<const TKey, TValue>>>;

template <typename TValue> using NSet = std::set<TValue, std::less<TValue>, NAllocator<TValue>>;

template <typename TValue> using NList = std::list<TValue, NAllocator<TValue>>;

template <typename TValue> using NDqueue = std::deque<TValue, NAllocator<TValue>>;

using NString = std::basic_string<char, std::char_traits<char>, NAllocator<char>>;

} // namespace Nut
