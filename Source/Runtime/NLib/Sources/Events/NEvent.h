#pragma once

#include "Core/CObject.h"
#include "Containers/CString.h"
#include "Containers/CArray.h"
#include "Containers/CHashMap.h"
#include "DateTime/NDateTime.h"
#include "Threading/CThread.h"
#include "Delegates/CDelegate.h"

namespace NLib
{

/**
 * @brief 事件基类 - 所有事件的基础类
 */
class LIBNLIB_API NEvent : public CObject
{
    NCLASS()
class NEvent : public NObject
{
    GENERATED_BODY()

public:
    NEvent();
    explicit NEvent(const CString& InEventType);
    virtual ~NEvent();
    
    // 事件标识
    uint64_t GetEventId() const { return EventId; }
    const CString& GetEventType() const { return EventType; }
    void SetEventType(const CString& InEventType) { EventType = InEventType; }
    
    // 时间戳
    const NDateTime& GetTimestamp() const { return Timestamp; }
    void SetTimestamp(const NDateTime& InTimestamp) { Timestamp = InTimestamp; }
    
    // 事件控制
    bool IsConsumed() const { return bConsumed; }
    void Consume() { bConsumed = true; }
    void Unconsume() { bConsumed = false; }
    
    bool IsCancellable() const { return bCancellable; }
    void SetCancellable(bool bInCancellable) { bCancellable = bInCancellable; }
    
    bool IsCancelled() const { return bCancelled; }
    void Cancel() { if (bCancellable) bCancelled = true; }
    
    // 事件优先级
    int32_t GetPriority() const { return Priority; }
    void SetPriority(int32_t InPriority) { Priority = InPriority; }
    
    // 事件源
    CObject* GetSource() const { return Source.Get(); }
    void SetSource(CObject* InSource) { Source = InSource; }
    
    // 事件数据
    template<typename TType>
    void SetData(const CString& Key, const TType& Value);
    
    template<typename TType>
    TType GetData(const CString& Key, const TType& DefaultValue = TType{}) const;
    
    bool HasData(const CString& Key) const;
    void RemoveData(const CString& Key);
    void ClearData();
    
    // 事件分类
    const CArray<CString>& GetCategories() const { return Categories; }
    void AddCategory(const CString& Category);
    void RemoveCategory(const CString& Category);
    bool HasCategory(const CString& Category) const;
    void ClearCategories();
    
    // 事件克隆
    virtual TSharedPtr<NEvent> Clone() const;
    
    virtual CString ToString() const override;

protected:
    // 可由子类重写的方法
    virtual void OnConsumed() {}
    virtual void OnCancelled() {}

private:
    uint64_t EventId;
    CString EventType;
    NDateTime Timestamp;
    bool bConsumed;
    bool bCancellable;
    bool bCancelled;
    int32_t Priority;
    TWeakPtr<CObject> Source;
    CHashMap<CString, CString> EventData; // 简化实现，实际应该支持任意类型
    CArray<CString> Categories;
    
    static CAtomic<uint64_t> NextEventId;
};

/**
 * @brief 事件处理器接口
 */
class LIBNLIB_API IEventHandler
{
public:
    virtual ~IEventHandler() = default;
    
    // 事件处理
    virtual void HandleEvent(TSharedPtr<NEvent> Event) = 0;
    
    // 处理器信息
    virtual bool CanHandle(const CString& EventType) const = 0;
    virtual bool CanHandle(TSharedPtr<NEvent> Event) const { return CanHandle(Event->GetEventType()); }
    
    // 处理器优先级（数值越大优先级越高）
    virtual int32_t GetPriority() const { return 0; }
    
    // 处理器名称
    virtual CString GetHandlerName() const { return "IEventHandler"; }
    
    // 是否启用
    virtual bool IsEnabled() const { return true; }
};

/**
 * @brief 类型安全的事件处理器模板
 */
template<typename TEvent>
class CEventHandler : public IEventHandler
{
    static_assert(std::is_base_of_v<NEvent, TEvent>, "TEvent must derive from NEvent");
    
public:
    using HandlerFunction = NFunction<void(TSharedPtr<TEvent>)>;
    
    CEventHandler(HandlerFunction InFunction, int32_t InPriority = 0);
    virtual ~CEventHandler();
    
    virtual void HandleEvent(TSharedPtr<NEvent> Event) override;
    virtual bool CanHandle(const CString& EventType) const override;
    virtual int32_t GetPriority() const override;
    virtual CString GetHandlerName() const override;
    
    void SetEnabled(bool bInEnabled) { bEnabled = bInEnabled; }
    virtual bool IsEnabled() const override { return bEnabled; }

private:
    HandlerFunction Function;
    int32_t Priority;
    bool bEnabled;
    CString EventTypeName;
};

/**
 * @brief 多类型事件处理器
 */
class LIBNLIB_API NMultiEventHandler : public IEventHandler
{
public:
    using GenericHandlerFunction = NFunction<void(TSharedPtr<NEvent>)>;
    
    NMultiEventHandler(int32_t InPriority = 0);
    virtual ~NMultiEventHandler();
    
    // 添加类型特定的处理器
    template<typename TEvent>
    void AddHandler(typename CEventHandler<TEvent>::HandlerFunction Function);
    
    // 添加通用处理器
    void AddGenericHandler(const CString& EventType, GenericHandlerFunction Function);
    
    // 移除处理器
    void RemoveHandler(const CString& EventType);
    void ClearHandlers();
    
