#pragma once

#include "Containers/TString.h"
#include "Core/Object.h"
#include "Events/Delegate.h"
#include "Logging/LogCategory.h"
#include "Time/TimeTypes.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

#include "Thread.generate.h"

namespace NLib
{
/**
 * @brief 线程状态枚举
 */
enum class EThreadState : uint8_t
{
	NotStarted, // 未启动
	Running,    // 运行中
	Paused,     // 暂停
	Stopping,   // 正在停止
	Stopped,    // 已停止
	Finished    // 已完成
};

/**
 * @brief 线程优先级枚举
 */
enum class EThreadPriority : uint8_t
{
	Lowest = 0,
	BelowNormal = 1,
	Normal = 2,
	AboveNormal = 3,
	Highest = 4,
	Critical = 5
};

/**
 * @brief 可运行接口
 *
 * 定义了可在线程中执行的对象接口
 */
class IRunnable
{
public:
	virtual ~IRunnable() = default;

	/**
	 * @brief 初始化运行环境
	 * @return 是否初始化成功
	 */
	virtual bool Initialize()
	{
		return true;
	}

	/**
	 * @brief 执行主要工作
	 * @return 返回值（0表示成功）
	 */
	virtual uint32_t Run() = 0;

	/**
	 * @brief 停止执行
	 */
	virtual void Stop()
	{}

	/**
	 * @brief 清理资源
	 */
	virtual void Cleanup()
	{}

	/**
	 * @brief 获取运行对象名称
	 */
	virtual const char* GetRunnableName() const
	{
		return "IRunnable";
	}
};

/**
 * @brief 函数式Runnable包装器
 */
class CFunctionRunnable : public IRunnable
{
public:
	using FunctionType = std::function<uint32_t()>;
	using InitFunction = std::function<bool()>;
	using StopFunction = std::function<void()>;
	using CleanupFunction = std::function<void()>;

public:
	CFunctionRunnable(FunctionType InFunction, const char* InName = "FunctionRunnable")
	    : Function(std::move(InFunction)),
	      Name(InName)
	{}

	CFunctionRunnable(FunctionType InFunction,
	                  InitFunction InInitFunc,
	                  StopFunction InStopFunc,
	                  CleanupFunction InCleanupFunc,
	                  const char* InName = "FunctionRunnable")
	    : Function(std::move(InFunction)),
	      InitFunc(std::move(InInitFunc)),
	      StopFunc(std::move(InStopFunc)),
	      CleanupFunc(std::move(InCleanupFunc)),
	      Name(InName)
	{}

public:
	bool Initialize() override
	{
		return InitFunc ? InitFunc() : true;
	}

	uint32_t Run() override
	{
		return Function ? Function() : 0;
	}

	void Stop() override
	{
		if (StopFunc)
		{
			StopFunc();
		}
	}

	void Cleanup() override
	{
		if (CleanupFunc)
		{
			CleanupFunc();
		}
	}

	const char* GetRunnableName() const override
	{
		return Name.GetData();
	}

private:
	FunctionType Function;
	InitFunction InitFunc;
	StopFunction StopFunc;
	CleanupFunction CleanupFunc;
	CString Name;
};

/**
 * @brief 线程包装类
 *
 * 提供对std::thread的高级封装，支持：
 * - 线程状态管理
 * - 线程优先级设置
 * - 暂停和恢复
 * - 统计信息收集
 */
NCLASS()
class NThread : public NObject
{
	GENERATED_BODY()

public:
	// === 委托定义 ===
	DECLARE_DELEGATE(FOnThreadStarted);
	DECLARE_DELEGATE(FOnThreadFinished, uint32_t);
	DECLARE_DELEGATE(FOnThreadError, const CString&);

public:
	// === 构造函数 ===

	/**
	 * @brief 构造函数
	 */
	NThread(const char* InThreadName = "NLibThread")
	    : ThreadName(InThreadName),
	      State(EThreadState::NotStarted),
	      Priority(EThreadPriority::Normal),
	      ThreadId(0),
	      ExitCode(0),
	      bShouldStop(false),
	      bIsPaused(false),
	      CreationTime(CDateTime::Now())
	{}

	/**
	 * @brief 析构函数
	 */
	virtual ~NThread()
	{
		if (IsRunning())
		{
			Stop();
			Join();
		}
	}

public:
	// === 线程启动 ===

