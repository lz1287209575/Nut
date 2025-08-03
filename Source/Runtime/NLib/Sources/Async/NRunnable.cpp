#include "NRunnable.h"
#include "NRunnable.generate.h"

#include "Logging/CLogger.h"
#include "DateTime/NDateTime.h"

#include <chrono>

namespace NLib
{

// === NRunnable 实现 ===

NRunnable::NRunnable()
    : Name("Runnable"), bIsRunning(false), bStopRequested(false)
{}

NRunnable::NRunnable(const CString& InName)
    : Name(InName), bIsRunning(false), bStopRequested(false)
{}

NRunnable::~NRunnable()
{
    if (IsRunning())
    {
        Stop();
    }
}

void NRunnable::Stop()
{
    if (bIsRunning.Load())
    {
        bStopRequested.Store(true);
        CLogger::Debug("NRunnable: Stop requested for '{}'", Name);
    }
}

bool NRunnable::Initialize()
{
    CLogger::Debug("NRunnable: Initializing '{}'", Name);
    return true;
}

void NRunnable::Shutdown()
{
    bIsRunning.Store(false);
    bStopRequested.Store(false);
    CLogger::Debug("NRunnable: Shutdown '{}'", Name);
}

bool NRunnable::IsRunning() const
{
    return bIsRunning.Load();
}

CString NRunnable::GetName() const
{
    return Name;
}

void NRunnable::SetName(const CString& InName)
{
    Name = InName;
}

bool NRunnable::ShouldStop() const
{
    return bStopRequested.Load();
}

void NRunnable::SetRunning(bool InRunning)
{
    bool WasRunning = bIsRunning.Exchange(InRunning);
    
    if (InRunning && !WasRunning)
    {
        OnRunnableStarted();
        OnStarted.Broadcast(this);
        CLogger::Debug("NRunnable: Started '{}'", Name);
    }
    else if (!InRunning && WasRunning)
    {
        OnRunnableStopped();
        OnStopped.Broadcast(this);
        CLogger::Debug("NRunnable: Stopped '{}'", Name);
    }
}

// === NFunctionRunnable 实现 ===

NFunctionRunnable::NFunctionRunnable(RunnableFunction Function)
    : NRunnable(), SimpleFunction(Function), bIsStopAware(false)
{}

NFunctionRunnable::NFunctionRunnable(RunnableFunctionWithStop Function)
    : NRunnable(), StopAwareFunction(Function), bIsStopAware(true)
{}

NFunctionRunnable::NFunctionRunnable(RunnableFunction Function, const CString& Name)
    : NRunnable(Name), SimpleFunction(Function), bIsStopAware(false)
{}

NFunctionRunnable::NFunctionRunnable(RunnableFunctionWithStop Function, const CString& Name)
    : NRunnable(Name), StopAwareFunction(Function), bIsStopAware(true)
{}

void NFunctionRunnable::Run()
{
    if (!Initialize())
    {
        CLogger::Error("NFunctionRunnable: Failed to initialize '{}'", GetName());
        return;
    }
    
    SetRunning(true);
    
    try
    {
        if (bIsStopAware && StopAwareFunction)
        {
            // 提供停止检查函数
            StopAwareFunction([this]() -> bool {
                return ShouldStop();
            });
        }
        else if (!bIsStopAware && SimpleFunction)
        {
            SimpleFunction();
        }
        else
        {
            CLogger::Error("NFunctionRunnable: No valid function to execute in '{}'", GetName());
        }
    }
    catch (...)
    {
        CString ErrorMsg = "Exception occurred in function runnable";
        OnRunnableError(ErrorMsg);
        OnError.Broadcast(this, ErrorMsg);
        CLogger::Error("NFunctionRunnable: Exception in '{}'", GetName());
    }
    
    SetRunning(false);
    Shutdown();
}

// === NRunnableTask 实现 ===

NRunnableTask::NRunnableTask(TSharedPtr<IRunnable> Runnable)
    : RunnableObject(Runnable)
{}

NRunnableTask::~NRunnableTask()
{
    if (IsRunning() && ExecutionThread)
    {
        Cancel();
        ExecutionThread->Join();
    }
}

void NRunnableTask::Start()
{
    if (GetState() != EAsyncTaskState::Created || !RunnableObject)
    {
        return;
    }
    
    SetState(EAsyncTaskState::Running);
    
    // 在独立线程中执行可运行对象
    ExecutionThread = NewNObject<CThread>(RunnableThreadEntry, this);
    ExecutionThread->Start();
}

void NRunnableTask::Cancel()
{
    NAsyncTask<void>::Cancel();
    
    if (RunnableObject)
    {
        RunnableObject->Stop();
    }
}

TSharedPtr<IRunnable> NRunnableTask::GetRunnable() const
{
    return RunnableObject;
}

TSharedPtr<NRunnableTask> NRunnableTask::Create(TSharedPtr<IRunnable> Runnable)
{
    return NewNObject<NRunnableTask>(Runnable);
}

TSharedPtr<NRunnableTask> NRunnableTask::Create(NFunctionRunnable::RunnableFunction Function)
{
    auto FuncRunnable = NewNObject<NFunctionRunnable>(Function);
    return NewNObject<NRunnableTask>(FuncRunnable);
}

TSharedPtr<NRunnableTask> NRunnableTask::Create(NFunctionRunnable::RunnableFunction Function, const CString& Name)
{
    auto FuncRunnable = NewNObject<NFunctionRunnable>(Function, Name);
    return NewNObject<NRunnableTask>(FuncRunnable);
}

void NRunnableTask::ExecuteRunnable()
{
    try
    {
        if (RunnableObject)
        {
            RunnableObject->Run();
            SetState(EAsyncTaskState::Completed);
        }
        else
        {
            SetException("Runnable object is null");
            SetState(EAsyncTaskState::Faulted);
        }
    }
    catch (...)
    {
        SetException("Exception occurred during runnable execution");
        SetState(EAsyncTaskState::Faulted);
    }
}

void NRunnableTask::RunnableThreadEntry(void* TaskPtr)
{
    auto* Task = static_cast<NRunnableTask*>(TaskPtr);
    if (Task)
    {
        Task->ExecuteRunnable();
    }
}

// === NPeriodicRunnable 实现 ===

NPeriodicRunnable::NPeriodicRunnable(PeriodicFunction InFunction, int32_t InIntervalMs)
    : NRunnable("PeriodicRunnable"), Function(InFunction), IntervalMs(InIntervalMs), 
      bImmediateStart(false), ExecutionCount(0), LastExecutionTime(0), TotalExecutionTime(0.0)
{}

NPeriodicRunnable::NPeriodicRunnable(PeriodicFunction InFunction, int32_t InIntervalMs, const CString& Name)
    : NRunnable(Name), Function(InFunction), IntervalMs(InIntervalMs),
      bImmediateStart(false), ExecutionCount(0), LastExecutionTime(0), TotalExecutionTime(0.0)
{}

NPeriodicRunnable::~NPeriodicRunnable()
{
    Stop();
}

void NPeriodicRunnable::Run()
{
    if (!Initialize())
    {
        return;
    }
    
    SetRunning(true);
    
    if (bImmediateStart)
    {
        // 立即执行一次
        if (Function && !ShouldStop())
        {
            int64_t StartTime = GetCurrentTimeMs();
            try
            {
                Function();
                ExecutionCount++;
                LastExecutionTime.Store(GetCurrentTimeMs());
                TotalExecutionTime.Store(TotalExecutionTime.Load() + (LastExecutionTime.Load() - StartTime));
            }
            catch (...)
            {
                CString ErrorMsg = "Exception in periodic function";
                OnRunnableError(ErrorMsg);
                OnError.Broadcast(this, ErrorMsg);
            }
        }
    }
    
    while (!ShouldStop())
    {
        // 等待间隔时间
        int64_t SleepStart = GetCurrentTimeMs();
        while (!ShouldStop() && (GetCurrentTimeMs() - SleepStart) < IntervalMs)
        {
            CThread::Sleep(10); // 每10毫秒检查一次停止条件
        }
        
        if (ShouldStop())
        {
            break;
        }
        
        // 执行函数
        if (Function)
        {
            int64_t StartTime = GetCurrentTimeMs();
            try
            {
                Function();
                ExecutionCount++;
                LastExecutionTime.Store(GetCurrentTimeMs());
                TotalExecutionTime.Store(TotalExecutionTime.Load() + (LastExecutionTime.Load() - StartTime));
            }
            catch (...)
            {
                CString ErrorMsg = "Exception in periodic function";
                OnRunnableError(ErrorMsg);
                OnError.Broadcast(this, ErrorMsg);
            }
        }
    }
    
    SetRunning(false);
    Shutdown();
}

void NPeriodicRunnable::Stop()
{
    NRunnable::Stop();
}

void NPeriodicRunnable::SetInterval(int32_t InIntervalMs)
{
    IntervalMs = InIntervalMs;
}

int32_t NPeriodicRunnable::GetInterval() const
{
    return IntervalMs;
}

void NPeriodicRunnable::SetImmediateStart(bool bImmediate)
{
    bImmediateStart = bImmediate;
}

bool NPeriodicRunnable::GetImmediateStart() const
{
    return bImmediateStart;
}

uint64_t NPeriodicRunnable::GetExecutionCount() const
{
    return ExecutionCount.Load();
}

int64_t NPeriodicRunnable::GetLastExecutionTime() const
{
    return LastExecutionTime.Load();
}

double NPeriodicRunnable::GetAverageExecutionTime() const
{
    uint64_t Count = ExecutionCount.Load();
    if (Count == 0)
    {
        return 0.0;
    }
    return TotalExecutionTime.Load() / static_cast<double>(Count);
}

int64_t NPeriodicRunnable::GetCurrentTimeMs() const
{
    auto Now = std::chrono::steady_clock::now();
    auto Duration = Now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(Duration).count();
}

// === NRunnablePool 实现 ===

NRunnablePool::NRunnablePool()
    : MaxConcurrency(CThread::GetHardwareConcurrency()), bIsRunning(false), ActiveCount(0), CompletedCount(0)
{}

NRunnablePool::NRunnablePool(uint32_t InMaxConcurrency)
    : MaxConcurrency(InMaxConcurrency), bIsRunning(false), ActiveCount(0), CompletedCount(0)
{}

NRunnablePool::~NRunnablePool()
{
    Stop();
}

void NRunnablePool::Start()
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
    
