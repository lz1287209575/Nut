#pragma once

#include "Core/CObject.h"
#include "Containers/CString.h"
#include "Threading/CThread.h"
#include "Delegates/CDelegate.h"
#include "NAsyncTask.h"

namespace NLib
{

/**
 * @brief Future状态枚举
 */
enum class EFutureState : uint32_t
{
    Pending,    // 等待中
    Completed,  // 已完成
    Cancelled,  // 已取消
    Faulted     // 执行失败
};

/**
 * @brief Promise-Future模式的Promise端
 */
template<typename TType>
class CPromise;

/**
 * @brief Future基类 - 不依赖std的Future实现
 */
template<typename TType>
class NFuture : public NObject
{

public:
    NFuture();
    virtual ~NFuture();
    
    // 状态查询
    bool IsReady() const;
    bool IsPending() const;
    bool IsCompleted() const;
    bool IsCancelled() const;
    bool IsFaulted() const;
    EFutureState GetState() const;
    
    // 获取结果
    TType Get();
    TType Get(int32_t TimeoutMs);
    bool TryGet(TType& OutValue);
    bool TryGet(TType& OutValue, int32_t TimeoutMs);
    
    // 异步结果包装
    NAsyncResult<TType> GetAsyncResult();
    
    // 等待完成
    void Wait();
    bool Wait(int32_t TimeoutMs);
    
    // 取消操作
    void Cancel();
    
    // 异常信息
    CString GetExceptionMessage() const;
    
    // 链式操作
    template<typename TResult>
    TSharedPtr<NFuture<TResult>> Then(NFunction<TResult(TType)> Continuation);
    
    template<typename TResult>
    TSharedPtr<NFuture<TResult>> Then(NFunction<TResult(const NFuture<TType>&)> Continuation);
    
    // 静态工厂方法
    static TSharedPtr<NFuture<TType>> FromValue(const TType& Value);
    static TSharedPtr<NFuture<TType>> FromValue(TType&& Value);
    static TSharedPtr<NFuture<TType>> FromException(const CString& ExceptionMessage);
    static TSharedPtr<NFuture<TType>> FromAsyncTask(TSharedPtr<NAsyncTask<TType>> Task);
    
    // 组合操作
    static TSharedPtr<NFuture<CArray<TType>>> WhenAll(const CArray<TSharedPtr<NFuture<TType>>>& Futures);
    static TSharedPtr<NFuture<TType>> WhenAny(const CArray<TSharedPtr<NFuture<TType>>>& Futures);
    
    // 回调
    void OnCompleted(NFunction<void(const TType&)> Callback);
    void OnFaulted(NFunction<void(const CString&)> Callback);
    void OnCancelled(NFunction<void()> Callback);

private:
    friend class CPromise<TType>;
    
    EFutureState State;
    TType Value;
    CString ExceptionMessage;
    bool bHasValue;
    
    NEvent CompletionEvent;
    NMutex StateMutex;
    
    // 回调列表
    CArray<NFunction<void(const TType&)>> CompletedCallbacks;
    CArray<NFunction<void(const CString&)>> FaultedCallbacks;
    CArray<NFunction<void()>> CancelledCallbacks;
    
    void SetValue(const TType& InValue);
    void SetValue(TType&& InValue);
    void SetException(const CString& InExceptionMessage);
    void SetCancelled();
    void NotifyCallbacks();
    
    // 禁止复制
    NFuture(const NFuture&) = delete;
    NFuture& operator=(const NFuture&) = delete;
};

/**
 * @brief void特化版本
 */
template<>
class LIBNLIB_API NFuture<void> : public CObject
{
public:
    NFuture();
    virtual ~NFuture();
    
    // 状态查询
    bool IsReady() const;
    bool IsPending() const;
    bool IsCompleted() const;
    bool IsCancelled() const;
    bool IsFaulted() const;
    EFutureState GetState() const;
    
    // 获取结果
    void Get();
    bool Get(int32_t TimeoutMs);
    bool TryGet();
    bool TryGet(int32_t TimeoutMs);
    
