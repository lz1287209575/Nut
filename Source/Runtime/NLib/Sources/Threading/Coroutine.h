#pragma once

#include "Containers/TArray.h"
#include "Containers/TString.h"
#include "Core/Object.h"
#include "Core/SmartPointers.h"
#include "Coroutine.generate.h"
#include "Events/Delegate.h"
#include "Logging/LogCategory.h"
#include "Time/TimeTypes.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <setjmp.h>

namespace NLib
{
// 前向声明
class NCoroutineScheduler;
class NCoroutine;

/**
 * @brief 协程状态枚举
 */
enum class ECoroutineState : uint8_t
{
	Created,   // 已创建
	Ready,     // 就绪
	Running,   // 运行中
	Suspended, // 挂起
	Completed, // 已完成
	Failed,    // 执行失败
	Cancelled  // 已取消
};

/**
 * @brief 协程ID类型
 */
using FCoroutineId = uint64_t;
constexpr FCoroutineId INVALID_COROUTINE_ID = 0;

/**
 * @brief 协程栈大小配置
 */
constexpr size_t DEFAULT_COROUTINE_STACK_SIZE = 64 * 1024; // 64KB
constexpr size_t MIN_COROUTINE_STACK_SIZE = 4 * 1024;      // 4KB
constexpr size_t MAX_COROUTINE_STACK_SIZE = 1024 * 1024;   // 1MB

/**
 * @brief 协程上下文结构
 */
struct SCoroutineContext
{
	jmp_buf JumpBuffer;           // setjmp/longjmp缓冲区
	void* StackPointer = nullptr; // 栈指针
	size_t StackSize = 0;         // 栈大小
	void* StackBase = nullptr;    // 栈基址
	bool bIsInitialized = false;  // 是否已初始化

	SCoroutineContext() = default;

	~SCoroutineContext()
	{
		if (StackBase)
		{
			free(StackBase);
			StackBase = nullptr;
		}
	}

	// 禁用拷贝
	SCoroutineContext(const SCoroutineContext&) = delete;
	SCoroutineContext& operator=(const SCoroutineContext&) = delete;

	// 允许移动
	SCoroutineContext(SCoroutineContext&& Other) noexcept
	    : StackPointer(Other.StackPointer),
	      StackSize(Other.StackSize),
	      StackBase(Other.StackBase),
	      bIsInitialized(Other.bIsInitialized)
	{
		memcpy(JumpBuffer, Other.JumpBuffer, sizeof(jmp_buf));
		Other.StackPointer = nullptr;
		Other.StackSize = 0;
		Other.StackBase = nullptr;
		Other.bIsInitialized = false;
	}

	SCoroutineContext& operator=(SCoroutineContext&& Other) noexcept
	{
		if (this != &Other)
		{
			if (StackBase)
			{
				free(StackBase);
			}

			memcpy(JumpBuffer, Other.JumpBuffer, sizeof(jmp_buf));
			StackPointer = Other.StackPointer;
			StackSize = Other.StackSize;
			StackBase = Other.StackBase;
			bIsInitialized = Other.bIsInitialized;

			Other.StackPointer = nullptr;
			Other.StackSize = 0;
			Other.StackBase = nullptr;
			Other.bIsInitialized = false;
		}
		return *this;
	}
};

/**
 * @brief 协程统计信息
 */
struct SCoroutineStats
{
	uint32_t TotalYields = 0;  // 总让出次数
	uint32_t TotalResumes = 0; // 总恢复次数
	CTimespan TotalRunTime;    // 总运行时间
	CTimespan AverageRunTime;  // 平均运行时间
	CDateTime CreationTime;    // 创建时间
	CDateTime LastRunTime;     // 最后运行时间

	void Reset()
	{
		TotalYields = 0;
		TotalResumes = 0;
		TotalRunTime = CTimespan::Zero();
		AverageRunTime = CTimespan::Zero();
	}

