#pragma once

#include "Core/CObject.h"
#include "Containers/CString.h"
#include "Containers/CArray.h"
#include "Threading/CThread.h"
#include "Delegates/CDelegate.h"
#include "DateTime/NDateTime.h"

namespace NLib
{

/**
 * @brief 异步任务状态枚举
 */
enum class EAsyncTaskState : uint32_t
{
    Created,        // 已创建
    Running,        // 运行中
    Completed,      // 已完成
    Cancelled,      // 已取消
    Faulted        // 执行失败
};

/**
 * @brief 异步任务优先级
 */
enum class EAsyncTaskPriority : uint32_t
{
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

/**
 * @brief 任务取消令牌
 */
class LIBNLIB_API NCancellationToken : public CObject
{
    NCLASS()
class NCancellationToken : public NObject
{
    GENERATED_BODY()

public:
    NCancellationToken();
    virtual ~NCancellationToken();

    // 取消操作
    void Cancel();
    void CancelAfter(int32_t DelayMs);
    
    // 状态查询
    bool IsCancellationRequested() const;
    bool CanBeCancelled() const;
    
    // 取消回调
    void RegisterCallback(const CDelegate<void>& Callback);
    void UnregisterCallback(const CDelegate<void>& Callback);
    
    // 抛出取消异常
    void ThrowIfCancellationRequested();
    
    // 静态令牌
    static NCancellationToken None();

private:
    CAtomic<bool> bIsCancelled;
    CArray<CDelegate<void>> CancelCallbacks;
    NMutex CallbackMutex;
    
    void NotifyCallbacks();
};

/**
 * @brief 异步任务结果包装器
 */
template<typename TType>
class NAsyncResult
{
public:
    NAsyncResult();
    NAsyncResult(const TType& Value);
    NAsyncResult(TType&& Value);
    explicit NAsyncResult(const CString& ErrorMessage);
    
    // 状态查询
    bool IsSuccess() const;
    bool HasError() const;
    
    // 结果获取
    const TType& GetValue() const;
    TType& GetValue();
    const CString& GetError() const;
    
    // 操作符重载
    operator bool() const;
    const TType& operator*() const;
    TType& operator*();

private:
    TType Value;
    CString ErrorMessage;
    bool bHasValue;
    bool bHasError;
};

/**
 * @brief 异步任务基类
 */
class LIBNLIB_API NAsyncTaskBase : public CObject
{

public:
    NAsyncTaskBase();
    virtual ~NAsyncTaskBase();
    
    // 任务控制
    virtual void Start() = 0;
    virtual void Cancel();
    virtual void Wait();
    virtual bool WaitFor(int32_t TimeoutMs);
    
    // 状态查询
    EAsyncTaskState GetState() const;
    bool IsCompleted() const;
    bool IsRunning() const;
    bool IsCancelled() const;
    bool IsFaulted() const;
    
    // 任务属性
    uint64_t GetTaskId() const { return TaskId; }
    EAsyncTaskPriority GetPriority() const { return Priority; }
    void SetPriority(EAsyncTaskPriority InPriority) { Priority = InPriority; }
    
    CString GetName() const { return Name; }
    void SetName(const CString& InName) { Name = InName; }
    
    // 取消令牌
    NCancellationToken& GetCancellationToken() { return CancellationToken; }
    
    // 异常信息
    CString GetExceptionMessage() const { return ExceptionMessage; }

protected:
    void SetState(EAsyncTaskState NewState);
    void SetException(const CString& Message);
    
    virtual void OnStarted() {}
    virtual void OnCompleted() {}
    virtual void OnCancelled() {}
    virtual void OnFaulted() {}

private:
    uint64_t TaskId;
    EAsyncTaskState State;
    EAsyncTaskPriority Priority;
    CString Name;
    NCancellationToken CancellationToken;
    CString ExceptionMessage;
    
    NEvent CompletionEvent;
    static CAtomic<uint64_t> NextTaskId;
};

/**
 * @brief 带返回值的异步任务
 */
template<typename TResult>
class NAsyncTask : public NAsyncTaskBase
{

public:
    using TaskFunction = NFunction<TResult(NCancellationToken&)>;
    using TaskFunctionSimple = NFunction<TResult()>;
    
    // 构造函数
    NAsyncTask();
    explicit NAsyncTask(const TaskFunction& Function);
    explicit NAsyncTask(TaskFunctionSimple Function);
    virtual ~NAsyncTask();
    
    // 任务执行
    virtual void Start() override;
    