    virtual void HandleEvent(TSharedPtr<NEvent> Event) override;
    virtual bool CanHandle(const CString& EventType) const override;
    virtual int32_t GetPriority() const override;
    virtual CString GetHandlerName() const override;

private:
    CHashMap<CString, GenericHandlerFunction> Handlers;
    int32_t Priority;
};

/**
 * @brief 事件过滤器
 */
class LIBNLIB_API NEventFilter : public CObject
{
    NCLASS()
class NEventFilter : public NObject
{
    GENERATED_BODY()

public:
    using FilterFunction = NFunction<bool(TSharedPtr<NEvent>)>;
    
    NEventFilter();
    explicit NEventFilter(FilterFunction InFunction);
    virtual ~NEventFilter();
    
    virtual bool ShouldProcess(TSharedPtr<NEvent> Event) const;
    
    void SetFilterFunction(FilterFunction InFunction) { Function = InFunction; }
    
    // 静态过滤器工厂
    static TSharedPtr<NEventFilter> ByType(const CString& EventType);
    static TSharedPtr<NEventFilter> ByTypes(const CArray<CString>& EventTypes);
    static TSharedPtr<NEventFilter> ByCategory(const CString& Category);
    static TSharedPtr<NEventFilter> BySource(CObject* Source);
    static TSharedPtr<NEventFilter> ByPriority(int32_t MinPriority, int32_t MaxPriority = INT32_MAX);
    static TSharedPtr<NEventFilter> Custom(FilterFunction Function);

private:
    FilterFunction Function;
};

/**
 * @brief 事件统计信息
 */
struct LIBNLIB_API NEventStatistics
{
    uint64_t TotalEventsProcessed;
    uint64_t TotalEventsConsumed;
    uint64_t TotalEventsCancelled;
    CHashMap<CString, uint64_t> EventTypeCount;
    CHashMap<CString, uint64_t> HandlerExecutionCount;
    double AverageProcessingTime;
    double MaxProcessingTime;
    NDateTime LastEventTime;
    
    NEventStatistics()
        : TotalEventsProcessed(0), TotalEventsConsumed(0), TotalEventsCancelled(0)
        , AverageProcessingTime(0.0), MaxProcessingTime(0.0)
    {}
    
    void Reset();
    CString ToString() const;
};

// === 模板实现 ===

template<typename TType>
void NEvent::SetData(const CString& Key, const TType& Value)
{
    if constexpr (std::is_same_v<TType, CString>)
    {
        EventData[Key] = Value;
    }
    else if constexpr (std::is_arithmetic_v<TType>)
    {
        EventData[Key] = CString::Format("{}", Value);
    }
    else
    {
        EventData[Key] = Value.ToString(); // 假设T有ToString方法
    }
}

template<typename TType>
TType NEvent::GetData(const CString& Key, const TType& DefaultValue) const
{
    if (!EventData.Contains(Key))
    {
        return DefaultValue;
    }
    
    const CString& ValueStr = EventData[Key];
    
    if constexpr (std::is_same_v<TType, CString>)
    {
        return ValueStr;
    }
    else if constexpr (std::is_same_v<TType, bool>)
    {
        return ValueStr.ToLower() == "true" || ValueStr == "1";
    }
    else if constexpr (std::is_same_v<TType, int32_t>)
    {
        return static_cast<int32_t>(strtol(ValueStr.GetCStr(), nullptr, 10));
    }
    else if constexpr (std::is_same_v<TType, int64_t>)
    {
        return strtoll(ValueStr.GetCStr(), nullptr, 10);
    }
    else if constexpr (std::is_same_v<TType, float>)
    {
        return static_cast<float>(strtod(ValueStr.GetCStr(), nullptr));
    }
    else if constexpr (std::is_same_v<TType, double>)
    {
        return strtod(ValueStr.GetCStr(), nullptr);
    }
    else
    {
        return DefaultValue; // 不支持的类型返回默认值
    }
}

template<typename TEvent>
CEventHandler<TEvent>::CEventHandler(HandlerFunction InFunction, int32_t InPriority)
    : Function(InFunction), Priority(InPriority), bEnabled(true)
{
    // 获取事件类型名称（简化实现）
    EventTypeName = typeid(TEvent).name();
}

template<typename TEvent>
CEventHandler<TEvent>::~CEventHandler()
{}

template<typename TEvent>
void CEventHandler<TEvent>::HandleEvent(TSharedPtr<NEvent> Event)
{
    if (!bEnabled || !Function)
    {
        return;
    }
    
    // 尝试转换为目标事件类型
    if (auto TypedEvent = CObject::Cast<TEvent>(Event))
    {
        Function(TypedEvent);
    }
}

template<typename TEvent>
bool CEventHandler<TEvent>::CanHandle(const CString& EventType) const
{
    return EventType == EventTypeName;
}

template<typename TEvent>
int32_t CEventHandler<TEvent>::GetPriority() const
{
    return Priority;
}

template<typename TEvent>
CString CEventHandler<TEvent>::GetHandlerName() const
{
    return CString::Format("CEventHandler<{}>", EventTypeName);
}

template<typename TEvent>
void NMultiEventHandler::AddHandler(typename CEventHandler<TEvent>::HandlerFunction Function)
{
    CString EventTypeName = typeid(TEvent).name();
    Handlers[EventTypeName] = [Function](TSharedPtr<NEvent> Event) {
        if (auto TypedEvent = CObject::Cast<TEvent>(Event))
        {
            Function(TypedEvent);
        }
    };
}

} // namespace NLib