#pragma once

#include "Containers/TString.h"
#include "Core/Object.h"
#include "Core/SmartPointers.h"
#include "Logging/LogCategory.h"
#include "Task.h"
#include "ThreadPool.h"

#include <functional>
#include <future>

namespace NLib
{
/**
 * @brief 异步执行策略枚举
 */
enum class EAsyncPolicy : uint8_t
{
	ThreadPool,        // 使用线程池执行
	NewThread,         // 创建新线程执行
	DeferredExecution, // 延迟执行
	CurrentThread      // 当前线程执行（用于测试）
};

/**
 * @brief 异步操作配置
 */
struct SAsyncConfig
{
	EAsyncPolicy Policy = EAsyncPolicy::ThreadPool;     // 执行策略
	EThreadPriority Priority = EThreadPriority::Normal; // 线程优先级
	TString TaskName = TString("AsyncTask");            // 任务名称
	TSharedPtr<CThreadPool> CustomThreadPool = nullptr; // 自定义线程池

	SAsyncConfig() = default;

	SAsyncConfig(EAsyncPolicy InPolicy, const TString& InTaskName = TString("AsyncTask"))
	    : Policy(InPolicy),
	      TaskName(InTaskName)
	{}
};

/**
 * @brief 异步操作工具类
 *
 * 提供简单易用的异步操作接口：
 * - 异步函数执行
 * - 并行操作
 * - 延迟执行
 * - 结果组合
 */
class CAsync
{
public:
	// === 基础异步操作 ===

	/**
	 * @brief 异步执行函数
	 */
	template <typename TFunc>
	static auto Run(TFunc&& Function, const SAsyncConfig& Config = SAsyncConfig{})
	    -> TFuture<std::invoke_result_t<TFunc>>
	{
		using ResultType = std::invoke_result_t<TFunc>;

		switch (Config.Policy)
		{
		case EAsyncPolicy::ThreadPool:
			return RunOnThreadPool(std::forward<TFunc>(Function), Config);

		case EAsyncPolicy::NewThread:
			return RunOnNewThread(std::forward<TFunc>(Function), Config);

		case EAsyncPolicy::DeferredExecution:
			return RunDeferred(std::forward<TFunc>(Function), Config);

		case EAsyncPolicy::CurrentThread:
			return RunOnCurrentThread(std::forward<TFunc>(Function), Config);

		default:
			NLOG_THREADING(Error, "Unknown async policy: {}", static_cast<int>(Config.Policy));
			return TFuture<ResultType>();
		}
	}

	/**
	 * @brief 异步执行对象成员函数
	 */
	template <typename TObject, typename TFunc>
	static auto Run(TObject* Object, TFunc&& Function, const SAsyncConfig& Config = SAsyncConfig{})
	    -> TFuture<std::invoke_result_t<TFunc, TObject*>>
	{
		if (!Object)
		{
			NLOG_THREADING(Error, "Cannot run async operation with null object");
			using ResultType = std::invoke_result_t<TFunc, TObject*>;
			return TFuture<ResultType>();
		}

		return Run(
		    [Object, Function]() -> std::invoke_result_t<TFunc, TObject*> {
			    if constexpr (std::is_void_v<std::invoke_result_t<TFunc, TObject*>>)
			    {
				    (Object->*Function)();
			    }
			    else
			    {
				    return (Object->*Function)();
			    }
		    },
		    Config);
	}

	/**
	 * @brief 异步执行智能指针对象成员函数
	 */
	template <typename TObject, typename TFunc>
	static auto Run(TSharedPtr<TObject> Object, TFunc&& Function, const SAsyncConfig& Config = SAsyncConfig{})
	    -> TFuture<std::invoke_result_t<TFunc, TObject*>>
	{
		if (!Object.IsValid())
		{
			NLOG_THREADING(Error, "Cannot run async operation with invalid shared pointer");
			using ResultType = std::invoke_result_t<TFunc, TObject*>;
			return TFuture<ResultType>();
		}

		return Run(
		    [Object, Function]() -> std::invoke_result_t<TFunc, TObject*> {
			    if constexpr (std::is_void_v<std::invoke_result_t<TFunc, TObject*>>)
			    {
				    (Object.Get()->*Function)();
			    }
			    else
			    {
				    return (Object.Get()->*Function)();
			    }
		    },
		    Config);
	}

public:
	// === 并行操作 ===

