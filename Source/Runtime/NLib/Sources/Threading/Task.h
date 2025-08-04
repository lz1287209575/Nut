#pragma once

#include "Containers/TArray.h"
#include "Containers/TString.h"
#include "Core/Object.h"
#include "Core/SmartPointers.h"
#include "Events/Delegate.h"
#include "Logging/LogCategory.h"
#include "Time/TimeTypes.h"

#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <future>
#include <mutex>

namespace NLib
{
/**
 * @brief 任务状态枚举
 */
enum class ETaskState : uint8_t
{
	Created,   // 已创建
	Pending,   // 等待执行
	Running,   // 执行中
	Completed, // 已完成
	Cancelled, // 已取消
	Failed     // 执行失败
};

/**
 * @brief 任务优先级枚举
 */
enum class ETaskPriority : uint8_t
{
	Low = 0,
	Normal = 1,
	High = 2,
	Critical = 3
};

/**
 * @brief 任务执行上下文
 */
struct STaskContext
{
	std::thread::id ExecutingThreadId; // 执行线程ID
	CDateTime StartTime;               // 开始时间
	CDateTime EndTime;                 // 结束时间
	uint32_t RetryCount = 0;           // 重试次数

	CTimespan GetExecutionTime() const
	{
		return EndTime - StartTime;
	}
};

// 前向声明
template <typename T>
class TFuture;
template <typename T>
class TPromise;

/**
 * @brief 任务基类接口
 */
class ITaskBase
{
public:
	virtual ~ITaskBase() = default;

	/**
	 * @brief 执行任务
	 */
	virtual void Execute() = 0;

	/**
	 * @brief 取消任务
	 */
	virtual void Cancel() = 0;

	/**
	 * @brief 获取任务状态
	 */
	virtual ETaskState GetState() const = 0;

	/**
	 * @brief 获取任务ID
	 */
	virtual uint64_t GetTaskId() const = 0;

	/**
	 * @brief 获取任务名称
	 */
	virtual const CString& GetTaskName() const = 0;

	/**
	 * @brief 获取任务优先级
	 */
	virtual ETaskPriority GetPriority() const = 0;

	/**
	 * @brief 检查任务是否完成
	 */
	virtual bool IsCompleted() const = 0;
};

/**
 * @brief 类型化任务类
 */
template <typename TResult>
class TTask : public ITaskBase, public NObject
{
	GENERATED_BODY()

public:
	using FunctionType = std::function<TResult()>;
	using ResultType = TResult;

public:
	// === 委托定义 ===
	DECLARE_DELEGATE(FOnTaskStarted, uint64_t);
	DECLARE_DELEGATE(FOnTaskCompleted, uint64_t, const TResult&);
	DECLARE_DELEGATE(FOnTaskFailed, uint64_t, const CString&);
	DECLARE_DELEGATE(FOnTaskCancelled, uint64_t);

private:
	static std::atomic<uint64_t> NextTaskId;

public:
	// === 构造函数 ===

	TTask(FunctionType InFunction, const CString& InTaskName = "Task", ETaskPriority InPriority = ETaskPriority::Normal)
	    : Function(std::move(InFunction)),
	      TaskName(InTaskName),
	      Priority(InPriority),
	      State(ETaskState::Created),
	      TaskId(NextTaskId.fetch_add(1) + 1),
	      bCancellationRequested(false),
	      CreationTime(CDateTime::Now())
	{}

	virtual ~TTask() = default;

public:
	// === ITaskBase接口实现 ===

