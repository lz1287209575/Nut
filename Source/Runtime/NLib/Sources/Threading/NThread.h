#pragma once

#include "Containers/CArray.h"
#include "Containers/CString.h"
#include "Core/CObject.h"
#include "DateTime/NDateTime.h"
#include "Delegates/CDelegate.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

namespace NLib
{

/**
 * @brief 线程状态枚举
 */
enum class EThreadState : uint32_t
{
	Unstarted,     // 未开始
	Running,       // 运行中
	WaitSleepJoin, // 等待、睡眠或连接
	Stopped,       // 已停止
	Suspended,     // 已挂起
	Aborted        // 已中止
};

/**
 * @brief 线程优先级枚举
 */
enum class EThreadPriority : uint32_t
{
	Lowest = 0,
	BelowNormal = 1,
	Normal = 2,
	AboveNormal = 3,
	Highest = 4
};

/**
 * @brief 线程公寓状态枚举
 */
enum class EApartmentState : uint32_t
{
	STA,    // 单线程公寓
	MTA,    // 多线程公寓
	Unknown // 未知
};

/**
 * @brief 互斥锁封装
 */
class LIBNLIB_API NMutex : public CObject
{
	NCLASS()
class NMutex : public NObject
{
    GENERATED_BODY()

public:
	NMutex();
	virtual ~NMutex();

	void Lock();
	bool TryLock();
	void Unlock();

	// 获取原生互斥锁对象（用于条件变量）
	std::mutex& GetNativeMutex()
	{
		return Mutex;
	}

private:
	std::mutex Mutex;

	// 禁止复制
	NMutex(const NMutex&) = delete;
	NMutex& operator=(const NMutex&) = delete;
};

/**
 * @brief 读写锁封装
 */
class LIBNLIB_API NReadWriteLock : public CObject
{
	NCLASS()
class NReadWriteLock : public NObject
{
    GENERATED_BODY()

public:
	NReadWriteLock();
	virtual ~NReadWriteLock();

	void LockRead();
	bool TryLockRead();
	void UnlockRead();

	void LockWrite();
	bool TryLockWrite();
	void UnlockWrite();

private:
	std::shared_mutex ReadWriteMutex;

	// 禁止复制
	NReadWriteLock(const NReadWriteLock&) = delete;
	NReadWriteLock& operator=(const NReadWriteLock&) = delete;
};

/**
 * @brief 条件变量封装
 */
class LIBNLIB_API NConditionVariable : public CObject
{
	NCLASS()
class NConditionVariable : public NObject
{
    GENERATED_BODY()

public:
	NConditionVariable();
	virtual ~NConditionVariable();

	void Wait(NMutex& Mutex);
	bool WaitFor(NMutex& Mutex, int32_t TimeoutMs);
	template <typename TPredicate>
	void Wait(NMutex& Mutex, TPredicate Pred);
	template <typename TPredicate>
	bool WaitFor(NMutex& Mutex, int32_t TimeoutMs, TPredicate Pred);

	void NotifyOne();
	void NotifyAll();

private:
	std::condition_variable CondVar;

	// 禁止复制
	NConditionVariable(const NConditionVariable&) = delete;
	NConditionVariable& operator=(const NConditionVariable&) = delete;
};

/**
 * @brief 自动锁管理器
 */
template <typename TMutex>
class CLockGuard
{
public:
	explicit CLockGuard(TMutex& InMutex)
	    : MutexRef(InMutex)
	{
		MutexRef.Lock();
	}

	~CLockGuard()
	{
		MutexRef.Unlock();
	}

private:
	TMutex& MutexRef;

	// 禁止复制
	CLockGuard(const CLockGuard&) = delete;
	CLockGuard& operator=(const CLockGuard&) = delete;
};

/**
 * @brief 读锁管理器
 */
class CReadLockGuard
{
public:
	explicit CReadLockGuard(NReadWriteLock& InLock)
	    : LockRef(InLock)
	{
		LockRef.LockRead();
	}

	~CReadLockGuard()
	{
		LockRef.UnlockRead();
	}

private:
	NReadWriteLock& LockRef;

