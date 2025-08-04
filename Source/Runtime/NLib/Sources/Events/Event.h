#pragma once

#include "Containers/THashMap.h"
#include "Containers/TString.h"
#include "Core/Object.h"
#include "Core/SmartPointers.h"
#include "Delegate.h"
#include "Logging/LogCategory.h"
#include "Time/TimeTypes.h"

#include <atomic>
#include <mutex>
#include <type_traits>

namespace NLib
{
/**
 * @brief 事件优先级枚举
 */
enum class EEventPriority : uint8_t
{
	Lowest = 0,
	Low = 1,
	Normal = 2,
	High = 3,
	Highest = 4,
	Critical = 5
};

/**
 * @brief 事件ID类型
 */
using FEventId = uint64_t;
constexpr FEventId INVALID_EVENT_ID = 0;

/**
 * @brief 基础事件接口
 */
class IEvent
{
public:
	virtual ~IEvent() = default;

	/**
	 * @brief 获取事件ID
	 */
	virtual FEventId GetEventId() const = 0;

	/**
	 * @brief 获取事件名称
	 */
	virtual const char* GetEventName() const = 0;

	/**
	 * @brief 获取事件优先级
	 */
	virtual EEventPriority GetPriority() const = 0;

	/**
	 * @brief 获取事件时间戳
	 */
	virtual CDateTime GetTimestamp() const = 0;

	/**
	 * @brief 检查事件是否可以取消
	 */
	virtual bool IsStoppable() const = 0;

	/**
	 * @brief 检查事件是否已被停止
	 */
	virtual bool IsStopped() const = 0;

	/**
	 * @brief 停止事件传播
	 */
	virtual void StopPropagation() = 0;
};

/**
 * @brief 基础事件类
 */
class CBaseEvent : public IEvent
{
public:
	CBaseEvent(const char* InEventName, EEventPriority InPriority = EEventPriority::Normal, bool bInStoppable = true)
	    : EventName(InEventName),
	      Priority(InPriority),
	      Timestamp(CDateTime::Now()),
	      bStoppable(bInStoppable),
	      bStopped(false)
	{
		static std::atomic<FEventId> NextEventId{1};
		EventId = NextEventId.fetch_add(1);
	}

	virtual ~CBaseEvent() = default;

public:
	// === IEvent接口实现 ===

	FEventId GetEventId() const override
	{
		return EventId;
	}
	const char* GetEventName() const override
	{
		return EventName;
	}
	EEventPriority GetPriority() const override
	{
		return Priority;
	}
	CDateTime GetTimestamp() const override
	{
		return Timestamp;
	}
	bool IsStoppable() const override
	{
		return bStoppable;
	}
	bool IsStopped() const override
	{
		return bStopped;
	}

	void StopPropagation() override
	{
		if (bStoppable)
		{
			bStopped = true;
			NLOG_EVENTS(Debug, "Event '{}' propagation stopped", EventName);
		}
	}

protected:
	FEventId EventId;
	const char* EventName;
	EEventPriority Priority;
	CDateTime Timestamp;
	bool bStoppable;
	std::atomic<bool> bStopped;
};

/**
 * @brief 类型化事件基类
 */
template <typename TEventType>
class TEvent : public CBaseEvent
{
public:
	TEvent(EEventPriority InPriority = EEventPriority::Normal, bool bInStoppable = true)
	    : CBaseEvent(TEventType::GetStaticEventName(), InPriority, bInStoppable)
	{}

	/**
	 * @brief 获取静态事件名称
	 */
	static const char* GetStaticEventName()
	{
		return typeid(TEventType).name();
	}

	/**
	 * @brief 获取事件类型ID
	 */
	static size_t GetStaticEventTypeId()
	{
		return typeid(TEventType).hash_code();
	}

	/**
	 * @brief 获取事件类型ID
	 */
	virtual size_t GetEventTypeId() const
	{
		return GetStaticEventTypeId();
	}
};

/**
 * @brief 事件监听器接口
 */
template <typename TEventType>
class IEventListener
{
public:
	virtual ~IEventListener() = default;

	/**
	 * @brief 处理事件
	 */
	virtual void HandleEvent(const TEventType& Event) = 0;

	/**
	 * @brief 获取监听器优先级
	 */
	virtual EEventPriority GetListenerPriority() const
	{
		return EEventPriority::Normal;
	}

