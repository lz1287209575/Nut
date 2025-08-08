#pragma once

#include "Containers/TArray.h"
#include "Containers/THashMap.h"
#include "Core/Object.h"
#include "Core/SmartPointers.h"
#include "Logging/LogCategory.h"

#include <atomic>
#include <functional>
#include <type_traits>

namespace NLib
{
/**
 * @brief 委托句柄类型
 */
using FDelegateHandle = uint64_t;

/**
 * @brief 无效的委托句柄
 */
constexpr FDelegateHandle INVALID_DELEGATE_HANDLE = 0;

/**
 * @brief 委托绑定信息
 */
struct SDelegateBinding
{
	FDelegateHandle Handle = INVALID_DELEGATE_HANDLE;
	bool bIsValid = false;
	bool bIsOneShot = false; // 是否为一次性委托

	SDelegateBinding() = default;
	SDelegateBinding(FDelegateHandle InHandle, bool bInOneShot = false)
	    : Handle(InHandle),
	      bIsValid(true),
	      bIsOneShot(bInOneShot)
	{}

	bool IsValid() const
	{
		return bIsValid && Handle != INVALID_DELEGATE_HANDLE;
	}
	void Invalidate()
	{
		bIsValid = false;
	}
};

/**
 * @brief 基础委托接口
 */
class IDelegateBase
{
public:
	virtual ~IDelegateBase() = default;

	/**
	 * @brief 获取委托ID
	 */
	virtual FDelegateHandle GetHandle() const = 0;

	/**
	 * @brief 检查委托是否有效
	 */
	virtual bool IsValid() const = 0;

	/**
	 * @brief 清空委托
	 */
	virtual void Clear() = 0;

	/**
	 * @brief 获取绑定函数数量
	 */
	virtual int32_t GetBindingCount() const = 0;
};

/**
 * @brief 单播委托 - 只能绑定一个函数
 */
template <typename TSignature>
class TDelegate;

template <typename TReturnType, typename... TArgs>
class TDelegate<TReturnType(TArgs...)> : public IDelegateBase
{
public:
	using FunctionType = std::function<TReturnType(TArgs...)>;
	using ReturnType = TReturnType;

private:
	static std::atomic<FDelegateHandle> NextHandle;

public:
	// === 构造函数 ===

	TDelegate()
	    : Handle(INVALID_DELEGATE_HANDLE)
	{}

	~TDelegate() = default;

	// 允许移动但禁止拷贝
	TDelegate(const TDelegate&) = delete;
	TDelegate& operator=(const TDelegate&) = delete;
	TDelegate(TDelegate&& Other) noexcept
	    : Function(std::move(Other.Function)),
	      Handle(Other.Handle)
	{
		Other.Handle = INVALID_DELEGATE_HANDLE;
	}
	TDelegate& operator=(TDelegate&& Other) noexcept
	{
		if (this != &Other)
		{
			Function = std::move(Other.Function);
			Handle = Other.Handle;
			Other.Handle = INVALID_DELEGATE_HANDLE;
		}
		return *this;
	}

public:
	// === 绑定函数 ===

	/**
	 * @brief 绑定普通函数
	 */
	template <typename TFunc>
	FDelegateHandle BindUFunction(TFunc&& Func)
	{
		static_assert(std::is_invocable_r_v<TReturnType, TFunc, TArgs...>, "Function signature mismatch");

		Function = std::forward<TFunc>(Func);
		Handle = NextHandle.fetch_add(1) + 1;

		NLOG_EVENTS(Trace, "Delegate bound to function, handle: {}", Handle);
		return Handle;
	}

	/**
	 * @brief 绑定成员函数
	 */
	template <typename TObject, typename TFunc>
	FDelegateHandle BindUObject(TObject* Object, TFunc&& Func)
	{
		static_assert(std::is_invocable_r_v<TReturnType, TFunc, TObject*, TArgs...>,
		              "Member function signature mismatch");

		if (!Object)
		{
			NLOG_EVENTS(Error, "Cannot bind delegate to null object");
			return INVALID_DELEGATE_HANDLE;
		}

		Function = [Object, Func](TArgs... Args) -> TReturnType {
			if constexpr (std::is_void_v<TReturnType>)
			{
				(Object->*Func)(Args...);
			}
			else
			{
				return (Object->*Func)(Args...);
			}
		};

		Handle = NextHandle.fetch_add(1) + 1;

		NLOG_EVENTS(Trace, "Delegate bound to member function, handle: {}", Handle);
		return Handle;
	}