    CLogger::Info("NRunnablePool: Started with {} worker threads", MaxConcurrency);
}

void NRunnablePool::Stop()
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
    
    // 清空队列
    CLockGuard<NMutex> Lock(QueueMutex);
    RunnableQueue.Clear();
    
    CLogger::Info("NRunnablePool: Stopped");
}

void NRunnablePool::StopGracefully(int32_t TimeoutMs)
{
    if (!bIsRunning.Load())
    {
        return;
    }
    
    // 停止接受新任务
    bIsRunning.Store(false);
    
    // 等待当前任务完成
    if (!WaitForAll(TimeoutMs))
    {
        CLogger::Warning("NRunnablePool: Graceful shutdown timeout, forcing stop");
    }
    
    Stop();
}

void NRunnablePool::Submit(TSharedPtr<IRunnable> Runnable)
{
    if (!Runnable || !bIsRunning.Load())
    {
        return;
    }
    
    CLockGuard<NMutex> Lock(QueueMutex);
    RunnableQueue.PushBack(RunnableEntry(Runnable));
    QueueCondition.NotifyOne();
}

void NRunnablePool::Submit(NFunctionRunnable::RunnableFunction Function)
{
    auto FuncRunnable = NewNObject<NFunctionRunnable>(Function);
    Submit(FuncRunnable);
}

