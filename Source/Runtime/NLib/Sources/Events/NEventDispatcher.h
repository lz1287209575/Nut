#pragma once

#include "Events/NEvent.h"
#include "Core/CObject.h"
#include "Containers/CString.h"
#include "Containers/CArray.h"
#include "Containers/CHashMap.h"
#include "Threading/CThread.h"
#include "Async/NAsyncTask.h"
#include "Delegates/CDelegate.h"

namespace NLib
{

/**
 * @brief 事件派发模式
 */
enum class EEventDispatchMode : uint32_t
{
    Immediate,      // 立即同步派发
    Deferred,       // 延迟到下一个更新周期
    Async,          // 异步派发
    Queued          // 队列模式，批量处理
};

/**
 * @brief 事件派发器 - 核心事件分发系统
 */
class LIBNLIB_API NEventDispatcher : public CObject
{
    NCLASS()
class NEventDispatcher : public NObject
{
    GENERATED_BODY()

public:
    NEventDispatcher();
    virtual ~NEventDispatcher();
    
    // 单例访问
    static NEventDispatcher& GetGlobalDispatcher();
    
    // 初始化和清理
    void Initialize();
    void Shutdown();
    void Update(); // 在主线程中调用，处理延迟事件
    
    // 事件发布
    void DispatchEvent(TSharedPtr<NEvent> Event, EEventDispatchMode Mode = EEventDispatchMode::Immediate);
    void DispatchEventDeferred(TSharedPtr<NEvent> Event);
    void DispatchEventAsync(TSharedPtr<NEvent> Event);
    
    // 模板便捷方法
    template<typename TEvent, typename... Args>
    void DispatchEvent(EEventDispatchMode Mode, Args&&... Arguments);
    
    template<typename TEvent, typename... Args>
    void DispatchEvent(Args&&... Arguments);
    
    template<typename TEvent, typename... Args>
    void DispatchEventDeferred(Args&&... Arguments);
    
    template<typename TEvent, typename... Args>
    void DispatchEventAsync(Args&&... Arguments);
    
    // 批量事件处理
    void DispatchEvents(const CArray<TSharedPtr<NEvent>>& Events, EEventDispatchMode Mode = EEventDispatchMode::Immediate);
    void QueueEvent(TSharedPtr<NEvent> Event);
    void ProcessQueuedEvents();
    void ClearEventQueue();
    
    // 处理器管理
    void RegisterHandler(TSharedPtr<IEventHandler> Handler);
    void RegisterHandler(const CString& EventType, TSharedPtr<IEventHandler> Handler);
    void UnregisterHandler(TSharedPtr<IEventHandler> Handler);
    void UnregisterHandler(const CString& EventType, TSharedPtr<IEventHandler> Handler);
    void UnregisterAllHandlers();
    void UnregisterAllHandlers(const CString& EventType);
    
    // 模板处理器注册
    template<typename TEvent>
    void RegisterHandler(typename CEventHandler<TEvent>::HandlerFunction Function, int32_t Priority = 0);
    
    template<typename TEvent>
    void RegisterHandler(TSharedPtr<CEventHandler<TEvent>> Handler);
    
    // 处理器查询
    bool HasHandler(const CString& EventType) const;
    int32_t GetHandlerCount(const CString& EventType) const;
    int32_t GetTotalHandlerCount() const;
    CArray<TSharedPtr<IEventHandler>> GetHandlers(const CString& EventType) const;
    
    // 过滤器管理
    void AddFilter(TSharedPtr<NEventFilter> Filter);
    void RemoveFilter(TSharedPtr<NEventFilter> Filter);
    void ClearFilters();
    bool PassesFilters(TSharedPtr<NEvent> Event) const;
    
    // 事件拦截器
    using EventInterceptor = NFunction<bool(TSharedPtr<NEvent>)>; // 返回false阻止事件继续传播
    void AddInterceptor(EventInterceptor Interceptor, int32_t Priority = 0);
    void RemoveInterceptor(EventInterceptor Interceptor);
    void ClearInterceptors();
    
