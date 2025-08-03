#pragma once

#include "Core/CObject.h"
#include "Containers/CString.h"
#include "Threading/CThread.h"
#include "Delegates/CDelegate.h"
#include "NAsyncTask.h"

namespace NLib
{

/**
 * @brief 可运行接口 - 定义可在线程中执行的对象
 */
class LIBNLIB_API IRunnable
{
public:
    virtual ~IRunnable() = default;
    
    // 执行入口点
    virtual void Run() = 0;
    
    // 请求停止
    virtual void Stop() {}
    
    // 初始化和清理
    virtual bool Initialize() { return true; }
    virtual void Shutdown() {}
    
    // 状态查询
    virtual bool IsRunning() const { return false; }
    virtual CString GetName() const { return "Unknown"; }
};

/**
 * @brief 可运行对象基类 - NObject版本的IRunnable
 */
class LIBNLIB_API NRunnable : public CObject, public IRunnable
{
    NCLASS()
class NRunnable : public NObject
{
    GENERATED_BODY()

public:
    NRunnable();
    explicit NRunnable(const CString& Name);
    virtual ~NRunnable();
    
    // IRunnable接口实现
    virtual void Run() override = 0;
    virtual void Stop() override;
    virtual bool Initialize() override;
    virtual void Shutdown() override;
    
    // 状态查询
    virtual bool IsRunning() const override;
    virtual CString GetName() const override;
    
    // 属性设置
    void SetName(const CString& InName);
    
    // 事件回调
    CMulticastDelegate<void, NRunnable*> OnStarted;
    CMulticastDelegate<void, NRunnable*> OnStopped;
    CMulticastDelegate<void, NRunnable*, const CString&> OnError;

protected:
    // 子类可重写的事件
    virtual void OnRunnableStarted() {}
    virtual void OnRunnableStopped() {}
    virtual void OnRunnableError(const CString& ErrorMessage) {}
    
    // 检查是否应该停止
    bool ShouldStop() const;
    
    // 设置运行状态
    void SetRunning(bool InRunning);

private:
    CString Name;
    CAtomic<bool> bIsRunning;
    CAtomic<bool> bStopRequested;
};

/**
 * @brief 函数式可运行对象 - 将函数包装为可运行对象
 */
class LIBNLIB_API NFunctionRunnable : public NRunnable
{
    NCLASS()
class NFunctionRunnable : public NRunnable
{
    GENERATED_BODY()

public:
    using RunnableFunction = NFunction<void()>;
    using RunnableFunctionWithStop = NFunction<void(const NFunction<bool()>&)>;
    
    explicit NFunctionRunnable(RunnableFunction Function);
    explicit NFunctionRunnable(RunnableFunctionWithStop Function);
    NFunctionRunnable(RunnableFunction Function, const CString& Name);
    NFunctionRunnable(RunnableFunctionWithStop Function, const CString& Name);
    
    virtual void Run() override;

private:
    RunnableFunction SimpleFunction;
    RunnableFunctionWithStop StopAwareFunction;
    bool bIsStopAware;
};

/**
 * @brief 可运行任务 - 将IRunnable包装为异步任务
 */
class LIBNLIB_API NRunnableTask : public NAsyncTask<void>
{
    DECLARE_NOBJECT_CLASS(NRunnableTask, NAsyncTask<void>)

public:
    explicit NRunnableTask(TSharedPtr<IRunnable> Runnable);
    virtual ~NRunnableTask();
    
    virtual void Start() override;
    virtual void Cancel() override;
    
    // 获取可运行对象
    TSharedPtr<IRunnable> GetRunnable() const;
    
    // 静态工厂方法
    static TSharedPtr<NRunnableTask> Create(TSharedPtr<IRunnable> Runnable);
    static TSharedPtr<NRunnableTask> Create(NFunctionRunnable::RunnableFunction Function);
    static TSharedPtr<NRunnableTask> Create(NFunctionRunnable::RunnableFunction Function, const CString& Name);

private:
    TSharedPtr<IRunnable> RunnableObject;
    TSharedPtr<CThread> ExecutionThread;
    
    void ExecuteRunnable();
    static void RunnableThreadEntry(void* TaskPtr);
};

/**
 * @brief 周期性可运行对象 - 定期执行的任务
 */
class LIBNLIB_API NPeriodicRunnable : public NRunnable
{
    NCLASS()
class NPeriodicRunnable : public NRunnable
{
    GENERATED_BODY()

public:
    using PeriodicFunction = NFunction<void()>;
    
    NPeriodicRunnable(PeriodicFunction Function, int32_t IntervalMs);
    NPeriodicRunnable(PeriodicFunction Function, int32_t IntervalMs, const CString& Name);
    virtual ~NPeriodicRunnable();
    
    virtual void Run() override;
    virtual void Stop() override;
    
    // 配置
    void SetInterval(int32_t IntervalMs);
    int32_t GetInterval() const;
    
    void SetImmediateStart(bool bImmediate);
    bool GetImmediateStart() const;
    
    // 统计信息
    uint64_t GetExecutionCount() const;
    int64_t GetLastExecutionTime() const;
    double GetAverageExecutionTime() const;

private:
    PeriodicFunction Function;
    int32_t IntervalMs;
    bool bImmediateStart;
    
