#pragma once

#include "Containers/CArray.h"
#include "Core/CObject.h"

#include <functional>
#include <memory>
#include <type_traits>

namespace NLib
{

/**
 * @brief 委托基类 - 类似UE的委托系统
 * 提供类型安全的函数指针封装和调用机制
 */
class LIBNLIB_API NDelegateBase : public CObject
{
	NCLASS()
class NDelegateBase : public NObject
{
    GENERATED_BODY()

public:
	NDelegateBase() = default;
	virtual ~NDelegateBase() = default;

	/**
	 * @brief 检查委托是否已绑定
	 */
	virtual bool IsBound() const = 0;

	/**
	 * @brief 清除委托绑定
	 */
	virtual void Clear() = 0;

	/**
	 * @brief 获取绑定的函数数量
	 */
	virtual int32_t GetBoundFunctionCount() const = 0;
};

/**
 * @brief 单播委托 - 只能绑定一个函数
 * @tparam ReturnType 返回类型
 * @tparam Args 参数类型列表
 */
template <typename TReturnType, typename... Args>
class CDelegate : public NDelegateBase
{
	NCLASS()
class CDelegate : public NDelegateBase
{
    GENERATED_BODY()

public:
	using FunctionType = std::function<ReturnType(Args...)>;
	using RetType = ReturnType;

	CDelegate() = default;
	virtual ~CDelegate() = default;

	/**
	 * @brief 绑定普通函数
	 */
	void BindStatic(ReturnType (*Function)(Args...))
	{
		BoundFunction = Function;
	}

	/**
	 * @brief 绑定成员函数
	 */
	template <typename TObjectType>
	void BindUObject(ObjectType* Object, ReturnType (ObjectType::*Method)(Args...))
	{
		static_assert(std::is_base_of_v<CObject, ObjectType>, "ObjectType must inherit from CObject");

		if (!Object)
		{
			Clear();
			return;
		}

		BoundFunction = [Object, Method](Args... Arguments) -> ReturnType { return (Object->*Method)(Arguments...); };
		BoundObject = Object;
	}

	/**
	 * @brief 绑定Lambda表达式
	 */
	template <typename TLambdaType>
	void BindLambda(LambdaType&& Lambda)
	{
		BoundFunction = std::forward<LambdaType>(Lambda);
		BoundObject = nullptr;
	}

	/**
	 * @brief 执行委托
	 */
	ReturnType Execute(Args... Arguments) const
	{
		if (!IsBound())
		{
			if constexpr (!std::is_void_v<ReturnType>)
			{
				return ReturnType{};
			}
			else
			{
				return;
			}
		}

		return BoundFunction(Arguments...);
	}

	/**
	 * @brief 安全执行委托（检查对象有效性）
	 */
	ReturnType ExecuteIfBound(Args... Arguments) const
	{
		if (!IsSafe())
		{
			if constexpr (!std::is_void_v<ReturnType>)
			{
				return ReturnType{};
			}
			else
			{
				return;
			}
		}

		return Execute(Arguments...);
	}

	// NDelegateBase 接口实现
	virtual bool IsBound() const override
	{
		return static_cast<bool>(BoundFunction);
	}

	virtual void Clear() override
	{
		BoundFunction = nullptr;
		BoundObject = nullptr;
	}

	virtual int32_t GetBoundFunctionCount() const override
	{
		return IsBound() ? 1 : 0;
	}

	/**
	 * @brief 检查绑定的对象是否仍然有效
	 */
	bool IsSafe() const
	{
		if (!IsBound())
		{
			return false;
		}

		// 如果绑定了NObject，检查其有效性
		if (BoundObject && !BoundObject->IsValid())
		{
			return false;
		}

		return true;
	}

	/**
	 * @brief 获取绑定的对象
	 */
	CObject* GetBoundObject() const
	{
		return BoundObject;
	}

	// 操作符重载
	ReturnType operator()(Args... Arguments) const
	{
		return Execute(Arguments...);
	}

	bool operator==(const CDelegate& Other) const
	{
		// 简单比较：只有当两个委托都未绑定时才相等
		return !IsBound() && !Other.IsBound();
	}

	bool operator!=(const CDelegate& Other) const
	{
		return !(*this == Other);
	}

private:
	FunctionType BoundFunction;
	CObject* BoundObject = nullptr; // 用于检查对象有效性
};

/**
 * @brief 多播委托 - 可以绑定多个函数
 * @tparam Args 参数类型列表（多播委托不支持返回值）
 */
template <typename... Args>
class CMulticastDelegate : public NDelegateBase
{
	NCLASS()
class CMulticastDelegate : public NDelegateBase
{
    GENERATED_BODY()

public:
	using FunctionType = std::function<void(Args...)>;
	using DelegateType = CDelegate<void, Args...>;