    // 配置选项
    void SetMaxQueueSize(int32_t MaxSize);
    int32_t GetMaxQueueSize() const;
    
    void SetAsyncThreadCount(int32_t ThreadCount);
    int32_t GetAsyncThreadCount() const;
    
    void SetBatchProcessingEnabled(bool bEnabled);
    bool IsBatchProcessingEnabled() const;
    
    void SetStatisticsEnabled(bool bEnabled);
    bool IsStatisticsEnabled() const;
    
    // 暂停和恢复
    void Pause();
    void Resume();
    bool IsPaused() const;
    
    void PauseEventType(const CString& EventType);
    void ResumeEventType(const CString& EventType);
    bool IsEventTypePaused(const CString& EventType) const;
    
    // 调试和监控
    const NEventStatistics& GetStatistics() const;
    void ResetStatistics();
    void DumpStatistics() const;
    
    CArray<CString> GetRegisteredEventTypes() const;
    CString GetDispatcherReport() const;
    
    // 事件记录
    void SetEventLoggingEnabled(bool bEnabled);
    bool IsEventLoggingEnabled() const;
    void SetMaxLoggedEvents(int32_t MaxCount);
    CArray<TSharedPtr<NEvent>> GetRecentEvents() const;
    
    // 错误处理
    using ErrorHandler = NFunction<void(TSharedPtr<NEvent>, const CString&)>;
    void SetErrorHandler(ErrorHandler Handler);
    void ClearErrorHandler();

private:
    struct HandlerEntry
    {
        TSharedPtr<IEventHandler> Handler;
        int32_t Priority;
        bool bEnabled;
        
        HandlerEntry(TSharedPtr<IEventHandler> InHandler, int32_t InPriority = 0)
            : Handler(InHandler), Priority(InPriority), bEnabled(true)
        {}
        
        bool operator<(const HandlerEntry& Other) const
        {
            return Priority > Other.Priority; // 高优先级排在前面
        }
    };
    
    struct InterceptorEntry
    {
        EventInterceptor Interceptor;
        int32_t Priority;
        
        InterceptorEntry(EventInterceptor InInterceptor, int32_t InPriority = 0)
            : Interceptor(InInterceptor), Priority(InPriority)
        {}
        
        bool operator<(const InterceptorEntry& Other) const
        {
            return Priority > Other.Priority;
        }
    };
    
    // 处理器存储
    CHashMap<CString, CArray<HandlerEntry>> HandlerMap;
    CArray<HandlerEntry> GlobalHandlers; // 处理所有事件的处理器
    
    // 过滤器和拦截器
    CArray<TSharedPtr<NEventFilter>> Filters;
    CArray<InterceptorEntry> Interceptors;
    
    // 事件队列
    CArray<TSharedPtr<NEvent>> EventQueue;
    CArray<TSharedPtr<NEvent>> DeferredEvents;
    
    // 异步处理
    TSharedPtr<class NAsyncTaskScheduler> AsyncScheduler;
    
    // 配置
    int32_t MaxQueueSize;
    int32_t AsyncThreadCount;
    bool bBatchProcessingEnabled;
    bool bStatisticsEnabled;
    bool bEventLoggingEnabled;
    int32_t MaxLoggedEvents;
    
    // 状态控制
    bool bInitialized;
    bool bShuttingDown;
    bool bPaused;
    CHashMap<CString, bool> PausedEventTypes;
    
    // 统计和日志
    NEventStatistics Statistics;
    CArray<TSharedPtr<NEvent>> RecentEvents;
    
    // 错误处理
    ErrorHandler CurrentErrorHandler;
    
    // 线程安全
    mutable NMutex HandlerMutex;
    mutable NMutex EventQueueMutex;
    mutable NMutex StatisticsMutex;
    
    // 内部方法
    void DispatchEventInternal(TSharedPtr<NEvent> Event);
    void ProcessEventImmediate(TSharedPtr<NEvent> Event);
    void ProcessEventAsync(TSharedPtr<NEvent> Event);
    