	void Execute() override
	{
		if (!TrySetState(ETaskState::Created, ETaskState::Running) &&
		    !TrySetState(ETaskState::Pending, ETaskState::Running))
		{
			NLOG_THREADING(Warning,
			               "Task '{}' (ID: {}) cannot be executed in current state: {}",
			               TaskName.GetData(),
			               TaskId,
			               GetStateString().GetData());
			return;
		}

		Context.ExecutingThreadId = std::this_thread::get_id();
		Context.StartTime = CDateTime::Now();

		OnTaskStarted.ExecuteIfBound(TaskId);

		NLOG_THREADING(Debug, "Task '{}' (ID: {}) started execution", TaskName.GetData(), TaskId);

		try
		{
			// 检查取消状态
			if (bCancellationRequested.load())
			{
				State = ETaskState::Cancelled;
				OnTaskCancelled.ExecuteIfBound(TaskId);
				NLOG_THREADING(Debug, "Task '{}' (ID: {}) was cancelled", TaskName.GetData(), TaskId);
				return;
			}

			// 执行任务函数
			if constexpr (std::is_void_v<TResult>)
			{
				Function();
				State = ETaskState::Completed;
				OnTaskCompleted.ExecuteIfBound(TaskId, TResult{});
			}
			else
			{
				TResult Result = Function();
				{
					std::lock_guard<std::mutex> Lock(ResultMutex);
					TaskResult = std::move(Result);
				}
				State = ETaskState::Completed;
				OnTaskCompleted.ExecuteIfBound(TaskId, TaskResult);
			}

			Context.EndTime = CDateTime::Now();

			NLOG_THREADING(Debug,
			               "Task '{}' (ID: {}) completed successfully in {:.2f}ms",
			               TaskName.GetData(),
			               TaskId,
			               Context.GetExecutionTime().GetTotalMilliseconds());

			// 通知等待者
			CompletionCondition.notify_all();
		}
		catch (const std::exception& e)
		{
			State = ETaskState::Failed;
			Context.EndTime = CDateTime::Now();

			CString ErrorMsg(e.what());
			{
				std::lock_guard<std::mutex> Lock(ResultMutex);
				Exception = std::current_exception();
			}

			NLOG_THREADING(Error, "Task '{}' (ID: {}) failed with exception: {}", TaskName.GetData(), TaskId, e.what());

			OnTaskFailed.ExecuteIfBound(TaskId, ErrorMsg);
			CompletionCondition.notify_all();
		}
		catch (...)
		{
			State = ETaskState::Failed;
			Context.EndTime = CDateTime::Now();

			{
				std::lock_guard<std::mutex> Lock(ResultMutex);
				Exception = std::current_exception();
			}

			NLOG_THREADING(Error, "Task '{}' (ID: {}) failed with unknown exception", TaskName.GetData(), TaskId);

			CString ErrorMsg("Unknown exception");
			OnTaskFailed.ExecuteIfBound(TaskId, ErrorMsg);
			CompletionCondition.notify_all();
		}
	}

	void Cancel() override
	{
		if (State == ETaskState::Created || State == ETaskState::Pending)
		{
			bCancellationRequested = true;
			State = ETaskState::Cancelled;
			OnTaskCancelled.ExecuteIfBound(TaskId);
			CompletionCondition.notify_all();

			NLOG_THREADING(Debug, "Task '{}' (ID: {}) cancelled", TaskName.GetData(), TaskId);
		}
	}

	ETaskState GetState() const override
	{
		return State.load();
	}

	uint64_t GetTaskId() const override
	{
		return TaskId;
	}

	const CString& GetTaskName() const override
	{
		return TaskName;
	}

	ETaskPriority GetPriority() const override
	{
		return Priority;
	}

	bool IsCompleted() const override
	{
		ETaskState CurrentState = State.load();
		return CurrentState == ETaskState::Completed || CurrentState == ETaskState::Cancelled ||
		       CurrentState == ETaskState::Failed;
	}

public:
	// === 结果访问 ===

	/**
	 * @brief 等待任务完成并获取结果
	 */
	TResult GetResult()
	{
		Wait();

		std::lock_guard<std::mutex> Lock(ResultMutex);

		if (Exception)
		{
			std::rethrow_exception(Exception);
		}

		if (State == ETaskState::Cancelled)
		{
			throw std::runtime_error("Task was cancelled");
		}

		if (State == ETaskState::Failed)
		{
			throw std::runtime_error("Task failed");
		}

		if constexpr (!std::is_void_v<TResult>)
		{
			return TaskResult;
		}
	}