    // 等待完成
    void Wait();
    bool Wait(int32_t TimeoutMs);
    
    // 取消操作
    void Cancel();
    
    // 异常信息
    CString GetExceptionMessage() const;
    
    // 链式操作
    template<typename TResult>
    TSharedPtr<NFuture<TResult>> Then(NFunction<TResult()> Continuation);
    
    // 静态工厂方法
    static TSharedPtr<NFuture<void>> CompletedFuture();
    static TSharedPtr<NFuture<void>> FromException(const CString& ExceptionMessage);
    static TSharedPtr<NFuture<void>> FromAsyncTask(TSharedPtr<NAsyncTask<void>> Task);
    
    // 组合操作
    static TSharedPtr<NFuture<void>> WhenAll(const CArray<TSharedPtr<NFuture<void>>>& Futures);
    
    // 回调
    void OnCompleted(NFunction<void()> Callback);
    void OnFaulted(NFunction<void(const CString&)> Callback);
    void OnCancelled(NFunction<void()> Callback);

private:
    friend class CPromise<void>;
    
    EFutureState State;
    CString ExceptionMessage;
    
    NEvent CompletionEvent;
    NMutex StateMutex;
    
    // 回调列表
    CArray<NFunction<void()>> CompletedCallbacks;
    CArray<NFunction<void(const CString&)>> FaultedCallbacks;
    CArray<NFunction<void()>> CancelledCallbacks;
    
    void SetCompleted();
    void SetException(const CString& InExceptionMessage);
    void SetCancelled();
    void NotifyCallbacks();
    
    // 禁止复制
    NFuture(const NFuture&) = delete;
    NFuture& operator=(const NFuture&) = delete;
};

/**
 * @brief Promise - Future的设置端
 */
template<typename TType>
class CPromise : public CObject
{
public:
    CPromise();
    virtual ~CPromise();
    
    // 获取关联的Future
    TSharedPtr<NFuture<TType>> GetFuture();
    
    // 设置结果
    void SetValue(const TType& Value);
    void SetValue(TType&& Value);
    void SetException(const CString& ExceptionMessage);
    void SetCancelled();
    
    // 状态查询
    bool IsSet() const;

private:
    TSharedPtr<NFuture<TType>> Future;
    bool bIsSet;
    
    // 禁止复制
    CPromise(const CPromise&) = delete;
    CPromise& operator=(const CPromise&) = delete;
};

/**
 * @brief void特化版本
 */
template<>
class LIBNLIB_API CPromise<void> : public CObject
{
public:
    CPromise();
    virtual ~CPromise();
    
    // 获取关联的Future
    TSharedPtr<NFuture<void>> GetFuture();
    
    // 设置结果
    void SetCompleted();
    void SetException(const CString& ExceptionMessage);
    void SetCancelled();
    
    // 状态查询
    bool IsSet() const;

private:
    TSharedPtr<NFuture<void>> Future;
    bool bIsSet;
    
    // 禁止复制
    CPromise(const CPromise&) = delete;
    CPromise& operator=(const CPromise&) = delete;
};

/**
 * @brief 延迟计算的Future
 */
template<typename TType>
class CLazyFuture : public NFuture<TType>
{

public:
    using ComputeFunction = NFunction<TType()>;
    
    explicit CLazyFuture(ComputeFunction Function);
    virtual ~CLazyFuture();
    
    // 重写获取方法以支持延迟计算
    virtual TType Get() override;
    virtual TType Get(int32_t TimeoutMs) override;

private:
    ComputeFunction Function;
    bool bIsComputed;
    
    void ComputeIfNeeded();
};

/**
 * @brief Future工厂类
 */
class LIBNLIB_API NFutureFactory
{
public:
    // 创建已完成的Future
    template<typename TType>
    static TSharedPtr<NFuture<TType>> CreateCompleted(const TType& Value);
    
    template<typename TType>
    static TSharedPtr<NFuture<TType>> CreateCompleted(TType&& Value);
    
