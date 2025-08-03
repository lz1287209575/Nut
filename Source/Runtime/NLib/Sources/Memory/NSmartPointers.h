#pragma once

/**
 * @file NSmartPointers.h
 * @brief 完整的智能指针系统
 * 
 * 这个文件包含了NLib的完整智能指针实现：
 * - CUniquePtr: 独占智能指针
 * - TSharedPtr: 共享智能指针
 * - TWeakPtr: 弱引用智能指针
 * - TSharedRef: 非空共享引用
 * - TWeakRef: 弱引用
 * 
 * 所有智能指针都与NAllocator系统集成，提供高性能的内存管理。
 */

#include "Memory/CUniquePtr.h"
#include "Memory/TSharedPtr.h"
#include "Memory/TWeakPtr.h"
#include "Memory/TSharedRef.h"
#include "Memory/TWeakRef.h"
#include "Memory/CSharedFromThis.h"

namespace NLib
{

/**
 * @brief 智能指针工厂函数总览
 * 
 * 独占指针：
 * - MakeUnique<T>(Args...) - 创建独占指针
 * - MakeUniqueWithAllocator<T>(Allocator, Args...) - 使用指定分配器
 * - MakeUniqueArray<T>(Size) - 创建数组
 * 
 * 共享指针：
 * - MakeShared<T>(Args...) - 创建共享指针（推荐）
 * - MakeSharedWithAllocator<T>(Allocator, Args...) - 使用指定分配器
 * 
 * 共享引用：
 * - MakeSharedRef<T>(Args...) - 创建非空共享引用
 * - MakeSharedRefWithAllocator<T>(Allocator, Args...) - 使用指定分配器
 * 
 * 弱引用：
 * - MakeWeakRef<T>(SharedPtr) - 从共享指针创建弱引用
 * 
 * 类型转换：
 * - StaticCast*, DynamicCast*, ConstCast* - 各种类型转换
 */

/**
 * @brief 智能指针使用指南
 * 
 * 1. 独占所有权：使用 CUniquePtr<T>
 *    - 当对象只有一个所有者时
 *    - 自动析构，零开销
 *    - 可移动，不可复制
 * 
 * 2. 共享所有权：使用 TSharedPtr<T>
 *    - 当对象有多个所有者时
 *    - 引用计数管理
 *    - 线程安全
 * 
 * 3. 观察对象：使用 TWeakPtr<T>
 *    - 不拥有对象，只观察
 *    - 解决循环引用问题
 *    - 可检测对象是否还存在
 * 
 * 4. 保证非空：使用 TSharedRef<T>
 *    - 保证指针永远不为空
 *    - 无需空指针检查
 *    - 更安全的API
 * 
 * 5. 弱观察：使用 TWeakRef<T>
 *    - NWeakPtr的引用版本
 *    - 提供更安全的接口
 * 
 * 6. 自引用：继承 CSharedFromThis<T>
 *    - 允许对象从自身获取NSharedPtr
 *    - 避免双重管理同一对象
 *    - 提供SharedFromThis()方法
 */

/**
 * @brief 性能考虑
 * 
 * 1. 优先使用 MakeShared 而不是 TSharedPtr(new T)
 *    - 减少内存分配次数
 *    - 更好的局部性
 * 
 * 2. 对于简单对象，考虑使用 CUniquePtr
 *    - 零开销抽象
 *    - 没有引用计数开销
 * 
 * 3. 避免过度使用弱指针
 *    - 只在需要打破循环引用时使用
 *    - Lock操作有一定开销
 * 
 * 4. 使用适当的分配器
 *    - 默认使用NAllocator
 *    - 特殊需求可使用自定义分配器
 */

// 类型别名 - 便于使用
template<typename TType> using UniquePtr = CUniquePtr<T>;
template<typename TType> using SharedPtr = TSharedPtr<T>;
template<typename TType> using WeakPtr = TWeakPtr<T>;
template<typename TType> using SharedRef = TSharedRef<T>;
template<typename TType> using WeakRef = TWeakRef<T>;

} // namespace NLib