    // 结果获取
    TResult GetResult();
    NAsyncResult<TResult> GetAsyncResult();
    bool TryGetResult(TResult& OutResult);
    
    // 任务链式操作
    template<typename TNext>
    TSharedPtr<NAsyncTask<TNext>> ContinueWith(NFunction<TNext(TResult)> Continuation);
    
    template<typename TNext>
    TSharedPtr<NAsyncTask<TNext>> ContinueWith(NFunction<TNext(NAsyncTask<TResult>&)> Continuation);
    
    // 静态工厂方法
    static TSharedPtr<NAsyncTask<TResult>> Run(const TaskFunction& Function);
    static TSharedPtr<NAsyncTask<TResult>> Run(TaskFunctionSimple Function);
    static TSharedPtr<NAsyncTask<TResult>> FromResult(const TResult& Result);
    static TSharedPtr<NAsyncTask<TResult>> FromException(const CString& ExceptionMessage);
    
    // 组合任务
    static TSharedPtr<NAsyncTask<CArray<TResult>>> WhenAll(const CArray<TSharedPtr<NAsyncTask<TResult>>>& Tasks);
    static TSharedPtr<NAsyncTask<TResult>> WhenAny(const CArray<TSharedPtr<NAsyncTask<TResult>>>& Tasks);

private:
    TaskFunction Function;
    TResult Result;
    bool bHasResult;
    
    void ExecuteTask();
    static void TaskThreadEntry(void* TaskPtr);
};

/**
 * @brief void特化版本
 */
template<>
class LIBNLIB_API NAsyncTask<void> : public NAsyncTaskBase
{
    NCLASS()
class NAsyncTask : public NAsyncTaskBase
{
    GENERATED_BODY()

public:
    using TaskFunction = NFunction<void(NCancellationToken&)>;
    using TaskFunctionSimple = NFunction<void()>;
    
    NAsyncTask();
    explicit NAsyncTask(const TaskFunction& Function);
    explicit NAsyncTask(TaskFunctionSimple Function);
    virtual ~NAsyncTask();
    
    virtual void Start() override;
    
    void GetResult(); // 等待完成
    bool TryGetResult();
    
    template<typename TNext>
    TSharedPtr<NAsyncTask<TNext>> ContinueWith(NFunction<TNext()> Continuation);
    
    static TSharedPtr<NAsyncTask<void>> Run(const TaskFunction& Function);
    static TSharedPtr<NAsyncTask<void>> Run(TaskFunctionSimple Function);
    static TSharedPtr<NAsyncTask<void>> CompletedTask();
    
    static TSharedPtr<NAsyncTask<void>> WhenAll(const CArray<TSharedPtr<NAsyncTask<void>>>& Tasks);

private:
    TaskFunction Function;
    
    void ExecuteTask();
    static void TaskThreadEntry(void* TaskPtr);
};

/**
 * @brief 异步任务调度器
 */
class LIBNLIB_API NAsyncTaskScheduler : public CObject
{
public:
    NAsyncTaskScheduler();
    explicit NAsyncTaskScheduler(uint32_t MaxConcurrency);
    virtual ~NAsyncTaskScheduler();
    
    // 任务调度
    void ScheduleTask(TSharedPtr<NAsyncTaskBase> Task);
    void ScheduleTask(TSharedPtr<NAsyncTaskBase> Task, EAsyncTaskPriority Priority);
    
    // 调度器控制
    void Start();
    void Stop();
    void StopGracefully(int32_t TimeoutMs = 30000);
    
    // 状态查询
    bool IsRunning() const;
    uint32_t GetMaxConcurrency() const { return MaxConcurrency; }
    uint32_t GetActiveTaskCount() const;
    uint32_t GetPendingTaskCount() const;
    
    // 等待所有任务完成
    void WaitForAllTasks();
    bool WaitForAllTasks(int32_t TimeoutMs);
    
    // 静态实例
    static NAsyncTaskScheduler& GetDefaultScheduler();
    static NAsyncTaskScheduler& GetBackgroundScheduler();

private:
    struct TaskEntry
    {
        TSharedPtr<NAsyncTaskBase> Task;
        EAsyncTaskPriority Priority;
        int64_t SubmitTime;
        
        TaskEntry(TSharedPtr<NAsyncTaskBase> InTask, EAsyncTaskPriority InPriority)
            : Task(InTask), Priority(InPriority), SubmitTime(GetCurrentTimeMs())
        {}
        
        static int64_t GetCurrentTimeMs();
    };
    
    uint32_t MaxConcurrency;
    CAtomic<bool> bIsRunning;
    CAtomic<uint32_t> ActiveTaskCount;
    