    static TSharedPtr<NFuture<void>> CreateCompleted();
    
    // 创建失败的Future
    template<typename TType>
    static TSharedPtr<NFuture<TType>> CreateFaulted(const CString& ExceptionMessage);
    
    // 创建取消的Future
    template<typename TType>
    static TSharedPtr<NFuture<TType>> CreateCancelled();
    
    // 从异步任务创建Future
    template<typename TType>
    static TSharedPtr<NFuture<TType>> FromAsyncTask(TSharedPtr<NAsyncTask<TType>> Task);
    
    // 创建延迟Future
    template<typename TType>
    static TSharedPtr<CLazyFuture<TType>> CreateLazy(typename CLazyFuture<TType>::ComputeFunction Function);
    
    // 运行函数并返回Future
    template<typename TType>
    static TSharedPtr<NFuture<TType>> Run(NFunction<TType()> Function);
    
    template<typename TType>
    static TSharedPtr<NFuture<TType>> Run(NFunction<TType(NCancellationToken&)> Function);

private:
    NFutureFactory() = delete; // 静态工厂类
};

/**
 * @brief Future工具类
 */
class LIBNLIB_API NFutureUtils
{
public:
    // 等待所有Future完成
    template<typename TType>
    static CArray<TType> WaitAll(const CArray<TSharedPtr<NFuture<TType>>>& Futures);
    
    template<typename TType>
    static CArray<TType> WaitAll(const CArray<TSharedPtr<NFuture<TType>>>& Futures, int32_t TimeoutMs);
    
    // 等待任意一个Future完成
    template<typename TType>
    static TType WaitAny(const CArray<TSharedPtr<NFuture<TType>>>& Futures);
    
    template<typename TType>
    static TSharedPtr<NFuture<TType>> WaitAnyFuture(const CArray<TSharedPtr<NFuture<TType>>>& Futures);
    
    // 转换操作
    template<typename TFrom, typename TTo>
    static TSharedPtr<NFuture<TTo>> Transform(TSharedPtr<NFuture<TFrom>> Source, NFunction<TTo(TFrom)> Transformer);
    
    // 延迟操作
    template<typename TType>
    static TSharedPtr<NFuture<TType>> Delay(TSharedPtr<NFuture<TType>> Source, int32_t DelayMs);
    