void NRunnablePool::Submit(NFunctionRunnable::RunnableFunction Function, const CString& Name)
{
    auto FuncRunnable = NewNObject<NFunctionRunnable>(Function, Name);
    Submit(FuncRunnable);
}

bool NRunnablePool::IsRunning() const
{
    return bIsRunning.Load();
}

uint32_t NRunnablePool::GetActiveCount() const
{
    return ActiveCount.Load();
}

uint32_t NRunnablePool::GetPendingCount() const
{
    CLockGuard<NMutex> Lock(QueueMutex);
    return static_cast<uint32_t>(RunnableQueue.GetSize());
}

uint32_t NRunnablePool::GetCompletedCount() const
{
    return CompletedCount.Load();
}

uint32_t NRunnablePool::GetMaxConcurrency() const
{
    return MaxConcurrency;
}

void NRunnablePool::WaitForAll()
{
    while (GetActiveCount() > 0 || GetPendingCount() > 0)
    {
        CThread::Sleep(10);
    }
}

bool NRunnablePool::WaitForAll(int32_t TimeoutMs)
{
    NStopwatch Stopwatch;
    Stopwatch.Start();
    
    while (GetActiveCount() > 0 || GetPendingCount() > 0)
    {
        if (Stopwatch.GetElapsedMilliseconds() >= TimeoutMs)
        {
            return false;
        }
        CThread::Sleep(10);
    }
    
    return true;
}

void NRunnablePool::WorkerThreadMain()
{
    while (bIsRunning.Load() || !RunnableQueue.IsEmpty())
    {
        try
        {
            RunnableEntry Entry = GetNextRunnable();
            if (Entry.Runnable)
            {
                ExecuteRunnable(Entry);
            }
        }
        catch (...)
        {
            CLogger::Error("NRunnablePool: Exception in worker thread");
        }
    }
}

NRunnablePool::RunnableEntry NRunnablePool::GetNextRunnable()
{
    CLockGuard<NMutex> Lock(QueueMutex);
    
    while (RunnableQueue.IsEmpty() && bIsRunning.Load())
    {
        QueueCondition.Wait(QueueMutex);
    }
    
    if (RunnableQueue.IsEmpty())
    {
        return RunnableEntry(nullptr);
    }
    
    RunnableEntry Entry = RunnableQueue[0];
    RunnableQueue.Erase(RunnableQueue.Begin());
    
    return Entry;
}

