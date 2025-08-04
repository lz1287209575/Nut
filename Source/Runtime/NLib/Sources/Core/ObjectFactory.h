#pragma once

#include "Memory/MemoryManager.h"
#include "Object.h"

namespace NLib
{
// === NObject工厂函数实现 ===

/**
 * @brief 创建NObject的全局工厂函数实现
 * @tparam TType 对象类型（必须继承自NObject）
 * @tparam TArgs 构造函数参数类型
 * @param Args 构造函数参数
 * @return 托管对象的智能指针
 */
template <typename TType, typename... TArgs>
TSharedPtr<TType> NewNObject(TArgs&&... Args)
{
	static_assert(std::is_base_of_v<NObject, TType>, "TType must inherit from NObject");

	// 获取内存管理器实例
	extern CMemoryManager& GetMemoryManager();
	auto& MemMgr = GetMemoryManager();

	// 分配内存
	void* Memory = MemMgr.AllocateObject(sizeof(TType));
	if (!Memory)
	{
		throw std::bad_alloc();
	}

	try
	{
		// 使用placement new在分配的内存上构造对象
		TType* Obj = new (Memory) TType(std::forward<TArgs>(Args)...);
		return TSharedPtr<TType>(Obj);
	}
	catch (...)
	{
		// 构造失败时释放内存
		MemMgr.DeallocateObject(Memory);
		throw;
	}
}

} // namespace NLib