	/**
	 * @brief 绑定Lambda表达式
	 */
	template <typename TLambda>
	FDelegateHandle BindLambda(TLambda&& Lambda)
	{
		static_assert(std::is_invocable_r_v<TReturnType, TLambda, TArgs...>, "Lambda signature mismatch");

		Function = std::forward<TLambda>(Lambda);
		Handle = NextHandle.fetch_add(1) + 1;

		NLOG_EVENTS(Trace, "Delegate bound to lambda, handle: {}", Handle);
		return Handle;
	}

	/**
	 * @brief 绑定智能指针对象的成员函数
	 */
	template <typename TObject, typename TFunc>
	FDelegateHandle BindSP(TSharedPtr<TObject> Object, TFunc&& Func)
	{
		static_assert(std::is_invocable_r_v<TReturnType, TFunc, TObject*, TArgs...>,
		              "Member function signature mismatch");

		if (!Object.IsValid())
		{
			NLOG_EVENTS(Error, "Cannot bind delegate to invalid shared pointer");
			return INVALID_DELEGATE_HANDLE;
		}

		Function = [Object, Func](TArgs... Args) -> TReturnType {
			if (Object.IsValid())
			{
				if constexpr (std::is_void_v<TReturnType>)
				{
					(Object.Get()->*Func)(Args...);
				}
				else
				{
					return (Object.Get()->*Func)(Args...);
				}
			}
			else if constexpr (!std::is_void_v<TReturnType>)
			{
				return TReturnType{};
			}
		};

		Handle = NextHandle.fetch_add(1) + 1;

		NLOG_EVENTS(Trace, "Delegate bound to shared pointer member function, handle: {}", Handle);
		return Handle;
	}

public:
	// === 执行委托 ===

	/**
	 * @brief 执行委托
	 */
	TReturnType Execute(TArgs... Args) const
	{
		if (!IsValid())
		{
			NLOG_EVENTS(Warning, "Attempting to execute invalid delegate");
			if constexpr (!std::is_void_v<TReturnType>)
			{
				return TReturnType{};
			}
			return;
		}

		try
		{
			if constexpr (std::is_void_v<TReturnType>)
			{
				Function(Args...);
			}
			else
			{
				return Function(Args...);
			}
		}
		catch (...)
		{
			NLOG_EVENTS(Error, "Exception thrown during delegate execution");
			if constexpr (!std::is_void_v<TReturnType>)
			{
				return TReturnType{};
			}
		}
	}

	/**
	 * @brief 如果绑定则执行委托
	 */
	TReturnType ExecuteIfBound(TArgs... Args) const
	{
		if (IsBound())
		{
			return Execute(Args...);
		}

		if constexpr (!std::is_void_v<TReturnType>)
		{
			return TReturnType{};
		}
	}

	/**
	 * @brief 操作符重载 - 直接调用
	 */
	TReturnType operator()(TArgs... Args) const
	{
		return Execute(Args...);
	}

public:
	// === IDelegateBase接口实现 ===

	FDelegateHandle GetHandle() const override
	{
		return Handle;
	}

	bool IsValid() const override
	{
		return Handle != INVALID_DELEGATE_HANDLE && Function != nullptr;
	}

	void Clear() override
	{
		Function = nullptr;
		Handle = INVALID_DELEGATE_HANDLE;
		NLOG_EVENTS(Trace, "Delegate cleared");
	}

	int32_t GetBindingCount() const override
	{
		return IsValid() ? 1 : 0;
	}

public:
	// === 状态查询 ===

	/**
	 * @brief 检查是否绑定了函数
	 */
	bool IsBound() const
	{
		return IsValid();
	}

	/**
	 * @brief 重置委托
	 */
	void Reset()
	{
		Clear();
	}

private:
	FunctionType Function;  // 绑定的函数
	FDelegateHandle Handle; // 委托句柄
};

// 静态成员初始化
template <typename TReturnType, typename... TArgs>
std::atomic<FDelegateHandle> TDelegate<TReturnType(TArgs...)>::NextHandle{1};

/**
 * @brief 多播委托 - 可以绑定多个函数
 */
template <typename TSignature>
class TMulticastDelegate;

template <typename... TArgs>
class TMulticastDelegate<void(TArgs...)> : public IDelegateBase
{
public:
	using FunctionType = std::function<void(TArgs...)>;

private:
	struct SBinding
	{
		FunctionType Function;
		FDelegateHandle Handle;
		bool bIsOneShot;