    CArray<TaskEntry> TaskQueue;
    NMutex QueueMutex;
    NConditionVariable QueueCondition;
    
    CArray<TSharedPtr<CThread>> WorkerThreads;
    
    void WorkerThreadMain();
    TaskEntry GetNextTask();
    void ExecuteTask(const TaskEntry& Entry);
    
    // 禁止复制
    NAsyncTaskScheduler(const NAsyncTaskScheduler&) = delete;
    NAsyncTaskScheduler& operator=(const NAsyncTaskScheduler&) = delete;
};

/**
 * @brief 并行任务执行器
 */
class LIBNLIB_API NParallelExecutor : public CObject
{
    NCLASS()
class NParallelExecutor : public NObject
{
    GENERATED_BODY()

public:
    // 并行For循环
    template<typename TFunction>
    static void ParallelFor(int32_t Start, int32_t End, TFunction Function);
    
    template<typename TFunction>
    static void ParallelFor(int32_t Start, int32_t End, int32_t BatchSize, TFunction Function);
    
    // 并行ForEach
    template<typename TContainer, typename TFunction>
    static void ParallelForEach(TContainer& Container, TFunction Function);
    
    // 并行调用
    template<typename... TFunctions>
    static void ParallelInvoke(TFunctions... Functions);
    