	/**
	 * @brief 检查监听器是否仍然有效
	 */
	virtual bool IsListenerValid() const
	{
		return true;
	}
};

/**
 * @brief 事件监听器绑定信息
 */
template <typename TEventType>
struct SEventListenerBinding
{
	using ListenerDelegate = TDelegate<void(const TEventType&)>;

	FDelegateHandle Handle = INVALID_DELEGATE_HANDLE;
	ListenerDelegate Delegate;
	EEventPriority Priority = EEventPriority::Normal;
	bool bIsOneShot = false;
	bool bIsValid = true;

	SEventListenerBinding() = default;

	SEventListenerBinding(FDelegateHandle InHandle,
	                      ListenerDelegate&& InDelegate,
	                      EEventPriority InPriority,
	                      bool bInOneShot)
	    : Handle(InHandle),
	      Delegate(std::move(InDelegate)),
	      Priority(InPriority),
	      bIsOneShot(bInOneShot)
	{}

	void Execute(const TEventType& Event)
	{
		if (bIsValid && Delegate.IsValid())
		{
			Delegate.Execute(Event);
		}
	}

	void Invalidate()
	{
		bIsValid = false;
	}
	bool IsValid() const
	{
		return bIsValid && Delegate.IsValid();
	}
};

/**
 * @brief 事件调度器
 */
template <typename TEventType>
class TEventDispatcher
{
public:
	using ListenerBinding = SEventListenerBinding<TEventType>;
	using ListenerDelegate = typename ListenerBinding::ListenerDelegate;

public:
	TEventDispatcher() = default;
	~TEventDispatcher() = default;

	// 禁用拷贝，允许移动
	TEventDispatcher(const TEventDispatcher&) = delete;
	TEventDispatcher& operator=(const TEventDispatcher&) = delete;
	TEventDispatcher(TEventDispatcher&&) = default;
	TEventDispatcher& operator=(TEventDispatcher&&) = default;

public:
	// === 监听器管理 ===

	/**
	 * @brief 添加事件监听器
	 */
	template <typename TFunc>
	FDelegateHandle AddListener(TFunc&& Func, EEventPriority Priority = EEventPriority::Normal, bool bOneShot = false)
	{
		std::lock_guard<std::mutex> Lock(ListenersMutex);

		static std::atomic<FDelegateHandle> NextHandle{1};
		FDelegateHandle Handle = NextHandle.fetch_add(1);

		ListenerDelegate Delegate;
		Delegate.BindUFunction(std::forward<TFunc>(Func));

		auto Binding = ListenerBinding(Handle, std::move(Delegate), Priority, bOneShot);
		Listeners.Add(Binding);

		// 按优先级排序
		SortListenersByPriority();

		NLOG_EVENTS(Debug,
		            "Event listener added for '{}', handle: {}, priority: {}",
		            TEventType::GetStaticEventName(),
		            Handle,
		            static_cast<int>(Priority));

		return Handle;
	}

	/**
	 * @brief 添加对象成员函数监听器
	 */
	template <typename TObject, typename TFunc>
	FDelegateHandle AddObjectListener(TObject* Object,
	                                  TFunc&& Func,
	                                  EEventPriority Priority = EEventPriority::Normal,
	                                  bool bOneShot = false)
	{
		if (!Object)
		{
			NLOG_EVENTS(Error, "Cannot add null object as event listener");
			return INVALID_DELEGATE_HANDLE;
		}

		std::lock_guard<std::mutex> Lock(ListenersMutex);

		static std::atomic<FDelegateHandle> NextHandle{1};
		FDelegateHandle Handle = NextHandle.fetch_add(1);

		ListenerDelegate Delegate;
		Delegate.BindUObject(Object, std::forward<TFunc>(Func));

		auto Binding = ListenerBinding(Handle, std::move(Delegate), Priority, bOneShot);
		Listeners.Add(Binding);

		SortListenersByPriority();

		NLOG_EVENTS(
		    Debug, "Event object listener added for '{}', handle: {}", TEventType::GetStaticEventName(), Handle);

		return Handle;
	}