	/**
	 * @brief 启动线程（使用Runnable）
	 */
	bool Start(TSharedPtr<IRunnable> InRunnable, EThreadPriority InPriority = EThreadPriority::Normal)
	{
		if (State != EThreadState::NotStarted)
		{
			NLOG_THREADING(Error, "Thread '{}' is already started", ThreadName.GetData());
			return false;
		}

		if (!InRunnable.IsValid())
		{
			NLOG_THREADING(Error, "Cannot start thread '{}' with null runnable", ThreadName.GetData());
			return false;
		}

		Runnable = InRunnable;
		Priority = InPriority;

		return StartInternal();
	}

	/**
	 * @brief 启动线程（使用函数）
	 */
	template <typename TFunc>
	bool Start(TFunc&& Function, EThreadPriority InPriority = EThreadPriority::Normal)
	{
		auto FuncRunnable = MakeShared<CFunctionRunnable>(
		    [Function = std::forward<TFunc>(Function)]() -> uint32_t {
			    if constexpr (std::is_void_v<std::invoke_result_t<TFunc>>)
			    {
				    Function();
				    return 0;
			    }
			    else
			    {
				    return static_cast<uint32_t>(Function());
			    }
		    },
		    ThreadName.GetData());

		return Start(FuncRunnable, InPriority);
	}

	/**
	 * @brief 启动线程（使用成员函数）
	 */
	template <typename TObject, typename TFunc>
	bool Start(TObject* Object, TFunc&& Function, EThreadPriority InPriority = EThreadPriority::Normal)
	{
		if (!Object)
		{
			NLOG_THREADING(Error, "Cannot start thread '{}' with null object", ThreadName.GetData());
			return false;
		}

		auto FuncRunnable = MakeShared<CFunctionRunnable>(
		    [Object, Function]() -> uint32_t {
			    if constexpr (std::is_void_v<std::invoke_result_t<TFunc, TObject*>>)
			    {
				    (Object->*Function)();
				    return 0;
			    }
			    else
			    {
				    return static_cast<uint32_t>((Object->*Function)());
			    }
		    },
		    ThreadName.GetData());

		return Start(FuncRunnable, InPriority);
	}

public:
	// === 线程控制 ===

	/**
	 * @brief 停止线程
	 */
	void Stop()
	{
		if (State == EThreadState::Running || State == EThreadState::Paused)
		{
			bShouldStop = true;
			State = EThreadState::Stopping;

			// 唤醒可能暂停的线程
			if (bIsPaused)
			{
				Resume();
			}

			// 通知Runnable停止
			if (Runnable.IsValid())
			{
				Runnable->Stop();
			}

			NLOG_THREADING(Debug, "Thread '{}' stop requested", ThreadName.GetData());
		}
	}

	/**
	 * @brief 暂停线程
	 */
	void Pause()
	{
		if (State == EThreadState::Running)
		{
			bIsPaused = true;
			State = EThreadState::Paused;
			NLOG_THREADING(Debug, "Thread '{}' paused", ThreadName.GetData());
		}
	}

	/**
	 * @brief 恢复线程
	 */
	void Resume()
	{
		if (State == EThreadState::Paused)
		{
			bIsPaused = false;
			State = EThreadState::Running;
			PauseCondition.notify_all();
			NLOG_THREADING(Debug, "Thread '{}' resumed", ThreadName.GetData());
		}
	}

	/**
	 * @brief 等待线程结束
	 */
	bool Join(const CTimespan& Timeout = CTimespan::Zero)
	{
		if (!Thread.joinable())
		{
			return true;
		}

		if (Timeout.IsZero())
		{
			Thread.join();
			return true;
		}
		else
		{
			// 带超时的等待
			auto TimeoutDuration = std::chrono::nanoseconds(Timeout.GetTicks() * 100);
			std::unique_lock<std::mutex> Lock(JoinMutex);

			return JoinCondition.wait_for(Lock, TimeoutDuration, [this]() {
				return State == EThreadState::Finished || State == EThreadState::Stopped;
			});
		}
	}

	/**
	 * @brief 分离线程
	 */
	void Detach()
	{
		if (Thread.joinable())
		{
			Thread.detach();
			NLOG_THREADING(Debug, "Thread '{}' detached", ThreadName.GetData());
		}
	}

public:
	// === 状态查询 ===

	/**
	 * @brief 检查线程是否正在运行
	 */
	bool IsRunning() const
	{
		return State == EThreadState::Running || State == EThreadState::Paused;
	}

	/**
	 * @brief 检查线程是否已完成
	 */
	bool IsFinished() const
	{
		return State == EThreadState::Finished || State == EThreadState::Stopped;
	}