    bool RunInterceptors(TSharedPtr<NEvent> Event);
    CArray<HandlerEntry> GetEventHandlers(TSharedPtr<NEvent> Event) const;
    void ExecuteHandler(const HandlerEntry& Entry, TSharedPtr<NEvent> Event);
    
    void UpdateStatistics(TSharedPtr<NEvent> Event, double ProcessingTime);
    void LogEvent(TSharedPtr<NEvent> Event);
    void HandleError(TSharedPtr<NEvent> Event, const CString& ErrorMessage);
    
    void SortHandlers();
    void SortInterceptors();
    
    // 禁止复制
    NEventDispatcher(const NEventDispatcher&) = delete;
    NEventDispatcher& operator=(const NEventDispatcher&) = delete;
};

/**
 * @brief 作用域事件处理器 - RAII方式管理事件处理器生命周期
 */
template<typename TEvent>
class NScopedEventHandler : public NObject
{
    NCLASS()
class NScopedEventHandler : public NObject
{
    GENERATED_BODY()

public:
    using HandlerFunction = typename CEventHandler<TEvent>::HandlerFunction;
    
    NScopedEventHandler(HandlerFunction Function, int32_t Priority = 0, NEventDispatcher* Dispatcher = nullptr);
    virtual ~NScopedEventHandler();
    
    void SetEnabled(bool bEnabled);
    bool IsEnabled() const;

private:
    TSharedPtr<CEventHandler<TEvent>> Handler;
    NEventDispatcher* TargetDispatcher;
};

/**
 * @brief 事件总线 - 高级事件管理接口
 */
class LIBNLIB_API NEventBus : public CObject
{
    NCLASS()
class NEventBus : public NObject
{
    GENERATED_BODY()

public:
    NEventBus();
    explicit NEventBus(TSharedPtr<NEventDispatcher> InDispatcher);
    virtual ~NEventBus();
    
    // 事件发布
    template<typename TEvent, typename... Args>
    void Publish(Args&&... Arguments);
    
    void Publish(TSharedPtr<NEvent> Event);
    void PublishDeferred(TSharedPtr<NEvent> Event);
    void PublishAsync(TSharedPtr<NEvent> Event);
    
    // 事件订阅
    template<typename TEvent>
    void Subscribe(typename CEventHandler<TEvent>::HandlerFunction Function, int32_t Priority = 0);
    
    template<typename TEvent>
    NScopedEventHandler<TEvent> CreateScopedHandler(typename CEventHandler<TEvent>::HandlerFunction Function, int32_t Priority = 0);
    
    // 取消订阅
    template<typename TEvent>
    void Unsubscribe();
    
    void UnsubscribeAll();
    
    // 总线控制
    void Pause();
    void Resume();
    bool IsPaused() const;
    