    // 超时操作
    template<typename TType>
    static TSharedPtr<NFuture<TType>> Timeout(TSharedPtr<NFuture<TType>> Source, int32_t TimeoutMs);

private:
    NFutureUtils() = delete; // 静态工具类
};

// === 模板实现 ===

template<typename TType>
NFuture<TType>::NFuture()
    : State(EFutureState::Pending), bHasValue(false)
{}

template<typename TType>
NFuture<TType>::~NFuture()
{}

template<typename TType>
bool NFuture<TType>::IsReady() const
{
    CLockGuard<NMutex> Lock(StateMutex);
    return State != EFutureState::Pending;
}

template<typename TType>
bool NFuture<TType>::IsPending() const
{
    CLockGuard<NMutex> Lock(StateMutex);
    return State == EFutureState::Pending;
}

template<typename TType>
bool NFuture<TType>::IsCompleted() const
{
    CLockGuard<NMutex> Lock(StateMutex);
    return State == EFutureState::Completed;
}

template<typename TType>
bool NFuture<TType>::IsCancelled() const
{
    CLockGuard<NMutex> Lock(StateMutex);
    return State == EFutureState::Cancelled;
}

template<typename TType>
bool NFuture<TType>::IsFaulted() const
{
    CLockGuard<NMutex> Lock(StateMutex);
    return State == EFutureState::Faulted;
}

template<typename TType>
EFutureState NFuture<TType>::GetState() const
{
    CLockGuard<NMutex> Lock(StateMutex);
    return State;
}

template<typename TType>
TType NFuture<TType>::Get()
{
    Wait();
    
    CLockGuard<NMutex> Lock(StateMutex);
    if (State == EFutureState::Completed && bHasValue)
    {
        return Value;
    }
    else if (State == EFutureState::Faulted)
    {
        // 在实际实现中，这里应该抛出异常
        return TType{};
    }
    else if (State == EFutureState::Cancelled)
    {
        // 在实际实现中，这里应该抛出取消异常
        return TType{};
    }
    
    return TType{};
}

template<typename TType>
TType NFuture<TType>::Get(int32_t TimeoutMs)
{
    if (!Wait(TimeoutMs))
    {
        // 超时
        return TType{};
    }
    
    return Get();
}

template<typename TType>
bool NFuture<TType>::TryGet(TType& OutValue)
{
    CLockGuard<NMutex> Lock(StateMutex);
    if (State == EFutureState::Completed && bHasValue)
    {
        OutValue = Value;
        return true;
    }
    return false;
}

template<typename TType>
bool NFuture<TType>::TryGet(TType& OutValue, int32_t TimeoutMs)
{
    if (!Wait(TimeoutMs))
    {
        return false;
    }
    
    return TryGet(OutValue);
}

template<typename TType>
NAsyncResult<TType> NFuture<TType>::GetAsyncResult()
{
    CLockGuard<NMutex> Lock(StateMutex);
    
    if (State == EFutureState::Completed && bHasValue)
    {
        return NAsyncResult<TType>(Value);
    }
    else if (State == EFutureState::Faulted)
    {
        return NAsyncResult<TType>(ExceptionMessage);
    }
    else if (State == EFutureState::Cancelled)
    {
        return NAsyncResult<TType>("Future was cancelled");
    }
    
    return NAsyncResult<TType>("Future is not ready");
}

template<typename TType>
void NFuture<TType>::Wait()
{
    if (!IsReady())
    {
        CompletionEvent.Wait();
    }
}

template<typename TType>
bool NFuture<TType>::Wait(int32_t TimeoutMs)
{
    if (IsReady())
    {
        return true;
    }
    
    return CompletionEvent.WaitFor(TimeoutMs);
}

template<typename TType>
void NFuture<TType>::Cancel()
{
    SetCancelled();
}

template<typename TType>
CString NFuture<TType>::GetExceptionMessage() const
{
    CLockGuard<NMutex> Lock(StateMutex);
    return ExceptionMessage;
}

template<typename TType>
template<typename TResult>
TSharedPtr<NFuture<TResult>> NFuture<TType>::Then(NFunction<TResult(TType)> Continuation)
{
    auto ContinuationFuture = NewNObject<NFuture<TResult>>();
    
    // 如果已经完成，立即执行继续任务
    if (IsReady())
    {
        if (IsCompleted())
        {
            try
            {
                TResult Result = Continuation(Value);
                ContinuationFuture->SetValue(NLib::Move(Result));
            }
            catch (...)
            {
                ContinuationFuture->SetException("Exception in continuation");
            }
        }
        else
        {
            ContinuationFuture->SetException(ExceptionMessage);
        }
    }
    else
    {
        // 注册回调
        OnCompleted([ContinuationFuture, Continuation](const TType& Value) {
            try
            {
                TResult Result = Continuation(Value);
                ContinuationFuture->SetValue(NLib::Move(Result));
            }
            catch (...)
            {
                ContinuationFuture->SetException("Exception in continuation");
            }
        });
        
        OnFaulted([ContinuationFuture](const CString& ErrorMessage) {
            ContinuationFuture->SetException(ErrorMessage);
        });
        
        OnCancelled([ContinuationFuture]() {
            ContinuationFuture->SetCancelled();
        });
    }
    
    return ContinuationFuture;
}

template<typename TType>
template<typename TResult>
TSharedPtr<NFuture<TResult>> NFuture<TType>::Then(NFunction<TResult(const NFuture<TType>&)> Continuation)
{
    auto ContinuationFuture = NewNObject<NFuture<TResult>>();
    
    if (IsReady())
    {
        try
        {
            TResult Result = Continuation(*this);
            ContinuationFuture->SetValue(NLib::Move(Result));
        }
        catch (...)
        {
            ContinuationFuture->SetException("Exception in continuation");
        }
    }
    else
    {
        OnCompleted([this, ContinuationFuture, Continuation](const TType& Value) {
            try
            {
                TResult Result = Continuation(*this);
                ContinuationFuture->SetValue(NLib::Move(Result));
            }
            catch (...)
            {
                ContinuationFuture->SetException("Exception in continuation");
            }
        });
        
        OnFaulted([ContinuationFuture](const CString& ErrorMessage) {
            ContinuationFuture->SetException(ErrorMessage);
        });
        
        OnCancelled([ContinuationFuture]() {
            ContinuationFuture->SetCancelled();
        });
    }
    
    return ContinuationFuture;
}

template<typename TType>
TSharedPtr<NFuture<TType>> NFuture<TType>::FromValue(const TType& Value)
{
    auto Future = NewNObject<NFuture<TType>>();
    Future->SetValue(Value);
    return Future;
}

template<typename TType>
TSharedPtr<NFuture<TType>> NFuture<TType>::FromValue(TType&& Value)
{
    auto Future = NewNObject<NFuture<TType>>();
    Future->SetValue(NLib::Move(Value));
    return Future;
}

template<typename TType>
TSharedPtr<NFuture<TType>> NFuture<TType>::FromException(const CString& ExceptionMessage)
{
    auto Future = NewNObject<NFuture<TType>>();
    Future->SetException(ExceptionMessage);
    return Future;
}

template<typename TType>
TSharedPtr<NFuture<TType>> NFuture<TType>::FromAsyncTask(TSharedPtr<NAsyncTask<TType>> Task)
{
    auto Future = NewNObject<NFuture<TType>>();
    
    // 在后台线程中等待任务完成
    auto WaitThread = NewNObject<CThread>([Future, Task]() {
        try
        {
            TType Result = Task->GetResult();
            Future->SetValue(NLib::Move(Result));
        }
        catch (...)
        {
            Future->SetException("Exception in async task");
        }
    });
    WaitThread->Start();
    WaitThread->Detach();
    
    return Future;
}

template<typename TType>
void NFuture<TType>::OnCompleted(NFunction<void(const TType&)> Callback)
{
    CLockGuard<NMutex> Lock(StateMutex);
    
    if (State == EFutureState::Completed && bHasValue)
    {
        // 立即执行回调
        Callback(Value);
    }
    else if (State == EFutureState::Pending)
    {
        CompletedCallbacks.PushBack(Callback);
    }
}

template<typename TType>
void NFuture<TType>::OnFaulted(NFunction<void(const CString&)> Callback)
{
    CLockGuard<NMutex> Lock(StateMutex);
    
    if (State == EFutureState::Faulted)
    {
        Callback(ExceptionMessage);
    }
    else if (State == EFutureState::Pending)
    {
        FaultedCallbacks.PushBack(Callback);
    }
}

template<typename TType>
void NFuture<TType>::OnCancelled(NFunction<void()> Callback)
{
    CLockGuard<NMutex> Lock(StateMutex);
    
    if (State == EFutureState::Cancelled)
    {
        Callback();
    }
    else if (State == EFutureState::Pending)
    {
        CancelledCallbacks.PushBack(Callback);
    }
}

template<typename TType>
void NFuture<TType>::SetValue(const TType& InValue)
{
    CLockGuard<NMutex> Lock(StateMutex);
    
    if (State == EFutureState::Pending)
    {
        Value = InValue;
        bHasValue = true;
        State = EFutureState::Completed;
        CompletionEvent.Set();
        NotifyCallbacks();
    }
}

template<typename TType>
void NFuture<TType>::SetValue(TType&& InValue)
{
    CLockGuard<NMutex> Lock(StateMutex);
    
    if (State == EFutureState::Pending)
    {
        Value = NLib::Move(InValue);
        bHasValue = true;
        State = EFutureState::Completed;
        CompletionEvent.Set();
        NotifyCallbacks();
    }
}

template<typename TType>
void NFuture<TType>::SetException(const CString& InExceptionMessage)
{
    CLockGuard<NMutex> Lock(StateMutex);
    
    if (State == EFutureState::Pending)
    {
        ExceptionMessage = InExceptionMessage;
        State = EFutureState::Faulted;
        CompletionEvent.Set();
        NotifyCallbacks();
    }
}

template<typename TType>
void NFuture<TType>::SetCancelled()
{
    CLockGuard<NMutex> Lock(StateMutex);
    
    if (State == EFutureState::Pending)
    {
        State = EFutureState::Cancelled;
        CompletionEvent.Set();
        NotifyCallbacks();
    }
}

template<typename TType>
void NFuture<TType>::NotifyCallbacks()
{
    // 注意：调用此方法时StateMutex已被锁定
    
    if (State == EFutureState::Completed && bHasValue)
    {
        for (const auto& Callback : CompletedCallbacks)
        {
            try
            {
                Callback(Value);
            }
            catch (...)
            {
                // 忽略回调中的异常
            }
        }
    }
    else if (State == EFutureState::Faulted)
    {
        for (const auto& Callback : FaultedCallbacks)
        {
            try
            {
                Callback(ExceptionMessage);
            }
            catch (...)
            {
                // 忽略回调中的异常
            }
        }
    }
    else if (State == EFutureState::Cancelled)
    {
        for (const auto& Callback : CancelledCallbacks)
        {
            try
            {
                Callback();
            }
            catch (...)
            {
                // 忽略回调中的异常
            }
        }
    }
    
    // 清空回调列表
    CompletedCallbacks.Clear();
    FaultedCallbacks.Clear();
    CancelledCallbacks.Clear();
}

// Promise实现
template<typename TType>
CPromise<TType>::CPromise()
    : Future(NewNObject<NFuture<TType>>()), bIsSet(false)
{}

template<typename TType>
CPromise<TType>::~CPromise()
{
    if (!bIsSet && Future)
    {
        Future->SetCancelled();
    }
}

template<typename TType>
TSharedPtr<NFuture<TType>> CPromise<TType>::GetFuture()
{
    return Future;
}

template<typename TType>
void CPromise<TType>::SetValue(const TType& Value)
{
    if (!bIsSet && Future)
    {
        Future->SetValue(Value);
        bIsSet = true;
    }
}

template<typename TType>
void CPromise<TType>::SetValue(TType&& Value)
{
    if (!bIsSet && Future)
    {
        Future->SetValue(NLib::Move(Value));
        bIsSet = true;
    }
}

template<typename TType>
void CPromise<TType>::SetException(const CString& ExceptionMessage)
{
    if (!bIsSet && Future)
    {
        Future->SetException(ExceptionMessage);
        bIsSet = true;
    }
}

template<typename TType>
void CPromise<TType>::SetCancelled()
{
    if (!bIsSet && Future)
    {
        Future->SetCancelled();
        bIsSet = true;
    }
}

template<typename TType>
bool CPromise<TType>::IsSet() const
{
    return bIsSet;
}

// LazyFuture实现
template<typename TType>
CLazyFuture<TType>::CLazyFuture(ComputeFunction InFunction)
    : Function(InFunction), bIsComputed(false)
{}

template<typename TType>
CLazyFuture<TType>::~CLazyFuture()
{}

template<typename TType>
TType CLazyFuture<TType>::Get()
{
    ComputeIfNeeded();
    return NFuture<TType>::Get();
}

template<typename TType>
TType CLazyFuture<TType>::Get(int32_t TimeoutMs)
{
    ComputeIfNeeded();
    return NFuture<TType>::Get(TimeoutMs);
}

template<typename TType>
void CLazyFuture<TType>::ComputeIfNeeded()
{
    if (!bIsComputed && Function)
    {
        try
        {
            TType Result = Function();
            this->SetValue(NLib::Move(Result));
            bIsComputed = true;
        }
        catch (...)
        {
            this->SetException("Exception in lazy computation");
            bIsComputed = true;
        }
    }
}

} // namespace NLib
