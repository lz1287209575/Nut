#include "NFuture.h"

#include "Logging/CLogger.h"
#include "DateTime/NDateTime.h"

namespace NLib
{

// === NFuture<void> 特化实现 ===

NFuture<void>::NFuture()
    : State(EFutureState::Pending)
{}

NFuture<void>::~NFuture()
{}

bool NFuture<void>::IsReady() const
{
    CLockGuard<NMutex> Lock(StateMutex);
    return State != EFutureState::Pending;
}

bool NFuture<void>::IsPending() const
{
    CLockGuard<NMutex> Lock(StateMutex);
    return State == EFutureState::Pending;
}

bool NFuture<void>::IsCompleted() const
{
    CLockGuard<NMutex> Lock(StateMutex);
    return State == EFutureState::Completed;
}

bool NFuture<void>::IsCancelled() const
{
    CLockGuard<NMutex> Lock(StateMutex);
    return State == EFutureState::Cancelled;
}

bool NFuture<void>::IsFaulted() const
{
    CLockGuard<NMutex> Lock(StateMutex);
    return State == EFutureState::Faulted;
}

EFutureState NFuture<void>::GetState() const
{
    CLockGuard<NMutex> Lock(StateMutex);
    return State;
}

void NFuture<void>::Get()
{
    Wait();
    
    CLockGuard<NMutex> Lock(StateMutex);
    if (State == EFutureState::Faulted)
    {
        CLogger::Error("NFuture<void>: Future completed with exception: {}", ExceptionMessage);
        // 在实际实现中，这里应该抛出异常
    }
    else if (State == EFutureState::Cancelled)
    {
        CLogger::Warning("NFuture<void>: Future was cancelled");
        // 在实际实现中，这里应该抛出取消异常
    }
}

bool NFuture<void>::Get(int32_t TimeoutMs)
{
    if (!Wait(TimeoutMs))
    {
        return false; // 超时
    }
    
    Get();
    return true;
}

bool NFuture<void>::TryGet()
{
    CLockGuard<NMutex> Lock(StateMutex);
    return State == EFutureState::Completed;
}

bool NFuture<void>::TryGet(int32_t TimeoutMs)
{
    if (!Wait(TimeoutMs))
    {
        return false;
    }
    
    return TryGet();
}

void NFuture<void>::Wait()
{
    if (!IsReady())
    {
        CompletionEvent.Wait();
    }
}

bool NFuture<void>::Wait(int32_t TimeoutMs)
{
    if (IsReady())
    {
        return true;
    }
    
    return CompletionEvent.WaitFor(TimeoutMs);
}

void NFuture<void>::Cancel()
{
    SetCancelled();
}

CString NFuture<void>::GetExceptionMessage() const
{
    CLockGuard<NMutex> Lock(StateMutex);
    return ExceptionMessage;
}

template<typename TResult>
TSharedPtr<NFuture<TResult>> NFuture<void>::Then(NFunction<TResult()> Continuation)
{
    auto ContinuationFuture = NewNObject<NFuture<TResult>>();
    
    if (IsReady())
    {
        if (IsCompleted())
        {
            try
            {
                TResult Result = Continuation();
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
        OnCompleted([ContinuationFuture, Continuation]() {
            try
            {
                TResult Result = Continuation();
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

TSharedPtr<NFuture<void>> NFuture<void>::CompletedFuture()
{
    auto Future = NewNObject<NFuture<void>>();
    Future->SetCompleted();
    return Future;
}

TSharedPtr<NFuture<void>> NFuture<void>::FromException(const CString& ExceptionMessage)
{
    auto Future = NewNObject<NFuture<void>>();
    Future->SetException(ExceptionMessage);
    return Future;
}

TSharedPtr<NFuture<void>> NFuture<void>::FromAsyncTask(TSharedPtr<NAsyncTask<void>> Task)
{
    auto Future = NewNObject<NFuture<void>>();
    
    // 在后台线程中等待任务完成
    auto WaitThread = NewNObject<CThread>([Future, Task]() {
        try
        {
            Task->GetResult();
            Future->SetCompleted();
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

TSharedPtr<NFuture<void>> NFuture<void>::WhenAll(const CArray<TSharedPtr<NFuture<void>>>& Futures)
{
    auto CombinedFuture = NewNObject<NFuture<void>>();
    
    if (Futures.IsEmpty())
    {
        CombinedFuture->SetCompleted();
        return CombinedFuture;
    }
    
    // 在后台线程中等待所有Future完成
    auto WaitThread = NewNObject<CThread>([CombinedFuture, Futures]() {
        try
        {
            for (const auto& Future : Futures)
            {
                if (Future)
                {
                    Future->Wait();
                    
                    if (Future->IsFaulted())
                    {
                        CombinedFuture->SetException(Future->GetExceptionMessage());
                        return;
                    }
                    else if (Future->IsCancelled())
                    {
                        CombinedFuture->SetCancelled();
                        return;
                    }
                }
            }
            
            CombinedFuture->SetCompleted();
        }
        catch (...)
        {
            CombinedFuture->SetException("Exception in WhenAll");
        }
    });
    WaitThread->Start();
    WaitThread->Detach();
    
    return CombinedFuture;
}

void NFuture<void>::OnCompleted(NFunction<void()> Callback)
{
    CLockGuard<NMutex> Lock(StateMutex);
    
    if (State == EFutureState::Completed)
    {
        Callback();
    }
    else if (State == EFutureState::Pending)
    {
        CompletedCallbacks.PushBack(Callback);
    }
}

void NFuture<void>::OnFaulted(NFunction<void(const CString&)> Callback)
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

void NFuture<void>::OnCancelled(NFunction<void()> Callback)
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

void NFuture<void>::SetCompleted()
{
    CLockGuard<NMutex> Lock(StateMutex);
    
    if (State == EFutureState::Pending)
    {
        State = EFutureState::Completed;
        CompletionEvent.Set();
        NotifyCallbacks();
    }
}

void NFuture<void>::SetException(const CString& InExceptionMessage)
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

void NFuture<void>::SetCancelled()
{
    CLockGuard<NMutex> Lock(StateMutex);
    
    if (State == EFutureState::Pending)
    {
        State = EFutureState::Cancelled;
        CompletionEvent.Set();
        NotifyCallbacks();
    }
}

void NFuture<void>::NotifyCallbacks()
{
    // 注意：调用此方法时StateMutex已被锁定
    
    if (State == EFutureState::Completed)
    {
        for (const auto& Callback : CompletedCallbacks)
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

// === CPromise<void> 特化实现 ===

CPromise<void>::CPromise()
    : Future(NewNObject<NFuture<void>>()), bIsSet(false)
{}

CPromise<void>::~CPromise()
{
    if (!bIsSet && Future)
    {
        Future->SetCancelled();
    }
}

TSharedPtr<NFuture<void>> CPromise<void>::GetFuture()
{
    return Future;
}

void CPromise<void>::SetCompleted()
{
    if (!bIsSet && Future)
    {
        Future->SetCompleted();
        bIsSet = true;
    }
}

void CPromise<void>::SetException(const CString& ExceptionMessage)
{
    if (!bIsSet && Future)
    {
        Future->SetException(ExceptionMessage);
        bIsSet = true;
    }
}

void CPromise<void>::SetCancelled()
{
    if (!bIsSet && Future)
    {
        Future->SetCancelled();
        bIsSet = true;
    }
}

bool CPromise<void>::IsSet() const
{
    return bIsSet;
}

} // namespace NLib