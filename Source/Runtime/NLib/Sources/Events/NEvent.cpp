#include "Events/NEvent.h"
#include "Core/CLogger.h"
#include "DateTime/NDateTime.h"

namespace NLib
{

// === NEvent 基类实现 ===

CAtomic<uint64_t> NEvent::NextEventId(1);

NEvent::NEvent()
    : EventId(NextEventId++)
    , Timestamp(NDateTime::Now())
    , bHandled(false)
    , bCancellable(true)
    , bCancelled(false)
    , Priority(0)
{
}

NEvent::NEvent(const CString& InEventType)
    : NEvent()
{
    EventType = InEventType;
}

NEvent::~NEvent()
{
}

void NEvent::Cancel()
{
    if (bCancellable && !bCancelled)
    {
        bCancelled = true;
        OnCancelled();
    }
}

void NEvent::SetHandled(bool bInHandled)
{
    bHandled = bInHandled;
    if (bHandled)
    {
        OnHandled();
    }
}

CString NEvent::GetEventType() const
{
    if (!EventType.IsEmpty())
    {
        return EventType;
    }
    
    // 如果没有显式设置类型，使用RTTI类型名
    return typeid(*this).name();
}

double NEvent::GetAge() const
{
    NDateTime Now = NDateTime::Now();
    NTimespan Age = Now - Timestamp;
    return Age.GetTotalSeconds();
}

CString NEvent::ToString() const
{
    return CString::Format(
        "Event(Id={}, Type={}, Handled={}, Cancelled={}, Age={:.3f}s)",
        EventId,
        GetEventType(),
        bHandled,
        bCancelled,
        GetAge()
    );
}

void NEvent::OnHandled()
{
    // 默认实现：什么都不做
}

void NEvent::OnCancelled()
{
    // 默认实现：什么都不做
}

// === NEventStatistics 实现 ===

NEventStatistics::NEventStatistics()
{
    Reset();
}

void NEventStatistics::Reset()
{
    TotalEventsDispatched = 0;
    TotalEventsHandled = 0;
    TotalEventsCancelled = 0;
    TotalHandlersExecuted = 0;
    TotalProcessingTime = 0.0;
    AverageProcessingTime = 0.0;
    MaxProcessingTime = 0.0;
    MinProcessingTime = DBL_MAX;
    EventsPerSecond = 0.0;
    StartTime = NDateTime::Now();
    LastResetTime = StartTime;
}

void NEventStatistics::UpdateEventDispatched(double ProcessingTime)
{
    TotalEventsDispatched++;
    UpdateProcessingTime(ProcessingTime);
}

void NEventStatistics::UpdateEventHandled()
{
    TotalEventsHandled++;
}

void NEventStatistics::UpdateEventCancelled()
{
    TotalEventsCancelled++;
}

void NEventStatistics::UpdateHandlerExecuted(double HandlerTime)
{
    TotalHandlersExecuted++;
    // 可以添加处理器特定的统计
}

void NEventStatistics::UpdateProcessingTime(double ProcessingTime)
{
    TotalProcessingTime += ProcessingTime;
    
    if (ProcessingTime > MaxProcessingTime)
    {
        MaxProcessingTime = ProcessingTime;
    }
    
    if (ProcessingTime < MinProcessingTime)
    {
        MinProcessingTime = ProcessingTime;
    }
    
    if (TotalEventsDispatched > 0)
    {
        AverageProcessingTime = TotalProcessingTime / TotalEventsDispatched;
    }
    
    // 计算每秒事件数
    NDateTime Now = NDateTime::Now();
    NTimespan Elapsed = Now - StartTime;
    double ElapsedSeconds = Elapsed.GetTotalSeconds();
    
    if (ElapsedSeconds > 0.0)
    {
        EventsPerSecond = TotalEventsDispatched / ElapsedSeconds;
    }
}

CString NEventStatistics::ToString() const
{
    return CString::Format(
        "Event Statistics:\n"
        "  Total Events Dispatched: {}\n"
        "  Total Events Handled: {}\n"
        "  Total Events Cancelled: {}\n"
        "  Total Handlers Executed: {}\n"
        "  Total Processing Time: {:.3f}s\n"
        "  Average Processing Time: {:.3f}ms\n"
        "  Max Processing Time: {:.3f}ms\n"
        "  Min Processing Time: {:.3f}ms\n"
        "  Events Per Second: {:.2f}",
        TotalEventsDispatched,
        TotalEventsHandled,
        TotalEventsCancelled,
        TotalHandlersExecuted,
        TotalProcessingTime,
        AverageProcessingTime * 1000.0,
        MaxProcessingTime * 1000.0,
        MinProcessingTime == DBL_MAX ? 0.0 : MinProcessingTime * 1000.0,
        EventsPerSecond
    );
}

// === IEventHandler 基类实现 ===

IEventHandler::IEventHandler()
    : Priority(0)
    , bEnabled(true)
{
}

IEventHandler::~IEventHandler()
{
}

bool IEventHandler::CanHandle(const CString& EventType) const
{
    return GetSupportedEventTypes().Contains(EventType);
}

// === IEventFilter 基类实现 ===

IEventFilter::IEventFilter()
    : bEnabled(true)
{
}

IEventFilter::~IEventFilter()
{
}

// === 具体事件类型实现 ===

// NStringEvent
NStringEvent::NStringEvent(const CString& InMessage)
    : NEvent("StringEvent")
    , Message(InMessage)
{
}

CString NStringEvent::ToString() const
{
    return CString::Format("StringEvent(Message=\"{}\")", Message);
}

// NIntEvent  
NIntEvent::NIntEvent(int32_t InValue)
    : NEvent("IntEvent")
    , Value(InValue)
{
}

CString NIntEvent::ToString() const
{
    return CString::Format("IntEvent(Value={})", Value);
}

// NFloatEvent
NFloatEvent::NFloatEvent(float InValue)
    : NEvent("FloatEvent")
    , Value(InValue)
{
}

CString NFloatEvent::ToString() const
{
    return CString::Format("FloatEvent(Value={:.3f})", Value);
}

// NBoolEvent
NBoolEvent::NBoolEvent(bool InValue)
    : NEvent("BoolEvent")
    , Value(InValue)
{
}

CString NBoolEvent::ToString() const
{
    return CString::Format("BoolEvent(Value={})", Value ? "true" : "false");
}

// === 具体处理器实现 ===

NLambdaEventHandler::NLambdaEventHandler(const CString& InEventType, const HandlerFunction& InFunction, int32_t InPriority)
    : EventType(InEventType)
    , Function(InFunction)
{
    SetPriority(InPriority);
}

NLambdaEventHandler::~NLambdaEventHandler()
{
}

void NLambdaEventHandler::HandleEvent(TSharedPtr<NEvent> Event)
{
    if (Function && IsEnabled())
    {
        Function(Event);
    }
}

bool NLambdaEventHandler::CanHandle(const CString& InEventType) const
{
    return EventType == InEventType || EventType == "*"; // "*" 表示处理所有事件
}

CArray<CString> NLambdaEventHandler::GetSupportedEventTypes() const
{
    return {EventType};
}

// === 具体过滤器实现 ===

NEventTypeFilter::NEventTypeFilter(const CArray<CString>& InAllowedTypes)
    : AllowedTypes(InAllowedTypes)
{
}

NEventTypeFilter::~NEventTypeFilter()
{
}

bool NEventTypeFilter::ShouldProcess(TSharedPtr<NEvent> Event)
{
    if (!IsEnabled() || !Event)
    {
        return false;
    }
    
    if (AllowedTypes.IsEmpty())
    {
        return true; // 空列表表示允许所有类型
    }
    
    CString EventType = Event->GetEventType();
    return AllowedTypes.Contains(EventType);
}

void NEventTypeFilter::AddAllowedType(const CString& EventType)
{
    if (!AllowedTypes.Contains(EventType))
    {
        AllowedTypes.PushBack(EventType);
    }
}

void NEventTypeFilter::RemoveAllowedType(const CString& EventType)
{
    AllowedTypes.Remove(EventType);
}

void NEventTypeFilter::ClearAllowedTypes()
{
    AllowedTypes.Clear();
}

NEventPriorityFilter::NEventPriorityFilter(int32_t InMinPriority, int32_t InMaxPriority)
    : MinPriority(InMinPriority)
    , MaxPriority(InMaxPriority)
{
}

NEventPriorityFilter::~NEventPriorityFilter()
{
}

bool NEventPriorityFilter::ShouldProcess(TSharedPtr<NEvent> Event)
{
    if (!IsEnabled() || !Event)
    {
        return false;
    }
    
    int32_t EventPriority = Event->GetPriority();
    return EventPriority >= MinPriority && EventPriority <= MaxPriority;
}

} // namespace NLib