	void UpdateRunTime(const CTimespan& RunTime)
	{
		TotalRunTime += RunTime;
		if (TotalResumes > 0)
		{
			AverageRunTime = CTimespan::FromSeconds(TotalRunTime.GetTotalSeconds() / TotalResumes);
		}
	}
};

/**
 * @brief 协程等待条件基类
 */
class ICoroutineWaitCondition
{
public:
	virtual ~ICoroutineWaitCondition() = default;

	/**
	 * @brief 检查等待条件是否满足
	 */
	virtual bool IsReady() const = 0;

	/**
	 * @brief 获取等待条件描述
	 */
	virtual TString GetDescription() const = 0;

	/**
	 * @brief 在等待期间执行的回调
	 */
	virtual void OnWait()
	{}

	/**
	 * @brief 等待超时处理
	 */
	virtual void OnTimeout()
	{}
};

/**
 * @brief 时间等待条件
 */
class CTimeWaitCondition : public ICoroutineWaitCondition
{
public:
	explicit CTimeWaitCondition(const CTimespan& Duration)
	    : StartTime(CDateTime::Now()),
	      WaitDuration(Duration)
	{}

	bool IsReady() const override
	{
		return (CDateTime::Now() - StartTime) >= WaitDuration;
	}

	TString GetDescription() const override
	{
		return TString("TimeWait");
	}

private:
	CDateTime StartTime;
	CTimespan WaitDuration;
};

/**
 * @brief 条件变量等待条件
 */
class CConditionWaitCondition : public ICoroutineWaitCondition
{
public:
	using ConditionFunc = std::function<bool()>;

	explicit CConditionWaitCondition(ConditionFunc InCondition, const TString& InDescription = TString("Condition"))
	    : Condition(std::move(InCondition)),
	      Description(InDescription)
	{}

	bool IsReady() const override
	{
		return Condition ? Condition() : true;
	}

	TString GetDescription() const override
	{
		return Description;
	}

private:
	ConditionFunc Condition;
	TString Description;
};

/**
 * @brief 协程类
 *
 * 提供轻量级的协作式多任务处理：
 * - 自定义栈管理
 * - 协程状态跟踪
 * - 等待条件支持
 * - 统计信息收集
 */
NCLASS()
class NCoroutine : public NObject
{
	GENERATED_BODY()

	friend class NCoroutineScheduler;

public:
	// === 委托定义 ===
	DECLARE_DELEGATE(FOnCoroutineStarted, FCoroutineId);
	DECLARE_DELEGATE(FOnCoroutineCompleted, FCoroutineId);
	DECLARE_DELEGATE(FOnCoroutineFailed, FCoroutineId, const TString&);
	DECLARE_DELEGATE(FOnCoroutineYielded, FCoroutineId);
	DECLARE_DELEGATE(FOnCoroutineResumed, FCoroutineId);

	using CoroutineFunction = std::function<void()>;

public:
	// === 构造函数 ===

	/**
	 * @brief 构造函数
	 */
	NCoroutine(CoroutineFunction InFunction,
	           const TString& InName = TString("Coroutine"),
	           size_t InStackSize = DEFAULT_COROUTINE_STACK_SIZE)
	    : Function(std::move(InFunction)),
	      Name(InName),
	      StackSize(InStackSize),
	      State(ECoroutineState::Created),
	      CoroutineId(GenerateCoroutineId()),
	      bIsMainCoroutine(false),
	      CurrentWaitCondition(nullptr)
	{
		// 验证栈大小
		if (StackSize < MIN_COROUTINE_STACK_SIZE)
		{
			StackSize = MIN_COROUTINE_STACK_SIZE;
		}
		else if (StackSize > MAX_COROUTINE_STACK_SIZE)
		{
			StackSize = MAX_COROUTINE_STACK_SIZE;
		}

		Stats.CreationTime = CDateTime::Now();

		NLOG_THREADING(
		    Debug, "Coroutine '{}' created with ID: {}, stack size: {} bytes", Name.GetData(), CoroutineId, StackSize);
	}