    // 映射归约
    template<typename TInput, typename TOutput, typename TMapFunc, typename TReduceFunc>
    static TOutput MapReduce(const CArray<TInput>& Input, TMapFunc MapFunction, TReduceFunc ReduceFunction, const TOutput& InitialValue);

private:
    NParallelExecutor() = default; // 静态类，不允许实例化
};

// === 模板实现 ===

template<typename TType>
NAsyncResult<TType>::NAsyncResult()
    : bHasValue(false), bHasError(false)
{}

template<typename TType>
NAsyncResult<TType>::NAsyncResult(const TType& InValue)
    : Value(InValue), bHasValue(true), bHasError(false)
{}

template<typename TType>
NAsyncResult<TType>::NAsyncResult(TType&& InValue)
    : Value(NLib::Move(InValue)), bHasValue(true), bHasError(false)
{}

template<typename TType>
NAsyncResult<TType>::NAsyncResult(const CString& InErrorMessage)
    : ErrorMessage(InErrorMessage), bHasValue(false), bHasError(true)
{}

template<typename TType>
bool NAsyncResult<TType>::IsSuccess() const
{
    return bHasValue && !bHasError;
}

template<typename TType>
bool NAsyncResult<TType>::HasError() const
{
    return bHasError;
}

template<typename TType>
const TType& NAsyncResult<TType>::GetValue() const
{
    if (!bHasValue)
    {
        static TType DefaultValue{};
        return DefaultValue;
    }
    return Value;
}

template<typename TType>
TType& NAsyncResult<TType>::GetValue()
{
    if (!bHasValue)
    {
        static TType DefaultValue{};
        return DefaultValue;
    }
    return Value;
}

template<typename TType>
const CString& NAsyncResult<TType>::GetError() const
{
    return ErrorMessage;
}

template<typename TType>
NAsyncResult<TType>::operator bool() const
{
    return IsSuccess();
}

template<typename TType>
const TType& NAsyncResult<TType>::operator*() const
{
    return GetValue();
}

template<typename TType>
TType& NAsyncResult<TType>::operator*()
{
    return GetValue();
}

template<typename TResult>
NAsyncTask<TResult>::NAsyncTask()
    : bHasResult(false)
{}

template<typename TResult>
NAsyncTask<TResult>::NAsyncTask(const TaskFunction& InFunction)
    : Function(InFunction), bHasResult(false)
{}

template<typename TResult>
NAsyncTask<TResult>::NAsyncTask(TaskFunctionSimple InFunction)
    : bHasResult(false)
{
    Function = [InFunction](NCancellationToken& Token) -> TResult {
        return InFunction();
    };
}

template<typename TResult>
NAsyncTask<TResult>::~NAsyncTask()
{
    if (IsRunning())
    {
        Cancel();
        WaitFor(5000); // 等待5秒
    }
}

template<typename TResult>
void NAsyncTask<TResult>::Start()
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

template<typename TResult>
TResult NAsyncTask<TResult>::GetResult()
{
    Wait();
    
    if (IsFaulted())
    {
        // 在实际实现中，这里应该抛出异常
        return TResult{};
    }
    
    return Result;
}

template<typename TResult>
NAsyncResult<TResult> NAsyncTask<TResult>::GetAsyncResult()
{
    if (!IsCompleted())
    {
        return NAsyncResult<TResult>("Task not completed");
    }
    
    if (IsFaulted())
    {
        return NAsyncResult<TResult>(GetExceptionMessage());
    }
    
    return NAsyncResult<TResult>(Result);
}

template<typename TResult>
bool NAsyncTask<TResult>::TryGetResult(TResult& OutResult)
{
    if (IsCompleted() && !IsFaulted())
    {
        OutResult = Result;
        return true;
    }
    return false;
}

template<typename TResult>
template<typename TNext>
TSharedPtr<NAsyncTask<TNext>> NAsyncTask<TResult>::ContinueWith(NFunction<TNext(TResult)> Continuation)
{
    auto ContinuationTask = NewNObject<NAsyncTask<TNext>>();
    
    ContinuationTask->Function = [this, Continuation](NCancellationToken& Token) -> TNext {
        TResult PreviousResult = this->GetResult();
        return Continuation(PreviousResult);
    };
    
    // 如果当前任务已完成，立即启动继续任务
    if (IsCompleted())
    {
        ContinuationTask->Start();
    }
    else
    {
        // 否则在后台线程中等待当前任务完成
        auto WaitThread = NewNObject<CThread>([this, ContinuationTask]() {
            this->Wait();
            ContinuationTask->Start();
        });
        WaitThread->Start();
        WaitThread->Detach();
    }
    
    return ContinuationTask;
}

template<typename TResult>
template<typename TNext>
TSharedPtr<NAsyncTask<TNext>> NAsyncTask<TResult>::ContinueWith(NFunction<TNext(NAsyncTask<TResult>&)> Continuation)
{
    auto ContinuationTask = NewNObject<NAsyncTask<TNext>>();
    
    ContinuationTask->Function = [this, Continuation](NCancellationToken& Token) -> TNext {
        this->Wait();
        return Continuation(*this);
    };
    
    if (IsCompleted())
    {
        ContinuationTask->Start();
    }
    else
    {
        auto WaitThread = NewNObject<CThread>([this, ContinuationTask]() {
            this->Wait();
            ContinuationTask->Start();
        });
        WaitThread->Start();
        WaitThread->Detach();
    }
    
    return ContinuationTask;
}

template<typename TResult>
TSharedPtr<NAsyncTask<TResult>> NAsyncTask<TResult>::Run(const TaskFunction& Function)
{
    auto Task = NewNObject<NAsyncTask<TResult>>(Function);
    Task->Start();
    return Task;
}

template<typename TResult>
TSharedPtr<NAsyncTask<TResult>> NAsyncTask<TResult>::Run(TaskFunctionSimple Function)
{
    auto Task = NewNObject<NAsyncTask<TResult>>(Function);
    Task->Start();
    return Task;
}

template<typename TResult>
TSharedPtr<NAsyncTask<TResult>> NAsyncTask<TResult>::FromResult(const TResult& InResult)
{
    auto Task = NewNObject<NAsyncTask<TResult>>();
    Task->Result = InResult;
    Task->bHasResult = true;
    Task->SetState(EAsyncTaskState::Completed);
    return Task;
}

template<typename TResult>
TSharedPtr<NAsyncTask<TResult>> NAsyncTask<TResult>::FromException(const CString& ExceptionMessage)
{
    auto Task = NewNObject<NAsyncTask<TResult>>();
    Task->SetException(ExceptionMessage);
    Task->SetState(EAsyncTaskState::Faulted);
    return Task;
}

template<typename TResult>
TSharedPtr<NAsyncTask<CArray<TResult>>> NAsyncTask<TResult>::WhenAll(const CArray<TSharedPtr<NAsyncTask<TResult>>>& Tasks)
{
    auto CombinedTask = NewNObject<NAsyncTask<CArray<TResult>>>();
    
    CombinedTask->Function = [Tasks](NCancellationToken& Token) -> CArray<TResult> {
        CArray<TResult> Results;
        
        for (const auto& Task : Tasks)
        {
            if (Token.IsCancellationRequested())
            {
                Token.ThrowIfCancellationRequested();
            }
            
            TResult Result = Task->GetResult();
            Results.PushBack(Result);
        }
        
        return Results;
    };
    
    CombinedTask->Start();
    return CombinedTask;
}

template<typename TResult>
TSharedPtr<NAsyncTask<TResult>> NAsyncTask<TResult>::WhenAny(const CArray<TSharedPtr<NAsyncTask<TResult>>>& Tasks)
{
    auto FirstTask = NewNObject<NAsyncTask<TResult>>();
    
    FirstTask->Function = [Tasks](NCancellationToken& Token) -> TResult {
        while (!Token.IsCancellationRequested())
        {
            for (const auto& Task : Tasks)
            {
                if (Task->IsCompleted())
                {
                    return Task->GetResult();
                }
            }
            
            CThread::Sleep(10); // 等待10毫秒
        }
        
        Token.ThrowIfCancellationRequested();
        return TResult{};
    };
    
    FirstTask->Start();
    return FirstTask;
}

template<typename TResult>
void NAsyncTask<TResult>::ExecuteTask()
{
    try
    {
        if (Function)
        {
            Result = Function(GetCancellationToken());
            bHasResult = true;
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

template<typename TResult>
void NAsyncTask<TResult>::TaskThreadEntry(void* TaskPtr)
{
    auto* Task = static_cast<NAsyncTask<TResult>*>(TaskPtr);
    if (Task)
    {
        Task->ExecuteTask();
    }
}

// 并行执行器模板实现
template<typename TFunction>
void NParallelExecutor::ParallelFor(int32_t Start, int32_t End, TFunction Function)
{
    uint32_t ThreadCount = CThread::GetHardwareConcurrency();
    ParallelFor(Start, End, (End - Start + ThreadCount - 1) / ThreadCount, Function);
}

template<typename TFunction>
void NParallelExecutor::ParallelFor(int32_t Start, int32_t End, int32_t BatchSize, TFunction Function)
{
    CArray<TSharedPtr<NAsyncTask<void>>> Tasks;
    
    for (int32_t i = Start; i < End; i += BatchSize)
    {
        int32_t BatchEnd = NLib::Min(i + BatchSize, End);
        
        auto Task = NAsyncTask<void>::Run([i, BatchEnd, Function]() {
            for (int32_t j = i; j < BatchEnd; ++j)
            {
                Function(j);
            }
        });
        
        Tasks.PushBack(Task);
    }
    
    // 等待所有任务完成
    auto WhenAllTask = NAsyncTask<void>::WhenAll(Tasks);
    WhenAllTask->Wait();
}

template<typename TContainer, typename TFunction>
void NParallelExecutor::ParallelForEach(TContainer& Container, TFunction Function)
{
    ParallelFor(0, static_cast<int32_t>(Container.GetSize()), [&Container, Function](int32_t Index) {
        Function(Container[Index]);
    });
}

template<typename... TFunctions>
void NParallelExecutor::ParallelInvoke(TFunctions... Functions)
{
    CArray<TSharedPtr<NAsyncTask<void>>> Tasks;
    
    auto AddTask = [&Tasks](auto Function) {
        auto Task = NAsyncTask<void>::Run(Function);
        Tasks.PushBack(Task);
    };
    
    (AddTask(Functions), ...); // C++17折叠表达式
    
    auto WhenAllTask = NAsyncTask<void>::WhenAll(Tasks);
    WhenAllTask->Wait();
}

template<typename TInput, typename TOutput, typename TMapFunc, typename TReduceFunc>
TOutput NParallelExecutor::MapReduce(const CArray<TInput>& Input, TMapFunc MapFunction, TReduceFunc ReduceFunction, const TOutput& InitialValue)
{
    uint32_t ThreadCount = CThread::GetHardwareConcurrency();
    CArray<TSharedPtr<NAsyncTask<TOutput>>> MapTasks;
    
    int32_t BatchSize = (Input.GetSize() + ThreadCount - 1) / ThreadCount;
    
    // Map阶段
    for (uint32_t i = 0; i < ThreadCount && i * BatchSize < Input.GetSize(); ++i)
    {
        int32_t Start = i * BatchSize;
        int32_t End = NLib::Min(Start + BatchSize, static_cast<int32_t>(Input.GetSize()));
        
        auto MapTask = NAsyncTask<TOutput>::Run([&Input, MapFunction, ReduceFunction, InitialValue, Start, End]() -> TOutput {
            TOutput LocalResult = InitialValue;
            for (int32_t j = Start; j < End; ++j)
            {
                auto MappedValue = MapFunction(Input[j]);
                LocalResult = ReduceFunction(LocalResult, MappedValue);
            }
            return LocalResult;
        });
        
        MapTasks.PushBack(MapTask);
    }
    
    // Reduce阶段
    TOutput FinalResult = InitialValue;
    for (const auto& Task : MapTasks)
    {
        TOutput PartialResult = Task->GetResult();
        FinalResult = ReduceFunction(FinalResult, PartialResult);
    }
    
    return FinalResult;
}

} // namespace NLib
