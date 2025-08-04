#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <typeinfo>

namespace NLib
{
// 前向声明
class CGarbageCollector;
class CMemoryManager;
template <typename TCharType, typename TAllocator>
class TString;
using CString = TString<char, CMemoryManager>;
struct SClassReflection;

// 容器前向声明
template <typename TElementType, typename TAllocator>
class TArray;

// 智能指针前向声明
template <typename TType>
class TSharedPtr;

template <typename TType>
class TWeakPtr;

template <typename TType>
class TSharedRef;

template <typename TType>
class TWeakRef;

/**
 * @brief NObject - 所有托管对象的基类
 *
 * 提供以下核心功能：
 * - 引用计数管理
 * - 垃圾回收支持
 * - 反射系统集成
 * - 对象生命周期管理
 * - 统一的内存管理
 */
class NObject
{
public:
	NObject();
	virtual ~NObject();

	// 禁用拷贝构造和赋值，强制使用智能指针管理
	NObject(const NObject&) = delete;
	NObject& operator=(const NObject&) = delete;

	// 允许移动语义
	NObject(NObject&& Other) noexcept;
	NObject& operator=(NObject&& Other) noexcept;

public:
	// === 引用计数管理 ===

	/**
	 * @brief 增加引用计数
	 * @return 新的引用计数值
	 */
	int32_t AddRef();

	/**
	 * @brief 减少引用计数，如果计数为0则销毁对象
	 * @return 新的引用计数值
	 */
	int32_t Release();

	/**
	 * @brief 获取当前引用计数
	 * @return 当前引用计数值
	 */
	int32_t GetRefCount() const;

public:
	// === 垃圾回收支持 ===

	/**
	 * @brief 标记对象为可达状态（GC标记阶段使用）
	 */
	void Mark();

	/**
	 * @brief 检查对象是否被标记
	 * @return true if marked, false otherwise
	 */
	bool IsMarked() const;

	/**
	 * @brief 清除标记（GC清理阶段使用）
	 */
	void UnMark();

	/**
	 * @brief 收集此对象引用的其他NObject
	 * 子类需要重写此方法来报告它们持有的其他NObject引用
	 * @param OutReferences 输出收集到的引用对象列表
	 */
	virtual void CollectReferences(TArray<NObject*, CMemoryManager>& OutReferences) const
	{
		(void)OutReferences; // 避免未使用参数警告
	}

public:
	// === 对象信息与反射 ===

	/**
	 * @brief 获取对象的类型信息
	 * @return 类型信息
	 */
	virtual const std::type_info& GetTypeInfo() const;

	/**
	 * @brief 获取对象的类型名称
	 * @return 类型名称字符串
	 */
	virtual const char* GetTypeName() const;

	/**
	 * @brief 获取类的反射信息
	 * @return 类反射信息指针，基类返回nullptr
	 */
	virtual const SClassReflection* GetClassReflection() const;

	/**
	 * @brief 获取静态类型名称
	 * @return 静态类型名称字符串
	 */
	static const char* GetStaticTypeName()
	{
		return "NObject";
	}

public:
	// === 对象生命周期 ===

	/**
	 * @brief 获取对象的唯一ID
	 * @return 对象ID
	 */
	uint64_t GetObjectId() const
	{
		return ObjectId;
	}

	/**
	 * @brief 检查对象是否有效（未被销毁）
	 * @return true if valid, false otherwise
	 */
	bool IsValid() const
	{
		return bIsValid.load();
	}

	/**
	 * @brief 销毁对象
	 * 不应该直接调用delete，而应该调用这个方法
	 */
	void Destroy();

public:
	// === 对象操作 ===

	/**
	 * @brief 比较两个对象是否相等
	 * @param Other 要比较的对象
	 * @return true if equal, false otherwise
	 */
	virtual bool Equals(const NObject* Other) const;

	/**
	 * @brief 获取对象的哈希码
	 * @return 哈希码
	 */
	virtual size_t GetHashCode() const;

	/**
	 * @brief 获取对象的字符串表示
	 * @return 字符串表示
	 */
	virtual CString ToString() const;

public:
	// === 内存管理 ===

	// 禁用标准new操作符，强制使用工厂函数
	static void* operator new(size_t Size) = delete;
	static void* operator new[](size_t Size) = delete;
	static void* operator new(size_t Size, std::align_val_t Alignment) = delete;

	// 允许placement new
	static void* operator new(size_t Size, void* Ptr) noexcept;

	// 自定义delete操作符
	static void operator delete(void* Ptr) noexcept;
	static void operator delete[](void* Ptr) noexcept;
	static void operator delete(void* Ptr, std::align_val_t Alignment) noexcept;
	static void operator delete(void* Ptr, void* PlacementPtr) noexcept;

protected:
	// === 内部状态 ===

	std::atomic<int32_t> RefCount; // 引用计数
	std::atomic<bool> bMarked;     // GC标记
	std::atomic<bool> bIsValid;    // 对象是否有效
	uint64_t ObjectId;             // 对象唯一ID

	static std::atomic<uint64_t> NextObjectId; // 下一个对象ID

private:
	// === GC系统友元 ===
	friend class CGarbageCollector;

	// GC专用方法
	void RegisterWithGC();
	void UnregisterFromGC();
};

// === 智能指针类型定义 ===
using NObjectPtr = TSharedPtr<NObject>;
using NObjectWeakPtr = TWeakPtr<NObject>;
using NObjectRef = TSharedRef<NObject>;
using NObjectWeakRef = TWeakRef<NObject>;

// === NObject工厂函数声明 ===

/**
 * @brief 创建NObject的全局工厂函数
 * @tparam TType 对象类型（必须继承自NObject）
 * @tparam TArgs 构造函数参数类型
 * @param Args 构造函数参数
 * @return 托管对象的智能指针
 */
template <typename TType, typename... TArgs>
TSharedPtr<TType> NewNObject(TArgs&&... Args);

} // namespace NLib