	/**
	 * @brief 并行执行多个函数
	 */
	template <typename... TFuncs>
	static auto RunParallel(TFuncs&&... Functions)
	{
		return std::make_tuple(Run(std::forward<TFuncs>(Functions))...);
	}

	/**
	 * @brief 并行执行数组中的函数
	 */
	template <typename TFunc>
	static TArray<TFuture<std::invoke_result_t<TFunc>>, CMemoryManager> RunParallel(
	    const TArray<TFunc, CMemoryManager>& Functions, const SAsyncConfig& Config = SAsyncConfig{})
	{
		using ResultType = std::invoke_result_t<TFunc>;
		TArray<TFuture<ResultType>, CMemoryManager> Futures;
		Futures.Reserve(Functions.Size());

		for (const auto& Function : Functions)
		{
			Futures.Add(Run(Function, Config));
		}

		return Futures;
	}

	/**
	 * @brief 等待所有Future完成
	 */
	template <typename TResult>
	static void WaitAll(const TArray<TFuture<TResult>, CMemoryManager>& Futures,
	                    const CTimespan& Timeout = CTimespan::Zero())
	{
		for (const auto& Future : Futures)
		{
			if (Timeout.IsZero())
			{
				Future.Wait();
			}
			else
			{
				Future.WaitFor(Timeout);
			}
		}
	}