    // 统计数据
    CAtomic<uint64_t> ExecutionCount;
    CAtomic<int64_t> LastExecutionTime;
    CAtomic<double> TotalExecutionTime;
    
    int64_t GetCurrentTimeMs() const;
};

/**
 * @brief 可运行池 - 管理多个可运行对象的执行
 */
class LIBNLIB_API NRunnablePool : public CObject
{
    NCLASS()
class NRunnablePool : public NObject
{
    GENERATED_BODY()

public:
    NRunnablePool();
    explicit NRunnablePool(uint32_t MaxConcurrency);
    virtual ~NRunnablePool();
    
    // 池控制
    void Start();
    void Stop();
    void StopGracefully(int32_t TimeoutMs = 30000);
    
    // 任务管理
    void Submit(TSharedPtr<IRunnable> Runnable);
    void Submit(NFunctionRunnable::RunnableFunction Function);
    void Submit(NFunctionRunnable::RunnableFunction Function, const CString& Name);
    
    // 状态查询
    bool IsRunning() const;
    uint32_t GetActiveCount() const;
    uint32_t GetPendingCount() const;
    uint32_t GetCompletedCount() const;
    uint32_t GetMaxConcurrency() const;
    
    // 等待完成
    void WaitForAll();
    bool WaitForAll(int32_t TimeoutMs);
    
    // 事件
    CMulticastDelegate<void, TSharedPtr<IRunnable>> OnRunnableStarted;
    CMulticastDelegate<void, TSharedPtr<IRunnable>> OnRunnableCompleted;
    CMulticastDelegate<void, TSharedPtr<IRunnable>, const CString&> OnRunnableError;

private:
    struct RunnableEntry
    {
        TSharedPtr<IRunnable> Runnable;
        int64_t SubmitTime;
        
        RunnableEntry(TSharedPtr<IRunnable> InRunnable)
            : Runnable(InRunnable), SubmitTime(GetCurrentTimeMs())
        {}
        
        static int64_t GetCurrentTimeMs();
    };
    
    uint32_t MaxConcurrency;
    CAtomic<bool> bIsRunning;
    CAtomic<uint32_t> ActiveCount;
    CAtomic<uint32_t> CompletedCount;
    
    CArray<RunnableEntry> RunnableQueue;
    NMutex QueueMutex;
    NConditionVariable QueueCondition;
    
    CArray<TSharedPtr<CThread>> WorkerThreads;
    
    void WorkerThreadMain();
    RunnableEntry GetNextRunnable();
    void ExecuteRunnable(const RunnableEntry& Entry);
    
    // 禁止复制
    NRunnablePool(const NRunnablePool&) = delete;
    NRunnablePool& operator=(const NRunnablePool&) = delete;
};

/**
 * @brief 可运行工厂 - 创建各种类型的可运行对象
 */
class LIBNLIB_API NRunnableFactory
{
public:
    // 创建函数式可运行对象
    static TSharedPtr<NFunctionRunnable> CreateFunction(NFunctionRunnable::RunnableFunction Function);
    static TSharedPtr<NFunctionRunnable> CreateFunction(NFunctionRunnable::RunnableFunction Function, const CString& Name);
    static TSharedPtr<NFunctionRunnable> CreateStopAware(NFunctionRunnable::RunnableFunctionWithStop Function);
    static TSharedPtr<NFunctionRunnable> CreateStopAware(NFunctionRunnable::RunnableFunctionWithStop Function, const CString& Name);
    
    // 创建周期性可运行对象
    static TSharedPtr<NPeriodicRunnable> CreatePeriodic(NPeriodicRunnable::PeriodicFunction Function, int32_t IntervalMs);
    static TSharedPtr<NPeriodicRunnable> CreatePeriodic(NPeriodicRunnable::PeriodicFunction Function, int32_t IntervalMs, const CString& Name);
    
    // 创建可运行任务
    static TSharedPtr<NRunnableTask> CreateTask(TSharedPtr<IRunnable> Runnable);
    static TSharedPtr<NRunnableTask> CreateTask(NFunctionRunnable::RunnableFunction Function);
    static TSharedPtr<NRunnableTask> CreateTask(NFunctionRunnable::RunnableFunction Function, const CString& Name);

private:
    NRunnableFactory() = delete; // 静态工厂类
};

/**
 * @brief 全局可运行池访问器
 */
class LIBNLIB_API NRunnableManager
{
public:
    // 获取默认可运行池
    static NRunnablePool& GetDefaultPool();
    static NRunnablePool& GetBackgroundPool();
    
    // 便捷方法
    static void RunInBackground(NFunctionRunnable::RunnableFunction Function);
    static void RunInBackground(NFunctionRunnable::RunnableFunction Function, const CString& Name);
    static TSharedPtr<NRunnableTask> RunAsync(NFunctionRunnable::RunnableFunction Function);
    static TSharedPtr<NRunnableTask> RunAsync(NFunctionRunnable::RunnableFunction Function, const CString& Name);
    
    // 系统控制
    static void Shutdown();

private:
    NRunnableManager() = delete; // 静态管理类
};

} // namespace NLib