	// 禁止复制
	CReadLockGuard(const CReadLockGuard&) = delete;
	CReadLockGuard& operator=(const CReadLockGuard&) = delete;
};

/**
 * @brief 写锁管理器
 */
class CWriteLockGuard
{
public:
	explicit CWriteLockGuard(NReadWriteLock& InLock)
	    : LockRef(InLock)
	{
		LockRef.LockWrite();
	}

	~CWriteLockGuard()
	{
		LockRef.UnlockWrite();
	}

private:
	NReadWriteLock& LockRef;

	// 禁止复制
	CWriteLockGuard(const CWriteLockGuard&) = delete;
	CWriteLockGuard& operator=(const CWriteLockGuard&) = delete;
};

// 便利宏定义
#define LOCK_GUARD(Mutex) CLockGuard<decltype(Mutex)> _LockGuard(Mutex)
#define READ_LOCK_GUARD(ReadWriteLock) CReadLockGuard _ReadLockGuard(ReadWriteLock)
#define WRITE_LOCK_GUARD(ReadWriteLock) CWriteLockGuard _WriteLockGuard(ReadWriteLock)

/**
 * @brief 线程局部存储
 */
template <typename TType>
class CThreadLocal
{
public:
	CThreadLocal() = default;
	explicit CThreadLocal(const TType& InitialValue)
	    : Value(InitialValue)
	{}

	TType& Get()
	{
		return Value;
	}
	const TType& Get() const
	{
		return Value;
	}

	void Set(const TType& NewValue)
	{
		Value = NewValue;
	}
	void Set(TType&& NewValue)
	{
		Value = std::move(NewValue);
	}

	TType& operator*()
	{
		return Value;
	}
	const TType& operator*() const
	{
		return Value;
	}

	TType* operator->()
	{
		return &Value;
	}
	const TType* operator->() const
	{
		return &Value;
	}

private:
	thread_local static TType Value;
};

template <typename TType>
thread_local TType CThreadLocal<TType>::Value{};

/**
 * @brief 原子操作封装
 */
template <typename TType>
class CAtomic
{
public:
	CAtomic() = default;
	explicit CAtomic(const TType& InitialValue)
	    : Value(InitialValue)
	{}

	TType Load() const
	{
		return Value.load();
	}
	void Store(const TType& NewValue)
	{
		Value.store(NewValue);
	}

	TType Exchange(const TType& NewValue)
	{
		return Value.exchange(NewValue);
	}

	bool CompareExchange(TType& Expected, const TType& Desired)
	{
		return Value.compare_exchange_weak(Expected, Desired);
	}

	// 算术操作（仅适用于整数类型）
	template <typename U = TType>
	typename std::enable_if<std::is_integral_v<U>, U>::type FetchAdd(const TType& Arg)
	{
		return Value.fetch_add(Arg);
	}

	template <typename U = TType>
	typename std::enable_if<std::is_integral_v<U>, U>::type FetchSub(const TType& Arg)
	{
		return Value.fetch_sub(Arg);
	}

	template <typename U = TType>
	typename std::enable_if<std::is_integral_v<U>, U>::type operator++()
	{
		return ++Value;
	}

	template <typename U = TType>
	typename std::enable_if<std::is_integral_v<U>, U>::type operator++(int)
	{
		return Value++;
	}

	template <typename U = TType>
	typename std::enable_if<std::is_integral_v<U>, U>::type operator--()
	{
		return --Value;
	}

	template <typename U = TType>
	typename std::enable_if<std::is_integral_v<U>, U>::type operator--(int)
	{
		return Value--;
	}

	// 隐式转换
	operator TType() const
	{
		return Load();
	}

	// 赋值操作
	CAtomic& operator=(const TType& NewValue)
	{
		Store(NewValue);
		return *this;
	}

private:
	std::atomic<TType> Value;
};

/**
 * @brief 线程封装类
 */
class LIBNLIB_API CThread : public CObject
{
	NCLASS()
class CThread : public CObject
{
    GENERATED_BODY()

public:
	using ThreadFunction = std::function<void()>;