		SBinding(FunctionType InFunction, FDelegateHandle InHandle, bool bInOneShot = false)
		    : Function(std::move(InFunction)),
		      Handle(InHandle),
		      bIsOneShot(bInOneShot)
		{}
	};

	static std::atomic<FDelegateHandle> NextHandle;

public:
	// === 构造函数 ===

	TMulticastDelegate() = default;
	~TMulticastDelegate() = default;

	// 禁用拷贝，允许移动
	TMulticastDelegate(const TMulticastDelegate&) = delete;
	TMulticastDelegate& operator=(const TMulticastDelegate&) = delete;
	TMulticastDelegate(TMulticastDelegate&&) = default;
	TMulticastDelegate& operator=(TMulticastDelegate&&) = default;

public:
	// === 绑定函数 ===

	/**
	 * @brief 添加普通函数
	 */
	template <typename TFunc>
	FDelegateHandle AddUFunction(TFunc&& Func, bool bOneShot = false)
	{
		static_assert(std::is_invocable_v<TFunc, TArgs...>, "Function signature mismatch");

		FDelegateHandle Handle = NextHandle.fetch_add(1) + 1;
		Bindings.Add(SBinding(std::forward<TFunc>(Func), Handle, bOneShot));

		NLOG_EVENTS(Trace, "MulticastDelegate added function, handle: {}, bindings: {}", Handle, Bindings.Size());
		return Handle;
	}

	/**
	 * @brief 添加成员函数
	 */
	template <typename TObject, typename TFunc>
	FDelegateHandle AddUObject(TObject* Object, TFunc&& Func, bool bOneShot = false)
	{
		static_assert(std::is_invocable_v<TFunc, TObject*, TArgs...>, "Member function signature mismatch");

		if (!Object)
		{
			NLOG_EVENTS(Error, "Cannot add null object to multicast delegate");
			return INVALID_DELEGATE_HANDLE;
		}

		FDelegateHandle Handle = NextHandle.fetch_add(1) + 1;

		auto Function = [Object, Func](TArgs... Args) { (Object->*Func)(Args...); };

		Bindings.Add(SBinding(std::move(Function), Handle, bOneShot));

		NLOG_EVENTS(
		    Trace, "MulticastDelegate added member function, handle: {}, bindings: {}", Handle, Bindings.Size());
		return Handle;
	}

	/**
	 * @brief 添加Lambda表达式
	 */
	template <typename TLambda>
	FDelegateHandle AddLambda(TLambda&& Lambda, bool bOneShot = false)
	{
		static_assert(std::is_invocable_v<TLambda, TArgs...>, "Lambda signature mismatch");

		FDelegateHandle Handle = NextHandle.fetch_add(1) + 1;
		Bindings.Add(SBinding(std::forward<TLambda>(Lambda), Handle, bOneShot));

		NLOG_EVENTS(Trace, "MulticastDelegate added lambda, handle: {}, bindings: {}", Handle, Bindings.Size());
		return Handle;
	}

	/**
	 * @brief 添加智能指针对象的成员函数
	 */
	template <typename TObject, typename TFunc>
	FDelegateHandle AddSP(TSharedPtr<TObject> Object, TFunc&& Func, bool bOneShot = false)
	{
		static_assert(std::is_invocable_v<TFunc, TObject*, TArgs...>, "Member function signature mismatch");

		if (!Object.IsValid())
		{
			NLOG_EVENTS(Error, "Cannot add invalid shared pointer to multicast delegate");
			return INVALID_DELEGATE_HANDLE;
		}

		FDelegateHandle Handle = NextHandle.fetch_add(1) + 1;

		auto Function = [Object, Func](TArgs... Args) {
			if (Object.IsValid())
			{
				(Object.Get()->*Func)(Args...);
			}
		};

		Bindings.Add(SBinding(std::move(Function), Handle, bOneShot));

		NLOG_EVENTS(Trace,
		            "MulticastDelegate added shared pointer member function, handle: {}, bindings: {}",
		            Handle,
		            Bindings.Size());
		return Handle;
	}

public:
	// === 移除绑定 ===

