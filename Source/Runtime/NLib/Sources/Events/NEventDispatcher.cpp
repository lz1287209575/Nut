#include "Events/NEventDispatcher.h"
#include "Core/CLogger.h"
#include "DateTime/NDateTime.h"

namespace NLib
{

// === NEventDispatcher 实现 ===

NEventDispatcher::NEventDispatcher()
    : MaxQueueSize(10000)
    , AsyncThreadCount(2)
    , bBatchProcessingEnabled(false)
    , bStatisticsEnabled(true)
    , bEventLoggingEnabled(false)
    , MaxLoggedEvents(1000)
    , bInitialized(false)
    , bShuttingDown(false)
    , bPaused(false)
{
}

NEventDispatcher::~NEventDispatcher()
{
    Shutdown();
}

NEventDispatcher& NEventDispatcher::GetGlobalDispatcher()
{
    static NEventDispatcher GlobalInstance;
    return GlobalInstance;
}

void NEventDispatcher::Initialize()
{
    if (bInitialized)
    {
        return;
    }
    
    bInitialized = true;
    bShuttingDown = false;
    
    // 创建异步任务调度器
    AsyncScheduler = NewNObject<NAsyncTaskScheduler>(AsyncThreadCount);
    AsyncScheduler->Start();
    
    CLogger::Get().LogInfo("NEventDispatcher initialized with {} async threads", AsyncThreadCount);
}

void NEventDispatcher::Shutdown()
{
    if (!bInitialized || bShuttingDown)
    {
        return;
    }
    
    bShuttingDown = true;
    
    // 处理剩余的延迟事件
    ProcessQueuedEvents();
    
    // 停止异步调度器
    if (AsyncScheduler)
    {
        AsyncScheduler->Stop();
        AsyncScheduler.Reset();
    }
    
    // 清理事件队列
    ClearEventQueue();
    
    // 清理处理器
    UnregisterAllHandlers();
    
    bInitialized = false;
    
    CLogger::Get().LogInfo("NEventDispatcher shutdown completed");
}

void NEventDispatcher::Update()
{
    if (!bInitialized || bShuttingDown || bPaused)
    {
        return;
    }
    
    // 处理延迟事件
    CArray<TSharedPtr<NEvent>> EventsToProcess;
    
    {
        CLockGuard<NMutex> Lock(EventQueueMutex);
        EventsToProcess = NLib::Move(DeferredEvents);
        DeferredEvents.Clear();
    }
    
    for (auto& Event : EventsToProcess)
    {
        if (Event && !Event->IsCancelled())
        {
            ProcessEventImmediate(Event);
        }
    }
    
    // 批量处理队列事件
    if (bBatchProcessingEnabled)
    {
        ProcessQueuedEvents();
    }
}

void NEventDispatcher::DispatchEvent(TSharedPtr<NEvent> Event, EEventDispatchMode Mode)
{
    if (!Event || bShuttingDown)
    {
        return;
    }
    
    if (bPaused || IsEventTypePaused(Event->GetEventType()))
    {
        return;
    }
    
    // 记录事件
    if (bEventLoggingEnabled)
    {
        LogEvent(Event);
    }
    
    switch (Mode)
    {
    case EEventDispatchMode::Immediate:
        ProcessEventImmediate(Event);
        break;
        
    case EEventDispatchMode::Deferred:
        DispatchEventDeferred(Event);
        break;
        
    case EEventDispatchMode::Async:
        ProcessEventAsync(Event);
        break;
        
    case EEventDispatchMode::Queued:
        QueueEvent(Event);
        break;
    }
}

void NEventDispatcher::DispatchEventDeferred(TSharedPtr<NEvent> Event)
{
    if (!Event || bShuttingDown)
    {
        return;
    }
    
    CLockGuard<NMutex> Lock(EventQueueMutex);
    DeferredEvents.PushBack(Event);
}

void NEventDispatcher::DispatchEventAsync(TSharedPtr<NEvent> Event)
{
    ProcessEventAsync(Event);
}

void NEventDispatcher::DispatchEvents(const CArray<TSharedPtr<NEvent>>& Events, EEventDispatchMode Mode)
{
    for (const auto& Event : Events)
    {
        DispatchEvent(Event, Mode);
    }
}

void NEventDispatcher::QueueEvent(TSharedPtr<NEvent> Event)
{
    if (!Event || bShuttingDown)
    {
        return;
    }
    
    CLockGuard<NMutex> Lock(EventQueueMutex);
    
    if (EventQueue.GetSize() >= MaxQueueSize)
    {
        CLogger::Get().LogWarning("Event queue is full, discarding oldest event");
        EventQueue.RemoveAt(0);
    }
    
    EventQueue.PushBack(Event);
}

void NEventDispatcher::ProcessQueuedEvents()
{
    CArray<TSharedPtr<NEvent>> EventsToProcess;
    
    {
        CLockGuard<NMutex> Lock(EventQueueMutex);
        EventsToProcess = NLib::Move(EventQueue);
        EventQueue.Clear();
    }
    
    for (auto& Event : EventsToProcess)
    {
        if (Event && !Event->IsCancelled())
        {
            ProcessEventImmediate(Event);
        }
    }
}

void NEventDispatcher::ClearEventQueue()
{
    CLockGuard<NMutex> Lock(EventQueueMutex);
    EventQueue.Clear();
    DeferredEvents.Clear();
}

void NEventDispatcher::RegisterHandler(TSharedPtr<IEventHandler> Handler)
{
    if (!Handler)
    {
        return;
    }
    
    CLockGuard<NMutex> Lock(HandlerMutex);
    
    // 添加到全局处理器列表
    GlobalHandlers.PushBack(HandlerEntry(Handler, Handler->GetPriority()));
    
    // 按优先级排序
    SortHandlers();
    
    CLogger::Get().LogInfo("Registered global event handler with priority {}", Handler->GetPriority());
}

void NEventDispatcher::RegisterHandler(const CString& EventType, TSharedPtr<IEventHandler> Handler)
{
    if (!Handler || EventType.IsEmpty())
    {
        return;
    }
    
    CLockGuard<NMutex> Lock(HandlerMutex);
    
    HandlerMap[EventType].PushBack(HandlerEntry(Handler, Handler->GetPriority()));
    
    // 按优先级排序
    SortHandlers();
    
    CLogger::Get().LogInfo("Registered event handler for type '{}' with priority {}", EventType, Handler->GetPriority());
}

void NEventDispatcher::UnregisterHandler(TSharedPtr<IEventHandler> Handler)
{
    if (!Handler)
    {
        return;
    }
    
    CLockGuard<NMutex> Lock(HandlerMutex);
    
    // 从全局处理器中移除
    for (auto It = GlobalHandlers.Begin(); It != GlobalHandlers.End(); ++It)
    {
        if (It->Handler == Handler)
        {
            GlobalHandlers.Erase(It);
            break;
        }
    }
    
    // 从特定类型处理器中移除
    for (auto& Pair : HandlerMap)
    {
        auto& Handlers = Pair.Value;
        for (auto It = Handlers.Begin(); It != Handlers.End(); ++It)
        {
            if (It->Handler == Handler)
            {
                Handlers.Erase(It);
                break;
            }
        }
    }
}

void NEventDispatcher::UnregisterHandler(const CString& EventType, TSharedPtr<IEventHandler> Handler)
{
    if (!Handler || EventType.IsEmpty())
    {
        return;
    }
    
    CLockGuard<NMutex> Lock(HandlerMutex);
    
    auto It = HandlerMap.Find(EventType);
    if (It != HandlerMap.End())
    {
        auto& Handlers = It->Value;
        for (auto HandlerIt = Handlers.Begin(); HandlerIt != Handlers.End(); ++HandlerIt)
        {
            if (HandlerIt->Handler == Handler)
            {
                Handlers.Erase(HandlerIt);
                break;
            }
        }
        
        // 如果列表为空，移除该事件类型
        if (Handlers.IsEmpty())
        {
            HandlerMap.Remove(EventType);
        }
    }
}

void NEventDispatcher::UnregisterAllHandlers()
{
    CLockGuard<NMutex> Lock(HandlerMutex);
    HandlerMap.Clear();
    GlobalHandlers.Clear();
}

void NEventDispatcher::UnregisterAllHandlers(const CString& EventType)
{
    CLockGuard<NMutex> Lock(HandlerMutex);
    HandlerMap.Remove(EventType);
}

bool NEventDispatcher::HasHandler(const CString& EventType) const
{
    CLockGuard<NMutex> Lock(HandlerMutex);
    return HandlerMap.Contains(EventType) || !GlobalHandlers.IsEmpty();
}

int32_t NEventDispatcher::GetHandlerCount(const CString& EventType) const
{
    CLockGuard<NMutex> Lock(HandlerMutex);
    
    int32_t Count = GlobalHandlers.GetSize();
    
    auto It = HandlerMap.Find(EventType);
    if (It != HandlerMap.End())
    {
        Count += It->Value.GetSize();
    }
    
    return Count;
}

int32_t NEventDispatcher::GetTotalHandlerCount() const
{
    CLockGuard<NMutex> Lock(HandlerMutex);
    
    int32_t Total = GlobalHandlers.GetSize();
    
    for (const auto& Pair : HandlerMap)
    {
        Total += Pair.Value.GetSize();
    }
    
    return Total;
}

CArray<TSharedPtr<IEventHandler>> NEventDispatcher::GetHandlers(const CString& EventType) const
{
    CLockGuard<NMutex> Lock(HandlerMutex);
    
    CArray<TSharedPtr<IEventHandler>> Result;
    
    // 添加全局处理器
    for (const auto& Entry : GlobalHandlers)
    {
        if (Entry.bEnabled && Entry.Handler->CanHandle(EventType))
        {
            Result.PushBack(Entry.Handler);
        }
    }
    
    // 添加特定类型处理器
    auto It = HandlerMap.Find(EventType);
    if (It != HandlerMap.End())
    {
        for (const auto& Entry : It->Value)
        {
            if (Entry.bEnabled)
            {
                Result.PushBack(Entry.Handler);
            }
        }
    }
    
    return Result;
}

void NEventDispatcher::AddFilter(TSharedPtr<NEventFilter> Filter)
{
    if (Filter)
    {
        Filters.PushBack(Filter);
    }
}

void NEventDispatcher::RemoveFilter(TSharedPtr<NEventFilter> Filter)
{
    Filters.Remove(Filter);
}

void NEventDispatcher::ClearFilters()
{
    Filters.Clear();
}

bool NEventDispatcher::PassesFilters(TSharedPtr<NEvent> Event) const
{
    for (const auto& Filter : Filters)
    {
        if (Filter && Filter->IsEnabled() && !Filter->ShouldProcess(Event))
        {
            return false;
        }
    }
    return true;
}

void NEventDispatcher::AddInterceptor(EventInterceptor Interceptor, int32_t Priority)
{
    Interceptors.PushBack(InterceptorEntry(Interceptor, Priority));
    SortInterceptors();
}

void NEventDispatcher::RemoveInterceptor(EventInterceptor Interceptor)
{
    for (auto It = Interceptors.Begin(); It != Interceptors.End(); ++It)
    {
        // 简化比较：在实际实现中需要更好的函数比较方法
        Interceptors.Erase(It);
        break;
    }
}

void NEventDispatcher::ClearInterceptors()
{
    Interceptors.Clear();
}

void NEventDispatcher::SetMaxQueueSize(int32_t MaxSize)
{
    MaxQueueSize = NMath::Max(1, MaxSize);
}

void NEventDispatcher::SetAsyncThreadCount(int32_t ThreadCount)
{
    AsyncThreadCount = NMath::Max(1, ThreadCount);
    
    if (AsyncScheduler)
    {
        AsyncScheduler->SetMaxConcurrency(AsyncThreadCount);
    }
}

void NEventDispatcher::Pause()
{
    bPaused = true;
}

void NEventDispatcher::Resume()
{
    bPaused = false;
}

void NEventDispatcher::PauseEventType(const CString& EventType)
{
    PausedEventTypes[EventType] = true;
}

void NEventDispatcher::ResumeEventType(const CString& EventType)
{
    PausedEventTypes.Remove(EventType);
}

bool NEventDispatcher::IsEventTypePaused(const CString& EventType) const
{
    return PausedEventTypes.Contains(EventType);
}

const NEventStatistics& NEventDispatcher::GetStatistics() const
{
    return Statistics;
}

void NEventDispatcher::ResetStatistics()
{
    CLockGuard<NMutex> Lock(StatisticsMutex);
    Statistics.Reset();
}

void NEventDispatcher::DumpStatistics() const
{
    CLogger::Get().LogInfo("Event Dispatcher Statistics:\n{}", Statistics.ToString());
}

CArray<CString> NEventDispatcher::GetRegisteredEventTypes() const
{
    CLockGuard<NMutex> Lock(HandlerMutex);
    
    CArray<CString> Types;
    for (const auto& Pair : HandlerMap)
    {
        Types.PushBack(Pair.Key);
    }
    
    return Types;
}

CString NEventDispatcher::GetDispatcherReport() const
{
    return CString::Format(
        "Event Dispatcher Report:\n"
        "  Initialized: {}\n"
        "  Paused: {}\n"
        "  Total Handlers: {}\n"
        "  Event Types: {}\n"
        "  Queue Size: {}/{}\n"
        "  Deferred Events: {}\n"
        "  Filters: {}\n"
        "  Interceptors: {}\n"
        "{}",
        bInitialized,
        bPaused,
        GetTotalHandlerCount(),
        HandlerMap.GetSize(),
        EventQueue.GetSize(),
        MaxQueueSize,
        DeferredEvents.GetSize(),
        Filters.GetSize(),
        Interceptors.GetSize(),
        Statistics.ToString()
    );
}

void NEventDispatcher::SetEventLoggingEnabled(bool bEnabled)
{
    bEventLoggingEnabled = bEnabled;
}

void NEventDispatcher::SetMaxLoggedEvents(int32_t MaxCount)
{
    MaxLoggedEvents = NMath::Max(0, MaxCount);
    
    // 裁剪现有日志
    while (RecentEvents.GetSize() > MaxLoggedEvents)
    {
        RecentEvents.RemoveAt(0);
    }
}

CArray<TSharedPtr<NEvent>> NEventDispatcher::GetRecentEvents() const
{
    return RecentEvents;
}

void NEventDispatcher::SetErrorHandler(ErrorHandler Handler)
{
    CurrentErrorHandler = Handler;
}

void NEventDispatcher::ClearErrorHandler()
{
    CurrentErrorHandler = nullptr;
}

void NEventDispatcher::DispatchEventInternal(TSharedPtr<NEvent> Event)
{
    ProcessEventImmediate(Event);
}

void NEventDispatcher::ProcessEventImmediate(TSharedPtr<NEvent> Event)
{
    if (!Event || Event->IsCancelled() || !PassesFilters(Event))
    {
        return;
    }
    
    NStopwatch Stopwatch;
    Stopwatch.Start();
    
    try
    {
        // 运行拦截器
        if (!RunInterceptors(Event))
        {
            return; // 事件被拦截
        }
        
        // 获取处理器
        CArray<HandlerEntry> Handlers = GetEventHandlers(Event);
        
        // 执行处理器
        for (const auto& Entry : Handlers)
        {
            if (Event->IsCancelled())
            {
                break;
            }
            
            ExecuteHandler(Entry, Event);
        }
        
        if (bStatisticsEnabled)
        {
            UpdateStatistics(Event, Stopwatch.GetElapsed().GetTotalSeconds());
        }
    }
    catch (...)
    {
        HandleError(Event, "Exception during event processing");
    }
}

void NEventDispatcher::ProcessEventAsync(TSharedPtr<NEvent> Event)
{
    if (!AsyncScheduler || !Event)
    {
        return;
    }
    
    auto Task = NAsyncTask<void>::Run([this, Event](NCancellationToken& Token) -> void {
        ProcessEventImmediate(Event);
    });
    
    AsyncScheduler->ScheduleTask(Task);
}

bool NEventDispatcher::RunInterceptors(TSharedPtr<NEvent> Event)
{
    for (const auto& Entry : Interceptors)
    {
        try
        {
            if (!Entry.Interceptor(Event))
            {
                return false; // 拦截器阻止了事件传播
            }
        }
        catch (...)
        {
            HandleError(Event, "Exception in event interceptor");
        }
    }
    return true;
}

CArray<NEventDispatcher::HandlerEntry> NEventDispatcher::GetEventHandlers(TSharedPtr<NEvent> Event) const
{
    CLockGuard<NMutex> Lock(HandlerMutex);
    
    CArray<HandlerEntry> Result;
    CString EventType = Event->GetEventType();
    
    // 添加全局处理器
    for (const auto& Entry : GlobalHandlers)
    {
        if (Entry.bEnabled && Entry.Handler->CanHandle(EventType))
        {
            Result.PushBack(Entry);
        }
    }
    
    // 添加特定类型处理器
    auto It = HandlerMap.Find(EventType);
    if (It != HandlerMap.End())
    {
        for (const auto& Entry : It->Value)
        {
            if (Entry.bEnabled)
            {
                Result.PushBack(Entry);
            }
        }
    }
    
    // 按优先级排序
    Result.Sort([](const HandlerEntry& A, const HandlerEntry& B) {
        return A.Priority > B.Priority;
    });
    
    return Result;
}

void NEventDispatcher::ExecuteHandler(const HandlerEntry& Entry, TSharedPtr<NEvent> Event)
{
    if (!Entry.Handler || !Entry.bEnabled)
    {
        return;
    }
    
    NStopwatch Stopwatch;
    Stopwatch.Start();
    
    try
    {
        Entry.Handler->HandleEvent(Event);
        
        if (bStatisticsEnabled)
        {
            CLockGuard<NMutex> Lock(StatisticsMutex);
            Statistics.UpdateHandlerExecuted(Stopwatch.GetElapsed().GetTotalSeconds());
        }
    }
    catch (...)
    {
        HandleError(Event, "Exception in event handler");
    }
}

void NEventDispatcher::UpdateStatistics(TSharedPtr<NEvent> Event, double ProcessingTime)
{
    CLockGuard<NMutex> Lock(StatisticsMutex);
    
    Statistics.UpdateEventDispatched(ProcessingTime);
    
    if (Event->IsHandled())
    {
        Statistics.UpdateEventHandled();
    }
    
    if (Event->IsCancelled())
    {
        Statistics.UpdateEventCancelled();
    }
}

void NEventDispatcher::LogEvent(TSharedPtr<NEvent> Event)
{
    if (!Event || MaxLoggedEvents <= 0)
    {
        return;
    }
    
    RecentEvents.PushBack(Event);
    
    while (RecentEvents.GetSize() > MaxLoggedEvents)
    {
        RecentEvents.RemoveAt(0);
    }
}

void NEventDispatcher::HandleError(TSharedPtr<NEvent> Event, const CString& ErrorMessage)
{
    CLogger::Get().LogError("Event Dispatcher Error: {}", ErrorMessage);
    
    if (CurrentErrorHandler)
    {
        try
        {
            CurrentErrorHandler(Event, ErrorMessage);
        }
        catch (...)
        {
            CLogger::Get().LogError("Exception in error handler");
        }
    }
}

void NEventDispatcher::SortHandlers()
{
    GlobalHandlers.Sort([](const HandlerEntry& A, const HandlerEntry& B) {
        return A.Priority > B.Priority;
    });
    
    for (auto& Pair : HandlerMap)
    {
        Pair.Value.Sort([](const HandlerEntry& A, const HandlerEntry& B) {
            return A.Priority > B.Priority;
        });
    }
}

void NEventDispatcher::SortInterceptors()
{
    Interceptors.Sort([](const InterceptorEntry& A, const InterceptorEntry& B) {
        return A.Priority > B.Priority;
    });
}

// === NEventBus 实现 ===

NEventBus::NEventBus()
    : Dispatcher(&NEventDispatcher::GetGlobalDispatcher())
{
}

NEventBus::NEventBus(TSharedPtr<NEventDispatcher> InDispatcher)
    : Dispatcher(InDispatcher)
{
    if (!Dispatcher)
    {
        Dispatcher = NewNObject<NEventDispatcher>();
    }
}

NEventBus::~NEventBus()
{
    UnsubscribeAll();
}

void NEventBus::Publish(TSharedPtr<NEvent> Event)
{
    if (Dispatcher)
    {
        Dispatcher->DispatchEvent(Event);
    }
}

void NEventBus::PublishDeferred(TSharedPtr<NEvent> Event)
{
    if (Dispatcher)
    {
        Dispatcher->DispatchEventDeferred(Event);
    }
}

void NEventBus::PublishAsync(TSharedPtr<NEvent> Event)
{
    if (Dispatcher)
    {
        Dispatcher->DispatchEventAsync(Event);
    }
}

void NEventBus::UnsubscribeAll()
{
    if (!Dispatcher)
    {
        return;
    }
    
    for (auto& Handler : RegisteredHandlers)
    {
        Dispatcher->UnregisterHandler(Handler);
    }
    RegisteredHandlers.Clear();
}

void NEventBus::Pause()
{
    if (Dispatcher)
    {
        Dispatcher->Pause();
    }
}

void NEventBus::Resume()
{
    if (Dispatcher)
    {
        Dispatcher->Resume();
    }
}

bool NEventBus::IsPaused() const
{
    return Dispatcher ? Dispatcher->IsPaused() : true;
}

NEventDispatcher* NEventBus::GetDispatcher() const
{
    return Dispatcher.Get();
}

} // namespace NLib