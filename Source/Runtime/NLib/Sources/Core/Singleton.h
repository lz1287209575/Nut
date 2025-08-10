#pragma once

#include "Object.h"

namespace NLib
{

/**
 * @brief 单例模式基类模板
 * @tparam T 单例类型
 */
template <typename T>
class TSingleton : public NObject
{
public:
	/**
	 * @brief 获取单例实例
	 * @return 单例实例引用
	 */
	static T& GetInstance()
	{
		static T Instance;
		return Instance;
	}

	/**
	 * @brief 获取单例实例指针
	 * @return 单例实例指针
	 */
	static T* GetInstancePtr()
	{
		return &GetInstance();
	}

protected:
	TSingleton() = default;
	virtual ~TSingleton() = default;

	// 禁止拷贝和赋值
	TSingleton(const TSingleton&) = delete;
	TSingleton& operator=(const TSingleton&) = delete;
	TSingleton(TSingleton&&) = delete;
	TSingleton& operator=(TSingleton&&) = delete;
};

// 类型别名，保持向后兼容
template <typename T>
using CSingleton = TSingleton<T>;

} // namespace NLib