	/**
	 * @brief 检查线程是否暂停
	 */
	bool IsPaused() const
	{
		return State == EThreadState::Paused;
	}

	/**
	 * @brief 检查是否应该停止
	 */
	bool ShouldStop() const
	{
		return bShouldStop.load();
	}

	/**
	 * @brief 获取线程状态
	 */
	EThreadState GetState() const
	{
		return State.load();
	}

	/**
	 * @brief 获取线程ID
	 */
	std::thread::id GetThreadId() const
	{
		return ThreadId;
	}

	/**
	 * @brief 获取退出码
	 */
	uint32_t GetExitCode() const
	{
		return ExitCode.load();
	}

public:
	// === 属性访问 ===

	/**
	 * @brief 获取线程名称
	 */
	const CString& GetThreadName() const
	{
		return ThreadName;
	}

	/**
	 * @brief 设置线程名称
	 */
	void SetThreadName(const char* NewName)
	{
		ThreadName = NewName;
	}

	/**
	 * @brief 获取线程优先级
	 */
	EThreadPriority GetPriority() const
	{
		return Priority;
	}

	/**
	 * @brief 获取创建时间
	 */
	CDateTime GetCreationTime() const
	{
		return CreationTime;
	}

	/**
	 * @brief 获取运行时间
	 */
	CTimespan GetRunTime() const
	{
		if (State == EThreadState::NotStarted)
		{
			return CTimespan::Zero;
		}

		CDateTime EndTime = IsFinished() ? FinishTime : CDateTime::Now();
		return EndTime - StartTime;
	}

public:
	// === 委托事件 ===

	FOnThreadStarted OnThreadStarted;
	FOnThreadFinished OnThreadFinished;
	FOnThreadError OnThreadError;

public:
	// === 静态方法 ===

	/**
	 * @brief 获取当前线程ID
	 */
	static std::thread::id GetCurrentThreadId()
	{
		return std::this_thread::get_id();
	}

	/**
	 * @brief 当前线程睡眠
	 */
	static void Sleep(const CTimespan& Duration)
	{
		auto SleepDuration = std::chrono::nanoseconds(Duration.GetTicks() * 100);
		std::this_thread::sleep_for(SleepDuration);
	}

	/**
	 * @brief 当前线程让出CPU
	 */
	static void Yield()
	{
		std::this_thread::yield();
	}

	/**
	 * @brief 获取硬件并发数
	 */
	static uint32_t GetHardwareConcurrency()
	{
		return std::thread::hardware_concurrency();
	}

protected:
	// === 内部实现 ===

	/**
	 * @brief 内部启动实现
	 */
	bool StartInternal()
	{
		try
		{
			Thread = std::thread([this]() { ThreadMain(); });

			State = EThreadState::Running;
			ThreadId = Thread.get_id();
			StartTime = CDateTime::Now();

			// 设置线程优先级
			SetThreadPriority();

			NLOG_THREADING(
			    Info, "Thread '{}' started, ID: {}", ThreadName.GetData(), reinterpret_cast<uintptr_t>(&ThreadId));

			// 触发启动事件
			OnThreadStarted.ExecuteIfBound();

			return true;
		}
		catch (const std::exception& e)
		{
			NLOG_THREADING(Error, "Failed to start thread '{}': {}", ThreadName.GetData(), e.what());

			CString ErrorMsg(e.what());
			OnThreadError.ExecuteIfBound(ErrorMsg);

			return false;
		}
	}

	/**
	 * @brief 线程主函数
	 */
	void ThreadMain()
	{
		uint32_t Result = 0;

		try
		{
			// 初始化Runnable
			if (Runnable.IsValid())
			{
				if (!Runnable->Initialize())
				{
					NLOG_THREADING(Error, "Thread '{}' runnable initialization failed", ThreadName.GetData());
					Result = 1;
					goto cleanup;
				}
			}

			// 主执行循环
			while (!ShouldStop())
			{
				// 检查暂停状态
				if (bIsPaused)
				{
					std::unique_lock<std::mutex> Lock(PauseMutex);
					PauseCondition.wait(Lock, [this]() { return !bIsPaused || ShouldStop(); });

					if (ShouldStop())
					{
						break;
					}
				}

				// 执行Runnable
				if (Runnable.IsValid())
				{
					Result = Runnable->Run();
					if (Result != 0)
					{
						break; // 非零返回值表示错误或完成
					}
				}
				else
				{
					break; // 没有Runnable，退出
				}
			}

		cleanup:
			// 清理Runnable
			if (Runnable.IsValid())
			{
				Runnable->Cleanup();
			}

			ExitCode = Result;
			FinishTime = CDateTime::Now();
			State = ShouldStop() ? EThreadState::Stopped : EThreadState::Finished;

			NLOG_THREADING(Info, "Thread '{}' finished with exit code: {}", ThreadName.GetData(), Result);

			// 触发完成事件
			OnThreadFinished.ExecuteIfBound(Result);

			// 通知等待的线程
			JoinCondition.notify_all();
		}
		catch (const std::exception& e)
		{
			NLOG_THREADING(Error, "Exception in thread '{}': {}", ThreadName.GetData(), e.what());

			ExitCode = 1;
			State = EThreadState::Stopped;
			FinishTime = CDateTime::Now();

			CString ErrorMsg(e.what());
			OnThreadError.ExecuteIfBound(ErrorMsg);

			JoinCondition.notify_all();
		}
	}