	/**
	 * @brief 等待任务完成
	 */
	void Wait()
	{
		std::unique_lock<std::mutex> Lock(WaitMutex);
		CompletionCondition.wait(Lock, [this]() { return IsCompleted(); });
	}

	/**
	 * @brief 带超时的等待
	 */
	bool WaitFor(const CTimespan& Timeout)
	{
		std::unique_lock<std::mutex> Lock(WaitMutex);
		auto TimeoutDuration = std::chrono::nanoseconds(Timeout.GetTicks() * 100);
		return CompletionCondition.wait_for(Lock, TimeoutDuration, [this]() { return IsCompleted(); });
	}

	/**
	 * @brief 尝试获取结果（非阻塞）
	 */
	bool TryGetResult(TResult& OutResult)
	{
		if (!IsCompleted())
		{
			return false;
		}

		std::lock_guard<std::mutex> Lock(ResultMutex);

		if (Exception || State == ETaskState::Cancelled || State == ETaskState::Failed)
		{
			return false;
		}

		if constexpr (!std::is_void_v<TResult>)
		{
			OutResult = TaskResult;
		}

		return true;
	}

public:
	// === 任务链式操作 ===

	/**
	 * @brief 任务完成后继续执行另一个任务
	 */
	template <typename TFunc>
	auto Then(TFunc&& Continuation) -> TSharedPtr<TTask<std::invoke_result_t<TFunc, TResult>>>
	{
		using TContinuationResult = std::invoke_result_t<TFunc, TResult>;

		auto ContinuationTask = MakeShared<TTask<TContinuationResult>>(
		    [this, Continuation = std::forward<TFunc>(Continuation)]() -> TContinuationResult {
			    TResult Result = this->GetResult();
			    if constexpr (std::is_void_v<TResult>)
			    {
				    return Continuation();
			    }
			    else
			    {
				    return Continuation(Result);
			    }
		    },
		    CString("ContinuationTask"),
		    Priority);

		return ContinuationTask;
	}

public:
	// === 状态和属性 ===

	/**
	 * @brief 获取执行上下文
	 */
	const STaskContext& GetContext() const
	{
		return Context;
	}

	/**
	 * @brief 获取创建时间
	 */
	CDateTime GetCreationTime() const
	{
		return CreationTime;
	}

	/**
	 * @brief 检查是否被取消
	 */
	bool IsCancelled() const
	{
		return State.load() == ETaskState::Cancelled;
	}

	/**
	 * @brief 检查是否执行失败
	 */
	bool IsFailed() const
	{
		return State.load() == ETaskState::Failed;
	}

	/**
	 * @brief 设置为等待状态
	 */
	void SetPending()
	{
		TrySetState(ETaskState::Created, ETaskState::Pending);
	}

public:
	// === 委托事件 ===

	FOnTaskStarted OnTaskStarted;
	FOnTaskCompleted OnTaskCompleted;
	FOnTaskFailed OnTaskFailed;
	FOnTaskCancelled OnTaskCancelled;

private:
	// === 内部实现 ===

	/**
	 * @brief 尝试设置状态
	 */
	bool TrySetState(ETaskState Expected, ETaskState Desired)
	{
		return State.compare_exchange_strong(Expected, Desired);
	}

	/**
	 * @brief 获取状态字符串
	 */
	CString GetStateString() const
	{
		switch (State.load())
		{
		case ETaskState::Created:
			return CString("Created");
		case ETaskState::Pending:
			return CString("Pending");
		case ETaskState::Running:
			return CString("Running");
		case ETaskState::Completed:
			return CString("Completed");
		case ETaskState::Cancelled:
			return CString("Cancelled");
		case ETaskState::Failed:
			return CString("Failed");
		default:
			return CString("Unknown");
		}
	}

private:
	// === 成员变量 ===