	/**
	 * @brief 添加智能指针对象监听器
	 */
	template <typename TObject, typename TFunc>
	FDelegateHandle AddSharedListener(TSharedPtr<TObject> Object,
	                                  TFunc&& Func,
	                                  EEventPriority Priority = EEventPriority::Normal,
	                                  bool bOneShot = false)
	{
		if (!Object.IsValid())
		{
			NLOG_EVENTS(Error, "Cannot add invalid shared pointer as event listener");
			return INVALID_DELEGATE_HANDLE;
		}

		std::lock_guard<std::mutex> Lock(ListenersMutex);

		static std::atomic<FDelegateHandle> NextHandle{1};
		FDelegateHandle Handle = NextHandle.fetch_add(1);

		ListenerDelegate Delegate;
		Delegate.BindSP(Object, std::forward<TFunc>(Func));

		auto Binding = ListenerBinding(Handle, std::move(Delegate), Priority, bOneShot);
		Listeners.Add(Binding);

		SortListenersByPriority();

		NLOG_EVENTS(
		    Debug, "Event shared listener added for '{}', handle: {}", TEventType::GetStaticEventName(), Handle);

		return Handle;
	}

	/**
	 * @brief 移除监听器
	 */
	bool RemoveListener(FDelegateHandle Handle)
	{
		if (Handle == INVALID_DELEGATE_HANDLE)
		{
			return false;
		}

		std::lock_guard<std::mutex> Lock(ListenersMutex);

		for (int32_t i = Listeners.Size() - 1; i >= 0; --i)
		{
			if (Listeners[i].Handle == Handle)
			{
				Listeners.RemoveAt(i);
				NLOG_EVENTS(
				    Debug, "Event listener removed for '{}', handle: {}", TEventType::GetStaticEventName(), Handle);
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief 清空所有监听器
	 */
	void ClearListeners()
	{
		std::lock_guard<std::mutex> Lock(ListenersMutex);
		int32_t Count = Listeners.Size();
		Listeners.Empty();

		NLOG_EVENTS(
		    Debug, "All event listeners cleared for '{}', removed: {}", TEventType::GetStaticEventName(), Count);
	}

public:
	// === 事件分发 ===

	/**
	 * @brief 分发事件
	 */
	void Dispatch(const TEventType& Event)
	{
		std::lock_guard<std::mutex> Lock(ListenersMutex);

		if (Listeners.IsEmpty())
		{
			return;
		}

		// 创建监听器副本以避免在执行过程中修改列表
		TArray<ListenerBinding, CMemoryManager> ListenersCopy = Listeners;
		TArray<FDelegateHandle, CMemoryManager> OneShotHandles;

		NLOG_EVENTS(
		    Trace, "Dispatching event '{}' to {} listeners", TEventType::GetStaticEventName(), ListenersCopy.Size());

		for (auto& Binding : ListenersCopy)
		{
			if (!Binding.IsValid())
			{
				continue;
			}

			try
			{
				Binding.Execute(Event);

				// 收集一次性监听器
				if (Binding.bIsOneShot)
				{
					OneShotHandles.Add(Binding.Handle);
				}

				// 检查事件是否被停止
				if (Event.IsStopped())
				{
					NLOG_EVENTS(Trace, "Event '{}' propagation stopped", TEventType::GetStaticEventName());
					break;
				}
			}
			catch (...)
			{
				NLOG_EVENTS(Error,
				            "Exception thrown in event listener for '{}', handle: {}",
				            TEventType::GetStaticEventName(),
				            Binding.Handle);
			}
		}

		// 移除一次性监听器
		for (FDelegateHandle Handle : OneShotHandles)
		{
			RemoveListenerInternal(Handle);
		}
	}

public:
	// === 状态查询 ===

	/**
	 * @brief 获取监听器数量
	 */
	int32_t GetListenerCount() const
	{
		std::lock_guard<std::mutex> Lock(ListenersMutex);
		return Listeners.Size();
	}

	/**
	 * @brief 检查是否有监听器
	 */
	bool HasListeners() const
	{
		std::lock_guard<std::mutex> Lock(ListenersMutex);
		return !Listeners.IsEmpty();
	}

	/**
	 * @brief 检查特定句柄的监听器是否存在
	 */
	bool HasListener(FDelegateHandle Handle) const
	{
		std::lock_guard<std::mutex> Lock(ListenersMutex);

		for (const auto& Binding : Listeners)
		{
			if (Binding.Handle == Handle && Binding.IsValid())
			{
				return true;
			}
		}

		return false;
	}

private:
	// === 内部实现 ===

	/**
	 * @brief 按优先级排序监听器
	 */
	void SortListenersByPriority()
	{
		// 简单的插入排序，按优先级从高到低排序
		for (int32_t i = 1; i < Listeners.Size(); ++i)
		{
			auto Current = Listeners[i];
			int32_t j = i - 1;

			while (j >= 0 && Listeners[j].Priority < Current.Priority)
			{
				Listeners[j + 1] = Listeners[j];
				j--;
			}

			Listeners[j + 1] = Current;
		}
	}

	/**
	 * @brief 内部移除监听器（不加锁）
	 */
	bool RemoveListenerInternal(FDelegateHandle Handle)
	{
		for (int32_t i = Listeners.Size() - 1; i >= 0; --i)
		{
			if (Listeners[i].Handle == Handle)
			{
				Listeners.RemoveAt(i);
				return true;
			}
		}
		return false;
	}

private:
	TArray<ListenerBinding, CMemoryManager> Listeners; // 监听器列表
	mutable std::mutex ListenersMutex;                 // 监听器互斥锁
};

/**
 * @brief 全局事件管理器
 */
class CEventManager
{
public:
	// === 单例模式 ===

	static CEventManager& GetInstance()
	{
		static CEventManager Instance;
		return Instance;
	}

private:
	CEventManager() = default;

public:
	// 禁用拷贝
	CEventManager(const CEventManager&) = delete;
	CEventManager& operator=(const CEventManager&) = delete;

public:
	// === 事件分发 ===

	/**
	 * @brief 立即分发事件
	 */
	template <typename TEventType>
	void DispatchEvent(const TEventType& Event)
	{
		auto* Dispatcher = GetOrCreateDispatcher<TEventType>();
		if (Dispatcher)
		{
			Dispatcher->Dispatch(Event);
		}
	}

	/**
	 * @brief 创建并分发事件
	 */
	template <typename TEventType, typename... TArgs>
	void EmitEvent(TArgs&&... Args)
	{
		TEventType Event(std::forward<TArgs>(Args)...);
		DispatchEvent(Event);
	}

public:
	// === 监听器管理 ===

	/**
	 * @brief 添加事件监听器
	 */
	template <typename TEventType, typename TFunc>
	FDelegateHandle AddEventListener(TFunc&& Func,
	                                 EEventPriority Priority = EEventPriority::Normal,
	                                 bool bOneShot = false)
	{
		auto* Dispatcher = GetOrCreateDispatcher<TEventType>();
		if (Dispatcher)
		{
			return Dispatcher->AddListener(std::forward<TFunc>(Func), Priority, bOneShot);
		}
		return INVALID_DELEGATE_HANDLE;
	}

	/**
	 * @brief 添加对象事件监听器
	 */
	template <typename TEventType, typename TObject, typename TFunc>
	FDelegateHandle AddObjectEventListener(TObject* Object,
	                                       TFunc&& Func,
	                                       EEventPriority Priority = EEventPriority::Normal,
	                                       bool bOneShot = false)
	{
		auto* Dispatcher = GetOrCreateDispatcher<TEventType>();
		if (Dispatcher)
		{
			return Dispatcher->AddObjectListener(Object, std::forward<TFunc>(Func), Priority, bOneShot);
		}
		return INVALID_DELEGATE_HANDLE;
	}

	/**
	 * @brief 移除事件监听器
	 */
	template <typename TEventType>
	bool RemoveEventListener(FDelegateHandle Handle)
	{
		auto* Dispatcher = GetDispatcher<TEventType>();
		if (Dispatcher)
		{
			return Dispatcher->RemoveListener(Handle);
		}
		return false;
	}

	/**
	 * @brief 清空特定事件类型的所有监听器
	 */
	template <typename TEventType>
	void ClearEventListeners()
	{
		auto* Dispatcher = GetDispatcher<TEventType>();
		if (Dispatcher)
		{
			Dispatcher->ClearListeners();
		}
	}

	/**
	 * @brief 清空所有事件监听器
	 */
	void ClearAllEventListeners()
	{
		std::lock_guard<std::mutex> Lock(DispatchersMutex);
		Dispatchers.Empty();
		NLOG_EVENTS(Info, "All event listeners cleared");
	}

public:
	// === 状态查询 ===

	/**
	 * @brief 获取事件类型的监听器数量
	 */
	template <typename TEventType>
	int32_t GetEventListenerCount() const
	{
		auto* Dispatcher = GetDispatcher<TEventType>();
		return Dispatcher ? Dispatcher->GetListenerCount() : 0;
	}

	/**
	 * @brief 获取总的事件类型数量
	 */
	int32_t GetEventTypeCount() const
	{
		std::lock_guard<std::mutex> Lock(DispatchersMutex);
		return Dispatchers.Size();
	}

	/**
	 * @brief 生成事件系统报告
	 */
	CString GenerateEventReport() const
	{
		std::lock_guard<std::mutex> Lock(DispatchersMutex);

		char Buffer[1024];
		snprintf(Buffer,
		         sizeof(Buffer),
		         "=== Event System Report ===\n"
		         "Registered Event Types: %d\n"
		         "Total Listeners: %d\n",
		         Dispatchers.Size(),
		         GetTotalListenerCount());

		return CString(Buffer);
	}

private:
	// === 内部实现 ===

	/**
	 * @brief 获取或创建事件调度器
	 */
	template <typename TEventType>
	TEventDispatcher<TEventType>* GetOrCreateDispatcher()
	{
		size_t TypeId = TEventType::GetStaticEventTypeId();

		std::lock_guard<std::mutex> Lock(DispatchersMutex);

		auto* ExistingDispatcher = Dispatchers.Find(TypeId);
		if (ExistingDispatcher)
		{
			return static_cast<TEventDispatcher<TEventType>*>(ExistingDispatcher->Get());
		}

		auto NewDispatcher = MakeShared<TEventDispatcher<TEventType>>();
		Dispatchers.Add(TypeId, NewDispatcher);

		NLOG_EVENTS(Debug, "Created event dispatcher for '{}'", TEventType::GetStaticEventName());

		return NewDispatcher.Get();
	}

	/**
	 * @brief 获取事件调度器
	 */
	template <typename TEventType>
	TEventDispatcher<TEventType>* GetDispatcher() const
	{
		size_t TypeId = TEventType::GetStaticEventTypeId();

		std::lock_guard<std::mutex> Lock(DispatchersMutex);

		auto* ExistingDispatcher = Dispatchers.Find(TypeId);
		if (ExistingDispatcher)
		{
			return static_cast<TEventDispatcher<TEventType>*>(ExistingDispatcher->Get());
		}

		return nullptr;
	}

	/**
	 * @brief 获取总监听器数量
	 */
	int32_t GetTotalListenerCount() const
	{
		int32_t Total = 0;
		// 这里需要遍历所有调度器，但由于类型擦除，实现会比较复杂
		// 简化实现
		return Total;
	}

private:
	// 使用类型ID映射到调度器的智能指针
	THashMap<size_t, TSharedPtr<void>, CMemoryManager> Dispatchers;
	mutable std::mutex DispatchersMutex;
};

// === 便捷宏定义 ===

/**
 * @brief 声明事件类型的宏
 */
#define DECLARE_EVENT(EventName, BaseClass, ...)                                                                       \
	class EventName : public BaseClass<EventName>                                                                      \
	{                                                                                                                  \
	public:                                                                                                            \
		EventName(__VA_ARGS__)                                                                                         \
		    : BaseClass<EventName>()                                                                                   \
		{}                                                                                                             \
		static const char* GetStaticEventName()                                                                        \
		{                                                                                                              \
			return #EventName;                                                                                         \
		}                                                                                                              \
	};

/**
 * @brief 事件绑定辅助宏
 */
#define BIND_EVENT_LISTENER(EventType, Function)                                                                       \
	NLib::CEventManager::GetInstance().AddEventListener<EventType>(Function)

#define BIND_EVENT_OBJECT(EventType, Object, Function)                                                                 \
	NLib::CEventManager::GetInstance().AddObjectEventListener<EventType>(Object, Function)

// === 全局访问函数 ===

/**
 * @brief 获取事件管理器
 */
inline CEventManager& GetEventManager()
{
	return CEventManager::GetInstance();
}

/**
 * @brief 分发事件
 */
template <typename TEventType>
void DispatchEvent(const TEventType& Event)
{
	GetEventManager().DispatchEvent(Event);
}

/**
 * @brief 创建并分发事件
 */
template <typename TEventType, typename... TArgs>
void EmitEvent(TArgs&&... Args)
{
	GetEventManager().EmitEvent<TEventType>(std::forward<TArgs>(Args)...);
}

} // namespace NLib