	// 构造函数
	CThread();
	explicit CThread(const ThreadFunction& Function);
	explicit CThread(ThreadFunction&& Function);
	virtual ~CThread();

	// 线程控制
	void Start();
	void Start(const ThreadFunction& Function);
	void Join();
	bool TryJoin(int32_t TimeoutMs);
	void Detach();

	// 线程状态
	bool IsAlive() const;
	EThreadState GetThreadState() const;
	uint32_t GetThreadId() const;
	bool IsBackground() const;
	void SetIsBackground(bool bBackground);

	// 线程属性
	CString GetName() const
	{
		return Name;
	}
	void SetName(const CString& InName)
	{
		Name = InName;
	}

	EThreadPriority GetPriority() const
	{
		return Priority;
	}
	void SetPriority(EThreadPriority InPriority);

	EApartmentState GetApartmentState() const
	{
		return ApartmentState;
	}
	void SetApartmentState(EApartmentState State)
	{
		ApartmentState = State;
	}

	// 静态方法
	static CThread* GetCurrentThread();
	static uint32_t GetCurrentThreadId();
	static void Sleep(int32_t Milliseconds);
	static void Yield();
	static uint32_t GetHardwareConcurrency();

	// 中断支持
	void Interrupt();
	bool IsInterrupted() const;
	static void CheckForInterruption();

protected:
	virtual void Run();

private:
	std::unique_ptr<std::thread> ThreadHandle;
	ThreadFunction Function;
	EThreadState ThreadState;
	EThreadPriority Priority;
	EApartmentState ApartmentState;
	CString Name;
	bool bIsBackground;
	CAtomic<bool> bIsInterrupted;

	void ThreadMain();
	void SetThreadState(EThreadState NewState);

	// 静态线程本地存储
	static thread_local CThread* CurrentThreadInstance;
};

/**
 * @brief 异步任务基类
 */
template <typename TResult>
class NTask : public NObject
{
    NCLASS()
class NTask : public NObject
{
    GENERATED_BODY()

public:
	using TaskFunction = std::function<TResult()>;

	// 构造函数
	NTask() = default;
	explicit NTask(const TaskFunction& Function)
	    : Function(Function)
	{}
	explicit NTask(TaskFunction&& Function)
	    : Function(std::move(Function))
	{}

	// 任务执行
	void Start()
	{
		if (bIsStarted.Exchange(true))
		{
			return; // 已经启动过了
		}

		Future = std::async(std::launch::async, [this]() -> TResult {
			try
			{
				if (Function)
				{
					return Function();
				}
				return TResult{};
			}
			catch (...)
			{
				Exception = std::current_exception();
				throw;
			}
		});

		bIsCompleted.Store(false);
	}

	// 等待完成
	void Wait()
	{
		if (Future.valid())
		{
			Future.wait();
			bIsCompleted.Store(true);
		}
	}

	bool WaitFor(int32_t TimeoutMs)
	{
		if (Future.valid())
		{
			auto Status = Future.wait_for(std::chrono::milliseconds(TimeoutMs));
			bool bCompleted = (Status == std::future_status::ready);
			if (bCompleted)
			{
				bIsCompleted.Store(true);
			}
			return bCompleted;
		}
		return true;
	}

	// 获取结果
	TResult GetResult()
	{
		if (Future.valid())
		{
			TResult Result = Future.get();
			bIsCompleted.Store(true);

			if (Exception)
			{
				std::rethrow_exception(Exception);
			}

			return Result;
		}
		return TResult{};
	}

	// 状态查询
	bool IsCompleted() const
	{
		return bIsCompleted.Load();
	}
	bool IsStarted() const
	{
		return bIsStarted.Load();
	}
	bool IsFaulted() const
	{
		return Exception != nullptr;
	}

	// 继续任务
	template <typename TNext>
	TSharedPtr<NTask<TNext>> ContinueWith(std::function<TNext(TResult)> Continuation)
	{
		auto ContinuationTask = NewNObject<NTask<TNext>>();

		ContinuationTask->Function = [this, Continuation]() -> TNext {
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
			std::thread([this, ContinuationTask]() {
				this->Wait();
				ContinuationTask->Start();
			}).detach();
		}

		return ContinuationTask;
	}