	/**
	 * @brief 设置线程优先级
	 */
	void SetThreadPriority()
	{
		// 平台特定的线程优先级设置
		// 这里提供简化实现，实际需要根据平台实现

#ifdef _WIN32
		// Windows实现
		HANDLE ThreadHandle = Thread.native_handle();
		int Priority = THREAD_PRIORITY_NORMAL;

		switch (Priority)
		{
		case EThreadPriority::Lowest:
			Priority = THREAD_PRIORITY_LOWEST;
			break;
		case EThreadPriority::BelowNormal:
			Priority = THREAD_PRIORITY_BELOW_NORMAL;
			break;
		case EThreadPriority::Normal:
			Priority = THREAD_PRIORITY_NORMAL;
			break;
		case EThreadPriority::AboveNormal:
			Priority = THREAD_PRIORITY_ABOVE_NORMAL;
			break;
		case EThreadPriority::Highest:
			Priority = THREAD_PRIORITY_HIGHEST;
			break;
		case EThreadPriority::Critical:
			Priority = THREAD_PRIORITY_TIME_CRITICAL;
			break;
		}

		SetThreadPriority(ThreadHandle, Priority);
#elif defined(__linux__) || defined(__APPLE__)
		// Unix/Linux实现
		// pthread_setschedparam等API的使用
#endif

		NLOG_THREADING(Trace, "Thread '{}' priority set to {}", ThreadName.GetData(), static_cast<int>(Priority));
	}

protected:
	// === 成员变量 ===

	CString ThreadName;              // 线程名称
	std::atomic<EThreadState> State; // 线程状态
	EThreadPriority Priority;        // 线程优先级
	std::thread Thread;              // 底层线程对象
	std::thread::id ThreadId;        // 线程ID

	TSharedPtr<IRunnable> Runnable; // 运行对象

	std::atomic<uint32_t> ExitCode; // 退出码
	std::atomic<bool> bShouldStop;  // 是否应该停止
	std::atomic<bool> bIsPaused;    // 是否暂停

	// 时间信息
	CDateTime CreationTime; // 创建时间
	CDateTime StartTime;    // 开始时间
	CDateTime FinishTime;   // 结束时间

	// 同步原语
	std::mutex PauseMutex;                  // 暂停互斥锁
	std::condition_variable PauseCondition; // 暂停条件变量
	std::mutex JoinMutex;                   // 等待互斥锁
	std::condition_variable JoinCondition;  // 等待条件变量
};

// === 类型别名 ===
using FThread = NThread;
using FRunnable = IRunnable;

// === 便捷函数 ===

/**
 * @brief 创建并启动线程
 */
template <typename TFunc>
TSharedPtr<NThread> CreateThread(const char* ThreadName,
                                 TFunc&& Function,
                                 EThreadPriority Priority = EThreadPriority::Normal)
{
	auto Thread = MakeShared<NThread>(ThreadName);
	if (Thread->Start(std::forward<TFunc>(Function), Priority))
	{
		return Thread;
	}
	return nullptr;
}

/**
 * @brief 创建并启动对象方法线程
 */
template <typename TObject, typename TFunc>
TSharedPtr<NThread> CreateThread(const char* ThreadName,
                                 TObject* Object,
                                 TFunc&& Function,
                                 EThreadPriority Priority = EThreadPriority::Normal)
{
	auto Thread = MakeShared<NThread>(ThreadName);
	if (Thread->Start(Object, std::forward<TFunc>(Function), Priority))
	{
		return Thread;
	}
	return nullptr;
}

} // namespace NLib