	FunctionType Function;  // 任务函数
	CString TaskName;       // 任务名称
	ETaskPriority Priority; // 任务优先级
	uint64_t TaskId;        // 任务ID

	std::atomic<ETaskState> State;            // 任务状态
	std::atomic<bool> bCancellationRequested; // 是否请求取消

	STaskContext Context;   // 执行上下文
	CDateTime CreationTime; // 创建时间

	// 结果存储
	mutable std::mutex ResultMutex; // 结果互斥锁
	TResult TaskResult;             // 任务结果
	std::exception_ptr Exception;   // 异常指针

	// 等待同步
	std::mutex WaitMutex;                        // 等待互斥锁
	std::condition_variable CompletionCondition; // 完成条件变量
};

// 静态成员初始化
template <typename TResult>
std::atomic<uint64_t> TTask<TResult>::NextTaskId{1};

/**
 * @brief Future类 - 异步操作的结果持有者
 */
template <typename T>
class TFuture
{
public:
	TFuture() = default;

	explicit TFuture(TSharedPtr<TTask<T>> InTask)
	    : Task(InTask)
	{}

	// 移动语义
	TFuture(TFuture&& Other) noexcept
	    : Task(std::move(Other.Task))
	{}

	TFuture& operator=(TFuture&& Other) noexcept
	{
		if (this != &Other)
		{
			Task = std::move(Other.Task);
		}
		return *this;
	}

	// 禁用拷贝
	TFuture(const TFuture&) = delete;
	TFuture& operator=(const TFuture&) = delete;

public:
	// === 结果访问 ===

	/**
	 * @brief 获取结果（阻塞）
	 */
	T Get()
	{
		if (!Task.IsValid())
		{
			throw std::future_error(std::future_errc::no_state);
		}

		return Task->GetResult();
	}

	/**
	 * @brief 等待完成
	 */
	void Wait()
	{
		if (Task.IsValid())
		{
			Task->Wait();
		}
	}

	/**
	 * @brief 带超时等待
	 */
	bool WaitFor(const CTimespan& Timeout)
	{
		if (Task.IsValid())
		{
			return Task->WaitFor(Timeout);
		}
		return false;
	}

	/**
	 * @brief 检查是否就绪
	 */
	bool IsReady() const
	{
		return Task.IsValid() && Task->IsCompleted();
	}

	/**
	 * @brief 检查是否有效
	 */
	bool IsValid() const
	{
		return Task.IsValid();
	}

public:
	// === 链式操作 ===

	/**
	 * @brief 链接后续操作
	 */
	template <typename TFunc>
	auto Then(TFunc&& Continuation) -> TFuture<std::invoke_result_t<TFunc, T>>
	{
		if (!Task.IsValid())
		{
			throw std::future_error(std::future_errc::no_state);
		}

		auto ContinuationTask = Task->Then(std::forward<TFunc>(Continuation));
		return TFuture<std::invoke_result_t<TFunc, T>>(ContinuationTask);
	}

private:
	TSharedPtr<TTask<T>> Task;
};

/**
 * @brief Promise类 - 异步操作的结果设置者
 */
template <typename T>
class TPromise
{
public:
	TPromise()
	{
		Task = MakeShared<TTask<T>>(
		    [this]() -> T {
			    std::unique_lock<std::mutex> Lock(PromiseMutex);
			    PromiseCondition.wait(Lock, [this]() { return bValueSet || bExceptionSet; });

			    if (bExceptionSet)
			    {
				    std::rethrow_exception(Exception);
			    }

			    if constexpr (!std::is_void_v<T>)
			    {
				    return Value;
			    }
		    },
		    CString("PromiseTask"));
	}