	// 静态工厂方法
	static TSharedPtr<NTask<TResult>> Run(const TaskFunction& Function)
	{
		auto Task = NewNObject<NTask<TResult>>(Function);
		Task->Start();
		return Task;
	}

	static TSharedPtr<NTask<void>> Run(std::function<void()> Action);

protected:
	TaskFunction Function;
	std::future<TResult> Future;
	CAtomic<bool> bIsStarted{false};
	CAtomic<bool> bIsCompleted{false};
	std::exception_ptr Exception{nullptr};
};

/**
 * @brief void特化版本
 */
template <>
class LIBNLIB_API NTask<void> : public CObject
{
	NCLASS()
class NTask : public NObject
{
    GENERATED_BODY()

public:
	using TaskFunction = std::function<void()>;

	NTask() = default;
	explicit NTask(const TaskFunction& Function);
	explicit NTask(TaskFunction&& Function);

	void Start();
	void Wait();
	bool WaitFor(int32_t TimeoutMs);
	void GetResult(); // 对于void任务，只是等待完成

	bool IsCompleted() const;
	bool IsStarted() const;
	bool IsFaulted() const;

	template <typename TNext>
	TSharedPtr<NTask<TNext>> ContinueWith(std::function<TNext()> Continuation);

	static TSharedPtr<NTask<void>> Run(const TaskFunction& Function);

private:
	TaskFunction Function;
	std::future<void> Future;
	CAtomic<bool> bIsStarted{false};
	CAtomic<bool> bIsCompleted{false};
	std::exception_ptr Exception{nullptr};
};

/**
 * @brief 线程池
 */
class LIBNLIB_API NThreadPool : public CObject
{
	NCLASS()
class NThreadPool : public NObject
{
    GENERATED_BODY()

public:
	NThreadPool();
	explicit NThreadPool(uint32_t ThreadCount);
	virtual ~NThreadPool();

	// 任务提交
	template <typename TFunction, typename... TArgs>
	auto Submit(TFunction&& Function, TArgs&&... Args)
	    -> std::future<typename std::result_of<TFunction(TArgs...)>::type>
	{
		using ReturnType = typename std::result_of<TFunction(TArgs...)>::type;

		auto Task = std::make_shared<std::packaged_task<ReturnType()>>(
		    std::bind(std::forward<TFunction>(Function), std::forward<TArgs>(Args)...));

		std::future<ReturnType> Result = Task->get_future();

		{
			LOCK_GUARD(QueueMutex);

			if (bIsShutdown.Load())
			{
				throw std::runtime_error("ThreadPool is shutdown");
			}

			Tasks.emplace([Task]() { (*Task)(); });
		}

		Condition.NotifyOne();
		return Result;
	}

	// 线程池控制
	void Shutdown();
	void WaitForAll();

	// 状态查询
	uint32_t GetThreadCount() const
	{
		return ThreadCount;
	}
	uint32_t GetActiveThreadCount() const
	{
		return ActiveThreadCount.Load();
	}
	uint32_t GetPendingTaskCount() const;
	bool IsShutdown() const
	{
		return bIsShutdown.Load();
	}

	// 静态实例
	static NThreadPool& GetGlobalThreadPool();

private:
	std::vector<std::unique_ptr<std::thread>> Workers;
	std::queue<std::function<void()>> Tasks;

	NMutex QueueMutex;
	NConditionVariable Condition;

	uint32_t ThreadCount;
	CAtomic<uint32_t> ActiveThreadCount{0};
	CAtomic<bool> bIsShutdown{false};

	void WorkerThread();

	// 禁止复制
	NThreadPool(const NThreadPool&) = delete;
	NThreadPool& operator=(const NThreadPool&) = delete;
};

/**
 * @brief 定时器类
 */
class LIBNLIB_API NTimer : public CObject
{
	NCLASS()
class NTimer : public NObject
{
    GENERATED_BODY()

public:
	using TimerCallback = std::function<void()>;

	NTimer();
	explicit NTimer(const TimerCallback& Callback);
	virtual ~NTimer();