	/**
	 * @brief 主协程构造函数
	 */
	explicit NCoroutine(const TString& InName = TString("MainCoroutine"))
	    : Function(nullptr),
	      Name(InName),
	      StackSize(0),
	      State(ECoroutineState::Running),
	      CoroutineId(GenerateCoroutineId()),
	      bIsMainCoroutine(true),
	      CurrentWaitCondition(nullptr)
	{
		Stats.CreationTime = CDateTime::Now();

		NLOG_THREADING(Debug, "Main coroutine '{}' created with ID: {}", Name.GetData(), CoroutineId);
	}

	/**
	 * @brief 析构函数
	 */
	virtual ~NCoroutine()
	{
		if (State == ECoroutineState::Running)
		{
			Cancel();
		}

		NLOG_THREADING(Debug, "Coroutine '{}' (ID: {}) destroyed", Name.GetData(), CoroutineId);
	}

public:
	// === 协程控制 ===

	/**
	 * @brief 初始化协程
	 */
	bool Initialize()
	{
		if (bIsMainCoroutine)
		{
			return true; // 主协程不需要初始化栈
		}

		if (State != ECoroutineState::Created)
		{
			NLOG_THREADING(
			    Error, "Cannot initialize coroutine '{}' in state: {}", Name.GetData(), GetStateString().GetData());
			return false;
		}

		// 分配栈空间
		Context.StackBase = malloc(StackSize);
		if (!Context.StackBase)
		{
			NLOG_THREADING(Error, "Failed to allocate stack for coroutine '{}'", Name.GetData());
			return false;
		}

		Context.StackSize = StackSize;
		Context.StackPointer = static_cast<char*>(Context.StackBase) + StackSize;
		Context.bIsInitialized = true;

		State = ECoroutineState::Ready;

		NLOG_THREADING(Debug, "Coroutine '{}' initialized successfully", Name.GetData());
		return true;
	}

	/**
	 * @brief 启动协程
	 */
	bool Start()
	{
		if (!bIsMainCoroutine && !Context.bIsInitialized)
		{
			if (!Initialize())
			{
				return false;
			}
		}

		if (State != ECoroutineState::Ready && State != ECoroutineState::Created)
		{
			NLOG_THREADING(
			    Error, "Cannot start coroutine '{}' in state: {}", Name.GetData(), GetStateString().GetData());
			return false;
		}

		State = ECoroutineState::Running;
		Stats.LastRunTime = CDateTime::Now();

		OnCoroutineStarted.ExecuteIfBound(CoroutineId);

		NLOG_THREADING(Debug, "Coroutine '{}' started", Name.GetData());
		return true;
	}

	/**
	 * @brief 让出执行权
	 */
	void Yield()
	{
		if (State != ECoroutineState::Running)
		{
			return;
		}

		State = ECoroutineState::Suspended;
		Stats.TotalYields++;

		// 更新运行时间统计
		CDateTime Now = CDateTime::Now();
		CTimespan RunTime = Now - Stats.LastRunTime;
		Stats.UpdateRunTime(RunTime);

		OnCoroutineYielded.ExecuteIfBound(CoroutineId);

		NLOG_THREADING(Trace, "Coroutine '{}' yielded", Name.GetData());

		// 保存当前上下文并跳转到调度器
		if (setjmp(Context.JumpBuffer) == 0)
		{
			// 这里会跳转到调度器
			longjmp(GetSchedulerContext(), 1);
		}

		// 协程恢复执行时会到达这里
		State = ECoroutineState::Running;
		Stats.TotalResumes++;
		Stats.LastRunTime = CDateTime::Now();

		OnCoroutineResumed.ExecuteIfBound(CoroutineId);

		NLOG_THREADING(Trace, "Coroutine '{}' resumed", Name.GetData());
	}

