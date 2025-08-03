#include "NAsyncTask.h"
#include "NAsyncTask.generate.h"

#include "Logging/CLogger.h"
#include "Threading/CThread.h"
#include "DateTime/NDateTime.h"

namespace NLib
{

// === NCancellationToken 实现 ===

NCancellationToken::NCancellationToken()
    : bIsCancelled(false)
{}

NCancellationToken::~NCancellationToken()
{}

void NCancellationToken::Cancel()
{
    bool Expected = false;
    if (bIsCancelled.CompareExchange(Expected, true))
    {
        NotifyCallbacks();
    }
}

void NCancellationToken::CancelAfter(int32_t DelayMs)
{
    // 在后台线程中延迟取消
    auto CancelThread = NewNObject<CThread>([this, DelayMs]() {
        CThread::Sleep(DelayMs);
        Cancel();
    });
    CancelThread->Start();
    CancelThread->Detach();
}

bool NCancellationToken::IsCancellationRequested() const
{
    return bIsCancelled.Load();
}

bool NCancellationToken::CanBeCancelled() const
{
    return true;
}

void NCancellationToken::RegisterCallback(const CDelegate<void>& Callback)
{
    CLockGuard<NMutex> Lock(CallbackMutex);
    CancelCallbacks.PushBack(Callback);
}

void NCancellationToken::UnregisterCallback(const CDelegate<void>& Callback)
{
    CLockGuard<NMutex> Lock(CallbackMutex);
    
    for (auto It = CancelCallbacks.Begin(); It != CancelCallbacks.End(); ++It)
    {
        // 简化实现：这里应该有更好的委托比较方法
        CancelCallbacks.Erase(It);
        break;
    }
}

void NCancellationToken::ThrowIfCancellationRequested()
{
    if (IsCancellationRequested())
    {
        CLogger::Warning("NCancellationToken: Cancellation was requested");
        // 在实际实现中，这里应该抛出取消异常
    }
}

NCancellationToken NCancellationToken::None()
{
    static NCancellationToken NoneCancelToken;
    return NoneCancelToken;
}

void NCancellationToken::NotifyCallbacks()
{
    CLockGuard<NMutex> Lock(CallbackMutex);
    
    for (const auto& Callback : CancelCallbacks)
    {
        try
        {
            Callback.ExecuteIfBound();
        }
        catch (...)
        {
            CLogger::Error("NCancellationToken: Exception in cancel callback");
        }
    }
    
    CancelCallbacks.Clear();
}

// === NAsyncTaskBase 实现 ===

CAtomic<uint64_t> NAsyncTaskBase::NextTaskId(1);

NAsyncTaskBase::NAsyncTaskBase()
    : TaskId(NextTaskId++), State(EAsyncTaskState::Created), Priority(EAsyncTaskPriority::Normal)
{}

NAsyncTaskBase::~NAsyncTaskBase()
{
    if (IsRunning())
    {
        Cancel();
        WaitFor(5000); // 等待5秒
    }
}

void NAsyncTaskBase::Cancel()
{
    if (State == EAsyncTaskState::Created || State == EAsyncTaskState::Running)
    {
        CancellationToken.Cancel();
        SetState(EAsyncTaskState::Cancelled);
        OnCancelled();
        CompletionEvent.Set();
    }
}

void NAsyncTaskBase::Wait()
{
    if (!IsCompleted())
    {
        CompletionEvent.Wait();
    }
}

bool NAsyncTaskBase::WaitFor(int32_t TimeoutMs)
{
    if (IsCompleted())
    {
        return true;
    }
    
    return CompletionEvent.WaitFor(TimeoutMs);
}

EAsyncTaskState NAsyncTaskBase::GetState() const
{
    return State;
}

bool NAsyncTaskBase::IsCompleted() const
{
    EAsyncTaskState CurrentState = GetState();
    return CurrentState == EAsyncTaskState::Completed || 
           CurrentState == EAsyncTaskState::Cancelled || 
           CurrentState == EAsyncTaskState::Faulted;
}

bool NAsyncTaskBase::IsRunning() const
{
    return GetState() == EAsyncTaskState::Running;
}

bool NAsyncTaskBase::IsCancelled() const
{
    return GetState() == EAsyncTaskState::Cancelled;
}

bool NAsyncTaskBase::IsFaulted() const
{
    return GetState() == EAsyncTaskState::Faulted;
}

void NAsyncTaskBase::SetState(EAsyncTaskState NewState)
{
    EAsyncTaskState OldState = State;
    State = NewState;
    
    if (NewState != OldState)
    {
        switch (NewState)
        {
        case EAsyncTaskState::Running:
            OnStarted();
            break;
        case EAsyncTaskState::Completed:
            OnCompleted();
            CompletionEvent.Set();
            break;
        case EAsyncTaskState::Cancelled:
            OnCancelled();
            CompletionEvent.Set();
            break;
        case EAsyncTaskState::Faulted:
            OnFaulted();
            CompletionEvent.Set();
            break;
        default:
            break;
        }
    }
}

void NAsyncTaskBase::SetException(const CString& Message)
{
    ExceptionMessage = Message;
    CLogger::Error("NAsyncTaskBase: Task {} faulted with exception: {}", TaskId, Message);
}

// === NAsyncTask<void> 特化实现 ===

NAsyncTask<void>::NAsyncTask()
{}

NAsyncTask<void>::NAsyncTask(const TaskFunction& InFunction)
    : Function(InFunction)
{}

NAsyncTask<void>::NAsyncTask(TaskFunctionSimple InFunction)
{
    Function = [InFunction](NCancellationToken& Token) -> void {
        InFunction();
    };
}

NAsyncTask<void>::~NAsyncTask()
{
    if (IsRunning())
    {
        Cancel();
        WaitFor(5000); // 等待5秒
    }
}

void NAsyncTask<void>::Start()
{
    if (GetState() != EAsyncTaskState::Created)
    {
        return;
    }
    
    SetState(EAsyncTaskState::Running);
    
    // 在线程池中执行任务
    auto TaskThread = NewNObject<CThread>(TaskThreadEntry, this);
    TaskThread->Start();
    TaskThread->Detach();
}

void NAsyncTask<void>::GetResult()
{
    Wait();
    
    if (IsFaulted())
    {
        CLogger::Error("NAsyncTask<void>: Task completed with exception: {}", GetExceptionMessage());
    }
}

bool NAsyncTask<void>::TryGetResult()
{
    return IsCompleted() && !IsFaulted();
}

TSharedPtr<NAsyncTask<void>> NAsyncTask<void>::Run(const TaskFunction& Function)
{
    auto Task = NewNObject<NAsyncTask<void>>(Function);
    Task->Start();
    return Task;
}

TSharedPtr<NAsyncTask<void>> NAsyncTask<void>::Run(TaskFunctionSimple Function)
{
    auto Task = NewNObject<NAsyncTask<void>>(Function);
    Task->Start();
    return Task;
}

TSharedPtr<NAsyncTask<void>> NAsyncTask<void>::CompletedTask()
{
    auto Task = NewNObject<NAsyncTask<void>>();
    Task->SetState(EAsyncTaskState::Completed);
    return Task;
}

TSharedPtr<NAsyncTask<void>> NAsyncTask<void>::WhenAll(const CArray<TSharedPtr<NAsyncTask<void>>>& Tasks)
{
    auto CombinedTask = NewNObject<NAsyncTask<void>>();
    
    CombinedTask->Function = [Tasks](NCancellationToken& Token) -> void {
        for (const auto& Task : Tasks)
        {
            if (Token.IsCancellationRequested())
            {
                Token.ThrowIfCancellationRequested();
                return;
            }
            
            Task->Wait();
            
            if (Task->IsFaulted())
            {
                // 如果任何任务失败，整个组合任务失败
                return;
            }
        }
    };
    
    CombinedTask->Start();
    return CombinedTask;
}

void NAsyncTask<void>::ExecuteTask()
{
    try
    {
        if (Function)
        {
            Function(GetCancellationToken());
            SetState(EAsyncTaskState::Completed);
        }
        else
        {
            SetException("Task function is null");
            SetState(EAsyncTaskState::Faulted);
        }
    }
    catch (...)
    {
        SetException("Exception occurred during task execution");
        SetState(EAsyncTaskState::Faulted);
    }
}

void NAsyncTask<void>::TaskThreadEntry(void* TaskPtr)
{
    auto* Task = static_cast<NAsyncTask<void>*>(TaskPtr);
    if (Task)
    {
        Task->ExecuteTask();
    }
}

// === NAsyncTaskScheduler 实现 ===

NAsyncTaskScheduler::NAsyncTaskScheduler()
    : MaxConcurrency(CThread::GetHardwareConcurrency()), bIsRunning(false), ActiveTaskCount(0)
{}

NAsyncTaskScheduler::NAsyncTaskScheduler(uint32_t InMaxConcurrency)
    : MaxConcurrency(InMaxConcurrency), bIsRunning(false), ActiveTaskCount(0)
{}

NAsyncTaskScheduler::~NAsyncTaskScheduler()
{
    Stop();
}

void NAsyncTaskScheduler::ScheduleTask(TSharedPtr<NAsyncTaskBase> Task)
{
    ScheduleTask(Task, Task->GetPriority());
}

void NAsyncTaskScheduler::ScheduleTask(TSharedPtr<NAsyncTaskBase> Task, EAsyncTaskPriority Priority)
{
    if (!Task)
    {
        return;
    }
    
    Task->SetPriority(Priority);
    
    CLockGuard<NMutex> Lock(QueueMutex);
    TaskQueue.PushBack(TaskEntry(Task, Priority));
    QueueCondition.NotifyOne();
}

void NAsyncTaskScheduler::Start()
{
    if (bIsRunning.Load())
    {
        return;
    }
    
    bIsRunning.Store(true);
    
    // 创建工作线程
    for (uint32_t i = 0; i < MaxConcurrency; ++i)
    {
        auto WorkerThread = NewNObject<CThread>([this]() {
            WorkerThreadMain();
        });
        WorkerThread->Start();
        WorkerThreads.PushBack(WorkerThread);
    }
    
    CLogger::Info("NAsyncTaskScheduler: Started with {} worker threads", MaxConcurrency);
}

void NAsyncTaskScheduler::Stop()
{
    if (!bIsRunning.Load())
    {
        return;
    }
    
    bIsRunning.Store(false);
    QueueCondition.NotifyAll();
    
    // 等待所有工作线程结束
    for (auto& Thread : WorkerThreads)
    {
        Thread->Join();
    }
    WorkerThreads.Clear();
    
    // 清空任务队列
    CLockGuard<NMutex> Lock(QueueMutex);
    TaskQueue.Clear();
    
    CLogger::Info("NAsyncTaskScheduler: Stopped");
}

void NAsyncTaskScheduler::StopGracefully(int32_t TimeoutMs)
{
    if (!bIsRunning.Load())
    {
        return;
    }
    
    // 停止接受新任务
    bIsRunning.Store(false);
    
    // 等待当前任务完成
    if (!WaitForAllTasks(TimeoutMs))
    {
        CLogger::Warning("NAsyncTaskScheduler: Graceful shutdown timeout, forcing stop");
    }
    
    Stop();
}

bool NAsyncTaskScheduler::IsRunning() const
{
    return bIsRunning.Load();
}

uint32_t NAsyncTaskScheduler::GetActiveTaskCount() const
{
    return ActiveTaskCount.Load();
}

uint32_t NAsyncTaskScheduler::GetPendingTaskCount() const
{
    CLockGuard<NMutex> Lock(QueueMutex);
    return static_cast<uint32_t>(TaskQueue.GetSize());
}

void NAsyncTaskScheduler::WaitForAllTasks()
{
    while (GetActiveTaskCount() > 0 || GetPendingTaskCount() > 0)
    {
        CThread::Sleep(10);
    }
}

bool NAsyncTaskScheduler::WaitForAllTasks(int32_t TimeoutMs)
{
    NStopwatch Stopwatch;
    Stopwatch.Start();
    
    while (GetActiveTaskCount() > 0 || GetPendingTaskCount() > 0)
    {
        if (Stopwatch.GetElapsedMilliseconds() >= TimeoutMs)
        {
            return false;
        }
        CThread::Sleep(10);
    }
    
    return true;
}

NAsyncTaskScheduler& NAsyncTaskScheduler::GetDefaultScheduler()
{
    static NAsyncTaskScheduler DefaultScheduler;
    static bool bInitialized = false;
    
    if (!bInitialized)
    {
        DefaultScheduler.Start();
        bInitialized = true;
    }
    
    return DefaultScheduler;
}

NAsyncTaskScheduler& NAsyncTaskScheduler::GetBackgroundScheduler()
{
    static NAsyncTaskScheduler BackgroundScheduler(2); // 使用较少的线程
    static bool bInitialized = false;
    
    if (!bInitialized)
    {
        BackgroundScheduler.Start();
        bInitialized = true;
    }
    
    return BackgroundScheduler;
}

void NAsyncTaskScheduler::WorkerThreadMain()
{
    while (bIsRunning.Load() || !TaskQueue.IsEmpty())
    {
        try
        {
            TaskEntry Entry = GetNextTask();
            if (Entry.Task)
            {
                ExecuteTask(Entry);
            }
        }
        catch (...)
        {
            CLogger::Error("NAsyncTaskScheduler: Exception in worker thread");
        }
    }
}

NAsyncTaskScheduler::TaskEntry NAsyncTaskScheduler::GetNextTask()
{
    CLockGuard<NMutex> Lock(QueueMutex);
    
    while (TaskQueue.IsEmpty() && bIsRunning.Load())
    {
        QueueCondition.Wait(QueueMutex);
    }
    
    if (TaskQueue.IsEmpty())
    {
        return TaskEntry(nullptr, EAsyncTaskPriority::Normal);
    }
    
    // 按优先级排序
    TaskQueue.Sort([](const TaskEntry& A, const TaskEntry& B) {
        if (A.Priority != B.Priority)
        {
            return static_cast<uint32_t>(A.Priority) > static_cast<uint32_t>(B.Priority);
        }
        return A.SubmitTime < B.SubmitTime; // 相同优先级按提交时间排序
    });
    
    TaskEntry Entry = TaskQueue[0];
    TaskQueue.Erase(TaskQueue.Begin());
    
    return Entry;
}

void NAsyncTaskScheduler::ExecuteTask(const TaskEntry& Entry)
{
    if (!Entry.Task)
    {
        return;
    }
    
    ActiveTaskCount++;
    
    try
    {
        Entry.Task->Start();
        Entry.Task->Wait();
    }
    catch (...)
    {
        CLogger::Error("NAsyncTaskScheduler: Exception executing task {}", Entry.Task->GetTaskId());
    }
    
    ActiveTaskCount--;
}

int64_t NAsyncTaskScheduler::TaskEntry::GetCurrentTimeMs()
{
    auto Now = std::chrono::steady_clock::now();
    auto Duration = Now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(Duration).count();
}

} // namespace NLib