	// 定时器控制
	void Start(int32_t IntervalMs);
	void Start(int32_t DueTimeMs, int32_t IntervalMs);
	void Stop();
	void Reset();

	// 属性设置
	void SetCallback(const TimerCallback& Callback)
	{
		TimerCallback = Callback;
	}
	void SetInterval(int32_t IntervalMs)
	{
		Interval = IntervalMs;
	}
	void SetAutoReset(bool bAutoReset)
	{
		bIsAutoReset = bAutoReset;
	}

	// 状态查询
	bool IsEnabled() const
	{
		return bIsEnabled.Load();
	}
	int32_t GetInterval() const
	{
		return Interval;
	}
	bool GetAutoReset() const
	{
		return bIsAutoReset;
	}

	// 事件
	DECLARE_MULTICAST_DELEGATE(FOnElapsed);
	FOnElapsed OnElapsed;

private:
	TimerCallback Callback;
	std::unique_ptr<std::thread> TimerThread;

	int32_t Interval;
	int32_t DueTime;
	bool bIsAutoReset;
	CAtomic<bool> bIsEnabled{false};
	CAtomic<bool> bIsShutdown{false};

	NMutex TimerMutex;
	NConditionVariable TimerCondition;

	void TimerThreadMain();

	// 禁止复制
	NTimer(const NTimer&) = delete;
	NTimer& operator=(const NTimer&) = delete;
};

/**
 * @brief 信号量
 */
class LIBNLIB_API NSemaphore : public CObject
{
	NCLASS()
class NSemaphore : public NObject
{
    GENERATED_BODY()

public:
	explicit NSemaphore(int32_t InitialCount);
	NSemaphore(int32_t InitialCount, int32_t MaximumCount);
	virtual ~NSemaphore();

	void Wait();
	bool WaitFor(int32_t TimeoutMs);
	void Release();
	void Release(int32_t ReleaseCount);

	int32_t GetCurrentCount() const
	{
		return CurrentCount.Load();
	}
	int32_t GetMaximumCount() const
	{
		return MaximumCount;
	}

private:
	CAtomic<int32_t> CurrentCount;
	int32_t MaximumCount;

	NMutex SemaphoreMutex;
	NConditionVariable SemaphoreCondition;

	// 禁止复制
	NSemaphore(const NSemaphore&) = delete;
	NSemaphore& operator=(const NSemaphore&) = delete;
};

/**
 * @brief 事件对象
 */
class LIBNLIB_API NEvent : public CObject
{
	NCLASS()
class NEvent : public NObject
{
    GENERATED_BODY()

public:
	explicit NEvent(bool bInitialState = false);
	explicit NEvent(bool bInitialState, bool bManualReset);
	virtual ~NEvent();

	void Set();
	void Reset();
	void Wait();
	bool WaitFor(int32_t TimeoutMs);

	bool IsSet() const
	{
		return bIsSignaled.Load();
	}

private:
	CAtomic<bool> bIsSignaled;
	bool bManualReset;

	NMutex EventMutex;
	NConditionVariable EventCondition;

	// 禁止复制
	NEvent(const NEvent&) = delete;
	NEvent& operator=(const NEvent&) = delete;
};

// === 模板方法实现 ===

template <typename TPredicate>
void NConditionVariable::Wait(NMutex& Mutex, TPredicate Pred)
{
	std::unique_lock<std::mutex> Lock(Mutex.GetNativeMutex(), std::adopt_lock);
	CondVar.wait(Lock, Pred);
	Lock.release(); // 释放unique_lock的所有权，让NMutex继续管理
}

template <typename TPredicate>
bool NConditionVariable::WaitFor(NMutex& Mutex, int32_t TimeoutMs, TPredicate Pred)
{
	std::unique_lock<std::mutex> Lock(Mutex.GetNativeMutex(), std::adopt_lock);
	bool Result = CondVar.wait_for(Lock, std::chrono::milliseconds(TimeoutMs), Pred);
	Lock.release(); // 释放unique_lock的所有权，让NMutex继续管理
	return Result;
}

} // namespace NLib