	CMulticastDelegate() = default;
	virtual ~CMulticastDelegate() = default;

	/**
	 * @brief 添加静态函数
	 */
	void AddStatic(void (*Function)(Args...))
	{
		auto Delegate = NewNObject<DelegateType>();
		Delegate->BindStatic(Function);
		BoundDelegates.PushBack(Delegate);
	}

	/**
	 * @brief 添加成员函数
	 */
	template <typename TObjectType>
	void AddUObject(ObjectType* Object, void (ObjectType::*Method)(Args...))
	{
		auto Delegate = NewNObject<DelegateType>();
		Delegate->BindUObject(Object, Method);
		BoundDelegates.PushBack(Delegate);
	}

	/**
	 * @brief 添加Lambda
	 */
	template <typename TLambdaType>
	void AddLambda(LambdaType&& Lambda)
	{
		auto Delegate = NewNObject<DelegateType>();
		Delegate->BindLambda(std::forward<LambdaType>(Lambda));
		BoundDelegates.PushBack(Delegate);
	}

	/**
	 * @brief 移除所有与指定对象相关的委托
	 */
	void RemoveAll(const CObject* Object)
	{
		if (!Object)
		{
			return;
		}

		for (auto It = BoundDelegates.Begin(); It != BoundDelegates.End();)
		{
			if ((*It)->GetBoundObject() == Object)
			{
				It = BoundDelegates.Erase(It);
			}
			else
			{
				++It;
			}
		}
	}

	/**
	 * @brief 广播到所有绑定的函数
	 */
	void Broadcast(Args... Arguments) const
	{
		// 先收集所有有效的委托
		CArray<TSharedPtr<DelegateType>> ValidDelegates;
		for (const auto& Delegate : BoundDelegates)
		{
			if (Delegate->IsSafe())
			{
				ValidDelegates.PushBack(Delegate);
			}
		}

		// 执行所有有效的委托
		for (const auto& Delegate : ValidDelegates)
		{
			Delegate->Execute(Arguments...);
		}
	}

	// NDelegateBase 接口实现
	virtual bool IsBound() const override
	{
		return !BoundDelegates.IsEmpty();
	}

	virtual void Clear() override
	{
		BoundDelegates.Clear();
	}

	virtual int32_t GetBoundFunctionCount() const override
	{
		return static_cast<int32_t>(BoundDelegates.GetSize());
	}

	/**
	 * @brief 清理无效的委托
	 */
	void RemoveInvalidDelegates()
	{
		for (auto It = BoundDelegates.Begin(); It != BoundDelegates.End();)
		{
			if (!(*It)->IsSafe())
			{
				It = BoundDelegates.Erase(It);
			}
			else
			{
				++It;
			}
		}
	}

	// 操作符重载
	void operator()(Args... Arguments) const
	{
		Broadcast(Arguments...);
	}

private:
	CArray<TSharedPtr<DelegateType>> BoundDelegates;
};

// === 便利宏定义（类似UE风格） ===

/**
 * @brief 声明单播委托类型
 * 用法：DECLARE_DELEGATE_OneParam(FOnHealthChanged, float);
 */
#define DECLARE_DELEGATE(DelegateName) using DelegateName = NLib::CDelegate<void>

#define DECLARE_DELEGATE_OneParam(DelegateName, Param1Type) using DelegateName = NLib::CDelegate<void, Param1Type>

#define DECLARE_DELEGATE_TwoParams(DelegateName, Param1Type, Param2Type)                                               \
	using DelegateName = NLib::CDelegate<void, Param1Type, Param2Type>

#define DECLARE_DELEGATE_ThreeParams(DelegateName, Param1Type, Param2Type, Param3Type)                                 \
	using DelegateName = NLib::CDelegate<void, Param1Type, Param2Type, Param3Type>

/**
 * @brief 声明带返回值的委托
 */
#define DECLARE_DELEGATE_RetVal(ReturnType, DelegateName) using DelegateName = NLib::CDelegate<ReturnType>

#define DECLARE_DELEGATE_RetVal_OneParam(ReturnType, DelegateName, Param1Type)                                         \
	using DelegateName = NLib::CDelegate<ReturnType, Param1Type>

/**
 * @brief 声明多播委托类型
 */
#define DECLARE_MULTICAST_DELEGATE(DelegateName) using DelegateName = NLib::CMulticastDelegate<>

#define DECLARE_MULTICAST_DELEGATE_OneParam(DelegateName, Param1Type)                                                  \
	using DelegateName = NLib::CMulticastDelegate<Param1Type>

#define DECLARE_MULTICAST_DELEGATE_TwoParams(DelegateName, Param1Type, Param2Type)                                     \
	using DelegateName = NLib::CMulticastDelegate<Param1Type, Param2Type>

} // namespace NLib