	/**
	 * @brief 根据句柄移除绑定
	 */
	bool Remove(FDelegateHandle Handle)
	{
		if (Handle == INVALID_DELEGATE_HANDLE)
		{
			return false;
		}

		for (int32_t i = Bindings.Size() - 1; i >= 0; --i)
		{
			if (Bindings[i].Handle == Handle)
			{
				Bindings.RemoveAt(i);
				NLOG_EVENTS(
				    Trace, "MulticastDelegate removed binding, handle: {}, remaining: {}", Handle, Bindings.Size());
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief 移除对象的所有绑定
	 */
	template <typename TObject>
	int32_t RemoveAll(TObject* /*Object*/)
	{
		// 注意：这个实现比较简化，实际实现中需要更复杂的类型匹配
		// 可能需要存储额外的类型信息来精确匹配
		return 0; // 简化实现
	}

	// === 执行委托 ===

	/**
	 * @brief 广播执行所有绑定的函数
	 */
	void Broadcast(TArgs... Args)
	{
		if (Bindings.IsEmpty())
		{
			return;
		}

		// 创建一个副本以避免在执行过程中修改绑定列表导致的问题
		TArray<SBinding, CMemoryManager> BindingsCopy = Bindings;
		TArray<FDelegateHandle, CMemoryManager> OneShotHandles;

		for (const auto& Binding : BindingsCopy)
		{
			try
			{
				Binding.Function(Args...);

				// 收集一次性绑定
				if (Binding.bIsOneShot)
				{
					OneShotHandles.PushBack(Binding.Handle);
				}
			}
			catch (...)
			{
				NLOG_EVENTS(Error, "Exception thrown during multicast delegate execution, handle: {}", Binding.Handle);
			}
		}

		// 移除一次性绑定
		for (FDelegateHandle Handle : OneShotHandles)
		{
			Remove(Handle);
		}

		NLOG_EVENTS(Trace, "MulticastDelegate broadcast completed, {} bindings executed", BindingsCopy.Size());
	}

	/**
	 * @brief 操作符重载 - 直接广播
	 */
	void operator()(TArgs... Args)
	{
		Broadcast(Args...);
	}

public:
	// === IDelegateBase接口实现 ===

	FDelegateHandle GetHandle() const override
	{
		// 多播委托没有单一句柄，返回第一个绑定的句柄
		return Bindings.IsEmpty() ? INVALID_DELEGATE_HANDLE : Bindings[0].Handle;
	}

	bool IsValid() const override
	{
		return !Bindings.IsEmpty();
	}

	void Clear() override
	{
		int32_t Count = Bindings.Size();
		Bindings.Empty();
		NLOG_EVENTS(Trace, "MulticastDelegate cleared {} bindings", Count);
	}

	int32_t GetBindingCount() const override
	{
		return Bindings.Size();
	}

public:
	// === 状态查询 ===

	/**
	 * @brief 检查是否有绑定
	 */
	bool IsBound() const
	{
		return !Bindings.IsEmpty();
	}

	/**
	 * @brief 检查特定句柄是否绑定
	 */
	bool IsBound(FDelegateHandle Handle) const
	{
		for (const auto& Binding : Bindings)
		{
			if (Binding.Handle == Handle)
			{
				return true;
			}
		}
		return false;
	}

private:
	TArray<SBinding, CMemoryManager> Bindings; // 绑定列表
};

// 静态成员初始化
template <typename... TArgs>
std::atomic<FDelegateHandle> TMulticastDelegate<void(TArgs...)>::NextHandle{1};

// === 委托类型别名 ===

// 单播委托
template <typename TSignature>
using FDelegate = TDelegate<TSignature>;

// 多播委托
template <typename TSignature>
using FMulticastDelegate = TMulticastDelegate<TSignature>;

// 常用委托类型
using FSimpleDelegate = TDelegate<void()>;
using FSimpleMulticastDelegate = TMulticastDelegate<void()>;

// === 委托宏定义 ===

/**
 * @brief 声明委托类型的宏
 */
#define DECLARE_DELEGATE(DelegateName, ...) using DelegateName = NLib::TDelegate<void(__VA_ARGS__)>;

#define DECLARE_DELEGATE_RetVal(ReturnType, DelegateName, ...)                                                         \
	using DelegateName = NLib::TDelegate<ReturnType(__VA_ARGS__)>;

#define DECLARE_MULTICAST_DELEGATE(DelegateName, ...) using DelegateName = NLib::TMulticastDelegate<void(__VA_ARGS__)>;

/**
 * @brief 委托绑定辅助宏
 */
#define BIND_UFUNCTION(Delegate, Function) (Delegate).BindUFunction(Function)

#define BIND_UOBJECT(Delegate, Object, Function) (Delegate).BindUObject(Object, Function)

#define BIND_LAMBDA(Delegate, Lambda) (Delegate).BindLambda(Lambda)

} // namespace NLib