void NRunnablePool::ExecuteRunnable(const RunnableEntry& Entry)
{
    if (!Entry.Runnable)
    {
        return;
    }
    
    ActiveCount++;
    OnRunnableStarted.Broadcast(Entry.Runnable);
    
    try
    {
        Entry.Runnable->Run();
        OnRunnableCompleted.Broadcast(Entry.Runnable);
    }
    catch (...)
    {
        CString ErrorMsg = "Exception in runnable execution";
        OnRunnableError.Broadcast(Entry.Runnable, ErrorMsg);
        CLogger::Error("NRunnablePool: Exception executing runnable '{}'", Entry.Runnable->GetName());
    }
    
    ActiveCount--;
    CompletedCount++;
}

int64_t NRunnablePool::RunnableEntry::GetCurrentTimeMs()
{
    auto Now = std::chrono::steady_clock::now();
    auto Duration = Now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(Duration).count();
}

// === NRunnableFactory 实现 ===

TSharedPtr<NFunctionRunnable> NRunnableFactory::CreateFunction(NFunctionRunnable::RunnableFunction Function)
{
    return NewNObject<NFunctionRunnable>(Function);
}

TSharedPtr<NFunctionRunnable> NRunnableFactory::CreateFunction(NFunctionRunnable::RunnableFunction Function, const CString& Name)
{
    return NewNObject<NFunctionRunnable>(Function, Name);
}

TSharedPtr<NFunctionRunnable> NRunnableFactory::CreateStopAware(NFunctionRunnable::RunnableFunctionWithStop Function)
{
    return NewNObject<NFunctionRunnable>(Function);
}

TSharedPtr<NFunctionRunnable> NRunnableFactory::CreateStopAware(NFunctionRunnable::RunnableFunctionWithStop Function, const CString& Name)
{
    return NewNObject<NFunctionRunnable>(Function, Name);
}

TSharedPtr<NPeriodicRunnable> NRunnableFactory::CreatePeriodic(NPeriodicRunnable::PeriodicFunction Function, int32_t IntervalMs)
{
    return NewNObject<NPeriodicRunnable>(Function, IntervalMs);
}

TSharedPtr<NPeriodicRunnable> NRunnableFactory::CreatePeriodic(NPeriodicRunnable::PeriodicFunction Function, int32_t IntervalMs, const CString& Name)
{
    return NewNObject<NPeriodicRunnable>(Function, IntervalMs, Name);
}

TSharedPtr<NRunnableTask> NRunnableFactory::CreateTask(TSharedPtr<IRunnable> Runnable)
{
    return NewNObject<NRunnableTask>(Runnable);
}

TSharedPtr<NRunnableTask> NRunnableFactory::CreateTask(NFunctionRunnable::RunnableFunction Function)
{
    return NRunnableTask::Create(Function);
}

TSharedPtr<NRunnableTask> NRunnableFactory::CreateTask(NFunctionRunnable::RunnableFunction Function, const CString& Name)
{
    return NRunnableTask::Create(Function, Name);
}

// === NRunnableManager 实现 ===

NRunnablePool& NRunnableManager::GetDefaultPool()
{
    static NRunnablePool DefaultPool;
    static bool bInitialized = false;
    
    if (!bInitialized)
    {
        DefaultPool.Start();
        bInitialized = true;
    }
    
    return DefaultPool;
}

NRunnablePool& NRunnableManager::GetBackgroundPool()
{
    static NRunnablePool BackgroundPool(2); // 使用较少的线程
    static bool bInitialized = false;
    
    if (!bInitialized)
    {
        BackgroundPool.Start();
        bInitialized = true;
    }
    
    return BackgroundPool;
}

void NRunnableManager::RunInBackground(NFunctionRunnable::RunnableFunction Function)
{
    GetBackgroundPool().Submit(Function);
}

void NRunnableManager::RunInBackground(NFunctionRunnable::RunnableFunction Function, const CString& Name)
{
    GetBackgroundPool().Submit(Function, Name);
}

TSharedPtr<NRunnableTask> NRunnableManager::RunAsync(NFunctionRunnable::RunnableFunction Function)
{
    return NRunnableTask::Create(Function);
}

TSharedPtr<NRunnableTask> NRunnableManager::RunAsync(NFunctionRunnable::RunnableFunction Function, const CString& Name)
{
    return NRunnableTask::Create(Function, Name);
}

void NRunnableManager::Shutdown()
{
    // 这里应该停止所有全局池，但需要小心处理静态对象的析构顺序
    CLogger::Info("NRunnableManager: Shutdown requested");
}

} // namespace NLib