	/**
	 * @brief 等待指定时间
	 */
	void WaitFor(const CTimespan& Duration)
	{
		auto WaitCondition = MakeShared<CTimeWaitCondition>(Duration);
		WaitForCondition(WaitCondition);
	}

	/**
	 * @brief 等待条件满足
	 */
	void WaitForCondition(TSharedPtr<ICoroutineWaitCondition> Condition)
	{
		if (!Condition.IsValid())
		{
			return;
		}

		CurrentWaitCondition = Condition;

		NLOG_THREADING(
		    Trace, "Coroutine '{}' waiting for condition: {}", Name.GetData(), Condition->GetDescription().GetData());

		// 让出执行权直到条件满足
		while (!Condition->IsReady() && State == ECoroutineState::Running)
		{
			Condition->OnWait();
			Yield();
		}

		CurrentWaitCondition = nullptr;
	}

	/**
	 * @brief 等待条件满足（带超时）
	 */
	bool WaitForCondition(TSharedPtr<ICoroutineWaitCondition> Condition, const CTimespan& Timeout)
	{
		if (!Condition.IsValid())
		{
			return true;
		}

		CDateTime StartTime = CDateTime::Now();
		CurrentWaitCondition = Condition;

		while (!Condition->IsReady() && State == ECoroutineState::Running)
		{
			CTimespan Elapsed = CDateTime::Now() - StartTime;
			if (Elapsed >= Timeout)
			{
				Condition->OnTimeout();
				CurrentWaitCondition = nullptr;
				return false; // 超时
			}

			Condition->OnWait();
			Yield();
		}

		CurrentWaitCondition = nullptr;
		return true; // 条件满足
	}

	/**
	 * @brief 取消协程
	 */
	void Cancel()
	{
		if (State == ECoroutineState::Completed || State == ECoroutineState::Cancelled)
		{
			return;
		}

		State = ECoroutineState::Cancelled;
		CurrentWaitCondition = nullptr;

		NLOG_THREADING(Debug, "Coroutine '{}' cancelled", Name.GetData());
	}

	/**
	 * @brief 恢复协程执行
	 */
	void Resume()
	{
		if (State != ECoroutineState::Suspended)
		{
			return;
		}

		// 检查等待条件
		if (CurrentWaitCondition.IsValid() && !CurrentWaitCondition->IsReady())
		{
			return; // 等待条件未满足
		}

		NLOG_THREADING(Trace, "Resuming coroutine '{}'", Name.GetData());

		// 恢复协程上下文
		longjmp(Context.JumpBuffer, 1);
	}

public:
	// === 状态查询 ===

	/**
	 * @brief 获取协程ID
	 */
	FCoroutineId GetCoroutineId() const
	{
		return CoroutineId;
	}

	/**
	 * @brief 获取协程名称
	 */
	const TString& GetName() const
	{
		return Name;
	}

	/**
	 * @brief 获取协程状态
	 */
	ECoroutineState GetState() const
	{
		return State.load();
	}

	/**
	 * @brief 检查是否为主协程
	 */
	bool IsMainCoroutine() const
	{
		return bIsMainCoroutine;
	}

	/**
	 * @brief 检查是否正在运行
	 */
	bool IsRunning() const
	{
		return State.load() == ECoroutineState::Running;
	}

	/**
	 * @brief 检查是否已完成
	 */
	bool IsCompleted() const
	{
		ECoroutineState CurrentState = State.load();
		return CurrentState == ECoroutineState::Completed || CurrentState == ECoroutineState::Failed ||
		       CurrentState == ECoroutineState::Cancelled;
	}

	/**
	 * @brief 检查是否可以恢复
	 */
	bool CanResume() const
	{
		if (State.load() != ECoroutineState::Suspended)
		{
			return false;
		}

		return !CurrentWaitCondition.IsValid() || CurrentWaitCondition->IsReady();
	}