	/**
	 * @brief 等待任意一个Future完成
	 */
	template <typename TResult>
	static int32_t WaitAny(const TArray<TFuture<TResult>, CMemoryManager>& Futures,
	                       const CTimespan& Timeout = CTimespan::Zero())
	{
		if (Futures.IsEmpty())
		{
			return -1;
		}

		CClock StartTime;

		while (true)
		{
			for (int32_t i = 0; i < Futures.Size(); ++i)
			{
				if (Futures[i].IsReady())
				{
					return i;
				}
			}

			// 检查超时
			if (!Timeout.IsZero() && StartTime.GetElapsed() >= Timeout)
			{
				return -1;
			}

			// 短暂休眠避免忙等待
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}

public:
	// === 延迟和重试操作 ===

	/**
	 * @brief 延迟执行
	 */
	template <typename TFunc>
	static auto Delay(TFunc&& Function, const CTimespan& DelayTime, const SAsyncConfig& Config = SAsyncConfig{})
	    -> TFuture<std::invoke_result_t<TFunc>>
	{
		return Run(
		    [Function = std::forward<TFunc>(Function), DelayTime]() -> std::invoke_result_t<TFunc> {
			    auto SleepDuration = std::chrono::nanoseconds(DelayTime.GetTicks() * 100);
			    std::this_thread::sleep_for(SleepDuration);

			    if constexpr (std::is_void_v<std::invoke_result_t<TFunc>>)
			    {
				    Function();
			    }
			    else
			    {
				    return Function();
			    }
		    },
		    Config);
	}

	/**
	 * @brief 带重试的异步执行
	 */
	template <typename TFunc>
	static auto RunWithRetry(TFunc&& Function,
	                         uint32_t MaxRetries,
	                         const CTimespan& RetryInterval = CTimespan::FromSeconds(1),
	                         const SAsyncConfig& Config = SAsyncConfig{}) -> TFuture<std::invoke_result_t<TFunc>>
	{
		using ResultType = std::invoke_result_t<TFunc>;

		return Run(
		    [Function = std::forward<TFunc>(Function), MaxRetries, RetryInterval]() -> ResultType {
			    uint32_t Attempts = 0;

			    while (true)
			    {
				    try
				    {
					    if constexpr (std::is_void_v<ResultType>)
					    {
						    Function();
						    return;
					    }
					    else
					    {
						    return Function();
					    }
				    }
				    catch (const std::exception& e)
				    {
					    Attempts++;

					    if (Attempts > MaxRetries)
					    {
						    NLOG_THREADING(Error, "Function failed after {} attempts: {}", MaxRetries, e.what());
						    throw;
					    }

					    NLOG_THREADING(Warning, "Function failed, attempt {}/{}: {}", Attempts, MaxRetries, e.what());

					    // 等待重试间隔
					    auto SleepDuration = std::chrono::nanoseconds(RetryInterval.GetTicks() * 100);
					    std::this_thread::sleep_for(SleepDuration);
				    }
			    }
		    },
		    Config);
	}

public:
	// === 结果组合操作 ===

	/**
	 * @brief 组合两个Future的结果
	 */
	template <typename T1, typename T2, typename TFunc>
	static auto Combine(TFuture<T1>&& Future1, TFuture<T2>&& Future2, TFunc&& CombineFunc)
	    -> TFuture<std::invoke_result_t<TFunc, T1, T2>>
	{
		using ResultType = std::invoke_result_t<TFunc, T1, T2>;

		return Run([Future1 = std::move(Future1),
		            Future2 = std::move(Future2),
		            CombineFunc = std::forward<TFunc>(CombineFunc)]() mutable -> ResultType {
			T1 Result1 = Future1.Get();
			T2 Result2 = Future2.Get();

			if constexpr (std::is_void_v<ResultType>)
			{
				CombineFunc(Result1, Result2);
			}
			else
			{
				return CombineFunc(Result1, Result2);
			}
		});
	}

	/**
	 * @brief 链式操作
	 */
	template <typename TResult, typename TFunc>
	static auto Then(TFuture<TResult>&& Future, TFunc&& ContinuationFunc)
	    -> TFuture<std::invoke_result_t<TFunc, TResult>>
	{
		return Future.Then(std::forward<TFunc>(ContinuationFunc));
	}

public:
	// === 线程池管理 ===

	/**
	 * @brief 设置默认线程池
	 */
	static void SetDefaultThreadPool(TSharedPtr<CThreadPool> ThreadPool)
	{
		std::lock_guard<std::mutex> Lock(DefaultPoolMutex);
		DefaultThreadPool = ThreadPool;
		NLOG_THREADING(Info, "Default thread pool set");
	}

	/**
	 * @brief 获取默认线程池
	 */
	static TSharedPtr<CThreadPool> GetDefaultThreadPool()
	{
		std::lock_guard<std::mutex> Lock(DefaultPoolMutex);

		if (!DefaultThreadPool.IsValid())
		{
			// 创建默认线程池
			SThreadPoolConfig Config;
			Config.MinThreads = std::thread::hardware_concurrency();
			Config.MaxThreads = std::thread::hardware_concurrency() * 2;
			Config.bPrestart = true;

			DefaultThreadPool = CreateThreadPool(Config);
			NLOG_THREADING(Info, "Created default thread pool with {} threads", Config.MinThreads);
		}

		return DefaultThreadPool;
	}

private:
	// === 内部实现 ===

	/**
	 * @brief 在线程池上执行
	 */
	template <typename TFunc>
	static auto RunOnThreadPool(TFunc&& Function, const SAsyncConfig& Config) -> TFuture<std::invoke_result_t<TFunc>>
	{
		auto Pool = Config.CustomThreadPool.IsValid() ? Config.CustomThreadPool : GetDefaultThreadPool();

		if (!Pool.IsValid())
		{
			NLOG_THREADING(Error, "No available thread pool for async execution");
			using ResultType = std::invoke_result_t<TFunc>;
			return TFuture<ResultType>();
		}

		return Pool->SubmitTask(std::forward<TFunc>(Function), Config.TaskName);
	}

	/**
	 * @brief 在新线程上执行
	 */
	template <typename TFunc>
	static auto RunOnNewThread(TFunc&& Function, const SAsyncConfig& Config) -> TFuture<std::invoke_result_t<TFunc>>
	{
		using ResultType = std::invoke_result_t<TFunc>;

		auto Task = CreateTask(std::forward<TFunc>(Function), Config.TaskName);
		auto Future = TFuture<ResultType>(Task);

		// 创建新线程执行
		auto Thread = CreateThread(Config.TaskName.GetData(), [Task]() { Task->Execute(); }, Config.Priority);

		if (!Thread.IsValid())
		{
			NLOG_THREADING(Error, "Failed to create new thread for async execution");
			return TFuture<ResultType>();
		}

		// 分离线程，让其自动清理
		Thread->Detach();

		return Future;
	}

	/**
	 * @brief 延迟执行
	 */
	template <typename TFunc>
	static auto RunDeferred(TFunc&& Function, const SAsyncConfig& Config) -> TFuture<std::invoke_result_t<TFunc>>
	{
		using ResultType = std::invoke_result_t<TFunc>;

		// 创建Promise和Future
		auto Promise = TPromise<ResultType>();
		auto Future = Promise.GetFuture();

		// 创建延迟执行的任务（实际上立即执行但存储在Future中）
		try
		{
			if constexpr (std::is_void_v<ResultType>)
			{
				Function();
				Promise.SetValue();
			}
			else
			{
				auto Result = Function();
				Promise.SetValue(std::move(Result));
			}
		}
		catch (...)
		{
			Promise.SetException(std::current_exception());
		}

		return Future;
	}

	/**
	 * @brief 在当前线程执行
	 */
	template <typename TFunc>
	static auto RunOnCurrentThread(TFunc&& Function, const SAsyncConfig& Config) -> TFuture<std::invoke_result_t<TFunc>>
	{
		using ResultType = std::invoke_result_t<TFunc>;

		auto Promise = TPromise<ResultType>();
		auto Future = Promise.GetFuture();

		try
		{
			NLOG_THREADING(Trace, "Executing task '{}' on current thread", Config.TaskName.GetData());

			if constexpr (std::is_void_v<ResultType>)
			{
				Function();
				Promise.SetValue();
			}
			else
			{
				auto Result = Function();
				Promise.SetValue(std::move(Result));
			}
		}
		catch (...)
		{
			Promise.SetException(std::current_exception());
		}

		return Future;
	}

private:
	// === 静态成员变量 ===

	static TSharedPtr<CThreadPool> DefaultThreadPool;
	static std::mutex DefaultPoolMutex;
};

// 静态成员初始化
inline TSharedPtr<CThreadPool> CAsync::DefaultThreadPool = nullptr;
inline std::mutex CAsync::DefaultPoolMutex;

// === 便捷函数 ===

/**
 * @brief 快速异步执行
 */
template <typename TFunc>
auto Async(TFunc&& Function, const TString& TaskName = TString("AsyncTask"))
{
	SAsyncConfig Config(EAsyncPolicy::ThreadPool, TaskName);
	return CAsync::Run(std::forward<TFunc>(Function), Config);
}

/**
 * @brief 快速并行执行
 */
template <typename... TFuncs>
auto AsyncParallel(TFuncs&&... Functions)
{
	return CAsync::RunParallel(std::forward<TFuncs>(Functions)...);
}

/**
 * @brief 快速延迟执行
 */
template <typename TFunc>
auto AsyncDelay(TFunc&& Function, const CTimespan& DelayTime, const TString& TaskName = TString("DelayedTask"))
{
	SAsyncConfig Config(EAsyncPolicy::ThreadPool, TaskName);
	return CAsync::Delay(std::forward<TFunc>(Function), DelayTime, Config);
}

/**
 * @brief 快速重试执行
 */
template <typename TFunc>
auto AsyncRetry(TFunc&& Function, uint32_t MaxRetries, const CTimespan& RetryInterval = CTimespan::FromSeconds(1))
{
	return CAsync::RunWithRetry(std::forward<TFunc>(Function), MaxRetries, RetryInterval);
}

// === 类型别名 ===
using FAsync = CAsync;

} // namespace NLib