    // 获取底层派发器
    NEventDispatcher* GetDispatcher() const;

private:
    TSharedPtr<NEventDispatcher> Dispatcher;
    CArray<TSharedPtr<IEventHandler>> RegisteredHandlers;
};

// === 模板实现 ===

template<typename TEvent, typename... Args>
void NEventDispatcher::DispatchEvent(EEventDispatchMode Mode, Args&&... Arguments)
{
    auto Event = NewNObject<TEvent>(NLib::Forward<Args>(Arguments)...);
    DispatchEvent(Event, Mode);
}

template<typename TEvent, typename... Args>
void NEventDispatcher::DispatchEvent(Args&&... Arguments)
{
    DispatchEvent<TEvent>(EEventDispatchMode::Immediate, NLib::Forward<Args>(Arguments)...);
}

template<typename TEvent, typename... Args>
void NEventDispatcher::DispatchEventDeferred(Args&&... Arguments)
{
    DispatchEvent<TEvent>(EEventDispatchMode::Deferred, NLib::Forward<Args>(Arguments)...);
}

template<typename TEvent, typename... Args>
void NEventDispatcher::DispatchEventAsync(Args&&... Arguments)
{
    DispatchEvent<TEvent>(EEventDispatchMode::Async, NLib::Forward<Args>(Arguments)...);
}

template<typename TEvent>
void NEventDispatcher::RegisterHandler(typename CEventHandler<TEvent>::HandlerFunction Function, int32_t Priority)
{
    auto Handler = NewNObject<CEventHandler<TEvent>>(Function, Priority);
    RegisterHandler(Handler);
}

template<typename TEvent>
void NEventDispatcher::RegisterHandler(TSharedPtr<CEventHandler<TEvent>> Handler)
{
    RegisterHandler(StaticCast<IEventHandler>(Handler));
}

template<typename TEvent>
NScopedEventHandler<TEvent>::NScopedEventHandler(HandlerFunction Function, int32_t Priority, NEventDispatcher* Dispatcher)
    : Handler(NewNObject<CEventHandler<TEvent>>(Function, Priority))
    , TargetDispatcher(Dispatcher ? Dispatcher : &NEventDispatcher::GetGlobalDispatcher())
{
    TargetDispatcher->RegisterHandler(Handler);
}

template<typename TEvent>
NScopedEventHandler<TEvent>::~NScopedEventHandler()
{
    if (TargetDispatcher && Handler)
    {
        TargetDispatcher->UnregisterHandler(Handler);
    }
}

template<typename TEvent>
void NScopedEventHandler<TEvent>::SetEnabled(bool bEnabled)
{
    if (Handler)
    {
        Handler->SetEnabled(bEnabled);
    }
}

template<typename TEvent>
bool NScopedEventHandler<TEvent>::IsEnabled() const
{
    return Handler ? Handler->IsEnabled() : false;
}

template<typename TEvent, typename... Args>
void NEventBus::Publish(Args&&... Arguments)
{
    auto Event = NewNObject<TEvent>(NLib::Forward<Args>(Arguments)...);
    Publish(Event);
}

template<typename TEvent>
void NEventBus::Subscribe(typename CEventHandler<TEvent>::HandlerFunction Function, int32_t Priority)
{
    auto Handler = NewNObject<CEventHandler<TEvent>>(Function, Priority);
    Dispatcher->RegisterHandler(Handler);
    RegisteredHandlers.PushBack(Handler);
}

template<typename TEvent>
NScopedEventHandler<TEvent> NEventBus::CreateScopedHandler(typename CEventHandler<TEvent>::HandlerFunction Function, int32_t Priority)
{
    return NScopedEventHandler<TEvent>(Function, Priority, Dispatcher.Get());
}

template<typename TEvent>
void NEventBus::Unsubscribe()
{
    CString EventTypeName = typeid(TEvent).name();
    Dispatcher->UnregisterAllHandlers(EventTypeName);
    
    // 从已注册处理器列表中移除
    for (auto It = RegisteredHandlers.Begin(); It != RegisteredHandlers.End();)
    {
        if ((*It)->CanHandle(EventTypeName))
        {
            It = RegisteredHandlers.Erase(It);
        }
        else
        {
            ++It;
        }
    }
}

/**
 * @brief 事件系统便捷宏
 */
#define DISPATCH_EVENT(EventType, ...) \
    NLib::NEventDispatcher::GetGlobalDispatcher().DispatchEvent<EventType>(__VA_ARGS__)

#define DISPATCH_EVENT_DEFERRED(EventType, ...) \
    NLib::NEventDispatcher::GetGlobalDispatcher().DispatchEventDeferred<EventType>(__VA_ARGS__)

#define DISPATCH_EVENT_ASYNC(EventType, ...) \
    NLib::NEventDispatcher::GetGlobalDispatcher().DispatchEventAsync<EventType>(__VA_ARGS__)

#define REGISTER_EVENT_HANDLER(EventType, Function, Priority) \
    NLib::NEventDispatcher::GetGlobalDispatcher().RegisterHandler<EventType>(Function, Priority)

#define SUBSCRIBE_EVENT(EventType, Function) \
    REGISTER_EVENT_HANDLER(EventType, Function, 0)

} // namespace NLib