	/**
	 * @brief 获取栈大小
	 */
	size_t GetStackSize() const
	{
		return StackSize;
	}

	/**
	 * @brief 获取统计信息
	 */
	const SCoroutineStats& GetStats() const
	{
		return Stats;
	}

public:
	// === 委托事件 ===

	FOnCoroutineStarted OnCoroutineStarted;
	FOnCoroutineCompleted OnCoroutineCompleted;
	FOnCoroutineFailed OnCoroutineFailed;
	FOnCoroutineYielded OnCoroutineYielded;
	FOnCoroutineResumed OnCoroutineResumed;

protected:
	// === 内部实现 ===

	/**
	 * @brief 执行协程函数
	 */
	void Execute()
	{
		if (bIsMainCoroutine || !Function)
		{
			return;
		}

		try
		{
			Function();
			State = ECoroutineState::Completed;
			OnCoroutineCompleted.ExecuteIfBound(CoroutineId);

			NLOG_THREADING(Debug, "Coroutine '{}' completed successfully", Name.GetData());
		}
		catch (const std::exception& e)
		{
			State = ECoroutineState::Failed;
			TString ErrorMsg(e.what());
			OnCoroutineFailed.ExecuteIfBound(CoroutineId, ErrorMsg);

			NLOG_THREADING(Error, "Coroutine '{}' failed with exception: {}", Name.GetData(), e.what());
		}
		catch (...)
		{
			State = ECoroutineState::Failed;
			TString ErrorMsg("Unknown exception");
			OnCoroutineFailed.ExecuteIfBound(CoroutineId, ErrorMsg);

			NLOG_THREADING(Error, "Coroutine '{}' failed with unknown exception", Name.GetData());
		}
	}

	/**
	 * @brief 获取状态字符串
	 */
	TString GetStateString() const
	{
		switch (State.load())
		{
		case ECoroutineState::Created:
			return TString("Created");
		case ECoroutineState::Ready:
			return TString("Ready");
		case ECoroutineState::Running:
			return TString("Running");
		case ECoroutineState::Suspended:
			return TString("Suspended");
		case ECoroutineState::Completed:
			return TString("Completed");
		case ECoroutineState::Failed:
			return TString("Failed");
		case ECoroutineState::Cancelled:
			return TString("Cancelled");
		default:
			return TString("Unknown");
		}
	}

	/**
	 * @brief 生成协程ID
	 */
	static FCoroutineId GenerateCoroutineId()
	{
		static std::atomic<FCoroutineId> NextId{1};
		return NextId.fetch_add(1);
	}

	/**
	 * @brief 获取调度器上下文
	 */
	jmp_buf& GetSchedulerContext();

private:
	// === 成员变量 ===

	CoroutineFunction Function;         // 协程函数
	TString Name;                       // 协程名称
	size_t StackSize;                   // 栈大小
	std::atomic<ECoroutineState> State; // 协程状态
	FCoroutineId CoroutineId;           // 协程ID
	bool bIsMainCoroutine;              // 是否为主协程

	SCoroutineContext Context;                                // 协程上下文
	SCoroutineStats Stats;                                    // 统计信息
	TSharedPtr<ICoroutineWaitCondition> CurrentWaitCondition; // 当前等待条件
};

// === 类型别名 ===
using FCoroutine = NCoroutine;

// === 便捷等待条件创建函数 ===

/**
 * @brief 创建时间等待条件
 */
inline TSharedPtr<CTimeWaitCondition> CreateTimeWait(const CTimespan& Duration)
{
	return MakeShared<CTimeWaitCondition>(Duration);
}

/**
 * @brief 创建条件等待条件
 */
template <typename TFunc>
TSharedPtr<CConditionWaitCondition> CreateConditionWait(TFunc&& Condition,
                                                        const TString& Description = TString("Condition"))
{
	return MakeShared<CConditionWaitCondition>(std::forward<TFunc>(Condition), Description);
}

} // namespace NLib