	// 移动语义
	TPromise(TPromise&& Other) noexcept
	    : Task(std::move(Other.Task)),
	      bValueSet(Other.bValueSet.load()),
	      bExceptionSet(Other.bExceptionSet.load())
	{
		if constexpr (!std::is_void_v<T>)
		{
			Value = std::move(Other.Value);
		}
		Exception = std::move(Other.Exception);
	}

	TPromise& operator=(TPromise&& Other) noexcept
	{
		if (this != &Other)
		{
			Task = std::move(Other.Task);
			bValueSet = Other.bValueSet.load();
			bExceptionSet = Other.bExceptionSet.load();
			if constexpr (!std::is_void_v<T>)
			{
				Value = std::move(Other.Value);
			}
			Exception = std::move(Other.Exception);
		}
		return *this;
	}

	// 禁用拷贝
	TPromise(const TPromise&) = delete;
	TPromise& operator=(const TPromise&) = delete;

public:
	// === 值设置 ===

	/**
	 * @brief 设置值
	 */
	void SetValue(const T& InValue)
	    requires(!std::is_void_v<T>)
	{
		std::lock_guard<std::mutex> Lock(PromiseMutex);

		if (bValueSet || bExceptionSet)
		{
			throw std::future_error(std::future_errc::promise_already_satisfied);
		}

		Value = InValue;
		bValueSet = true;
		PromiseCondition.notify_all();
	}

	void SetValue(T&& InValue)
	    requires(!std::is_void_v<T>)
	{
		std::lock_guard<std::mutex> Lock(PromiseMutex);

		if (bValueSet || bExceptionSet)
		{
			throw std::future_error(std::future_errc::promise_already_satisfied);
		}

		Value = std::move(InValue);
		bValueSet = true;
		PromiseCondition.notify_all();
	}

	void SetValue()
	    requires(std::is_void_v<T>)
	{
		std::lock_guard<std::mutex> Lock(PromiseMutex);

		if (bValueSet || bExceptionSet)
		{
			throw std::future_error(std::future_errc::promise_already_satisfied);
		}

		bValueSet = true;
		PromiseCondition.notify_all();
	}

	/**
	 * @brief 设置异常
	 */
	void SetException(std::exception_ptr InException)
	{
		std::lock_guard<std::mutex> Lock(PromiseMutex);

		if (bValueSet || bExceptionSet)
		{
			throw std::future_error(std::future_errc::promise_already_satisfied);
		}

		Exception = InException;
		bExceptionSet = true;
		PromiseCondition.notify_all();
	}

	/**
	 * @brief 获取Future
	 */
	TFuture<T> GetFuture()
	{
		if (!Task.IsValid())
		{
			throw std::future_error(std::future_errc::no_state);
		}

		return TFuture<T>(Task);
	}

private:
	TSharedPtr<TTask<T>> Task;

	std::mutex PromiseMutex;
	std::condition_variable PromiseCondition;

	std::atomic<bool> bValueSet{false};
	std::atomic<bool> bExceptionSet{false};

	T Value;
	std::exception_ptr Exception;
};

// === 类型别名 ===
template <typename T>
using FTask = TTask<T>;

template <typename T>
using FFuture = TFuture<T>;

template <typename T>
using FPromise = TPromise<T>;

// === 工厂函数 ===

/**
 * @brief 创建任务
 */
template <typename TFunc>
auto CreateTask(TFunc&& Function,
                const CString& TaskName = CString("Task"),
                ETaskPriority Priority = ETaskPriority::Normal)
{
	using ResultType = std::invoke_result_t<TFunc>;
	return MakeShared<TTask<ResultType>>(std::forward<TFunc>(Function), TaskName, Priority);
}

/**
 * @brief 创建异步任务
 */
template <typename TFunc>
auto Async(TFunc&& Function, const CString& TaskName = CString("AsyncTask"))
{
	using ResultType = std::invoke_result_t<TFunc>;
	auto Task = CreateTask(std::forward<TFunc>(Function), TaskName);
	return TFuture<ResultType>(Task);
}

} // namespace NLib