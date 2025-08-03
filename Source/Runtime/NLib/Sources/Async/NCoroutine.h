#pragma once

#include "Core/CObject.h"
#include "Containers/CString.h"
#include "Memory/CMemoryManager.h"
#include "Delegates/CDelegate.h"

#include <cstdint>
#include <csetjmp>

namespace NLib
{

/**
 * @brief 协程状态枚举
 */
enum class ECoroutineState : uint32_t
{
    Created,    // 已创建
    Running,    // 运行中
    Suspended,  // 已挂起
    Completed,  // 已完成
    Aborted     // 已中止
};

/**
 * @brief 协程优先级枚举
 */
enum class ECoroutinePriority : uint32_t
{
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

/**
 * @brief 协程栈信息
 */
struct NCoroutineStack
{
    void* StackMemory;
    size_t StackSize;
    void* StackTop;
    void* StackBottom;
    
    NCoroutineStack()
        : StackMemory(nullptr), StackSize(0), StackTop(nullptr), StackBottom(nullptr)
    {}
    
    ~NCoroutineStack()
    {
        if (StackMemory)
        {
            CMemoryManager::GetInstance().Deallocate(StackMemory);
            StackMemory = nullptr;
        }
    }
};

/**
 * @brief 协程上下文 - 用于保存和恢复执行状态
 */
struct NCoroutineContext
{
    jmp_buf JumpBuffer;
    NCoroutineStack Stack;
    ECoroutineState State;
    void* UserData;
    
    NCoroutineContext()
        : State(ECoroutineState::Created), UserData(nullptr)
    {
        // 初始化jump buffer
        memset(&JumpBuffer, 0, sizeof(jmp_buf));
    }
};

/**
 * @brief 协程句柄 - 协程的轻量级引用
 */
class LIBNLIB_API NCoroutineHandle
{
public:
    NCoroutineHandle();
    explicit NCoroutineHandle(uint64_t InCoroutineId);
    
    // 基本操作
    bool IsValid() const;
    bool IsCompleted() const;
    bool IsSuspended() const;
    void Resume();
    void Abort();
    
    // 状态查询
    ECoroutineState GetState() const;
    uint64_t GetCoroutineId() const { return CoroutineId; }
    
    // 比较操作
    bool operator==(const NCoroutineHandle& Other) const;
    bool operator!=(const NCoroutineHandle& Other) const;
    
    // 无效句柄
    static NCoroutineHandle Invalid();

private:
    uint64_t CoroutineId;
    
    friend class CCoroutineScheduler;
};

/**
 * @brief 协程调度器 - 管理所有协程的执行
 */
class LIBNLIB_API CCoroutineScheduler : public CObject
{
    NCLASS()
class CCoroutineScheduler : public CObject
{
    GENERATED_BODY()

public:
    using CoroutineFunction = void(*)(void* UserData);
    
    CCoroutineScheduler();
    virtual ~CCoroutineScheduler();
    
    // 协程创建
    NCoroutineHandle CreateCoroutine(CoroutineFunction Function, void* UserData = nullptr);
    NCoroutineHandle CreateCoroutine(CoroutineFunction Function, void* UserData, size_t StackSize);
    NCoroutineHandle CreateCoroutine(CoroutineFunction Function, void* UserData, size_t StackSize, ECoroutinePriority Priority);
    
    // 协程控制
    void ResumeCoroutine(uint64_t CoroutineId);
    void SuspendCoroutine(uint64_t CoroutineId);
    void AbortCoroutine(uint64_t CoroutineId);
    void YieldCoroutine();
    
    // 调度控制
    void Update();
    void UpdateFor(int32_t MaxExecutionTimeMs);
    bool HasPendingCoroutines() const;
    
    // 状态查询
    size_t GetCoroutineCount() const;
    size_t GetActiveCoroutineCount() const;
    size_t GetSuspendedCoroutineCount() const;
    
    // 当前协程信息
    uint64_t GetCurrentCoroutineId() const;
    bool IsInCoroutine() const;
    
    // 静态实例
    static CCoroutineScheduler& GetGlobalScheduler();
    
    // 协程内部可用函数
    static void Yield();
    static void YieldFor(int32_t Milliseconds);
    static void Sleep(int32_t Milliseconds);
    
private:
    struct CoroutineInfo
    {
        uint64_t Id;
        CoroutineFunction Function;
        void* UserData;
        NCoroutineContext Context;
        ECoroutinePriority Priority;
        int64_t SleepUntil;
        CString Name;
        
        CoroutineInfo()
            : Id(0), Function(nullptr), UserData(nullptr), Priority(ECoroutinePriority::Normal), SleepUntil(0)
        {}
    };
    
    CArray<CoroutineInfo*> Coroutines;
    CArray<CoroutineInfo*> ReadyQueue;
    CArray<CoroutineInfo*> SuspendedQueue;
    
    uint64_t NextCoroutineId;
    uint64_t CurrentCoroutineId;
    CoroutineInfo* CurrentCoroutine;
    jmp_buf MainContext;
    
    static constexpr size_t DefaultStackSize = 65536; // 64KB
    
    // 内部方法
    CoroutineInfo* FindCoroutine(uint64_t CoroutineId);
    void RemoveCoroutine(uint64_t CoroutineId);
    void ScheduleNextCoroutine();
    bool SetupCoroutineStack(CoroutineInfo* Coroutine, size_t StackSize);
    void SwitchToCoroutine(CoroutineInfo* Coroutine);
    void SwitchToMain();
    int64_t GetCurrentTimeMs() const;
    
    // 协程入口点
    static void CoroutineEntry(CoroutineInfo* Coroutine);
    
    // 禁止复制
    CCoroutineScheduler(const CCoroutineScheduler&) = delete;
    CCoroutineScheduler& operator=(const CCoroutineScheduler&) = delete;
};

/**
 * @brief 协程等待器基类
 */
class LIBNLIB_API NCoroutineAwaiter : public CObject
{
    NCLASS()
class NCoroutineAwaiter : public NObject
{
    GENERATED_BODY()

public:
    NCoroutineAwaiter() = default;
    virtual ~NCoroutineAwaiter() = default;
    
    // 检查是否准备好继续执行
    virtual bool IsReady() = 0;
    
    // 等待完成
    virtual void Await() = 0;
    
    // 取消等待
    virtual void Cancel() {}
};

/**
 * @brief 时间等待器
 */
class LIBNLIB_API NTimeAwaiter : public NCoroutineAwaiter
{
    NCLASS()
class NTimeAwaiter : public NCoroutineAwaiter
{
    GENERATED_BODY()

public:
    explicit NTimeAwaiter(int32_t DelayMs);
    
    virtual bool IsReady() override;
    virtual void Await() override;

private:
    int64_t EndTime;
    int64_t GetCurrentTimeMs() const;
};

/**
 * @brief 协程等待器 - 等待另一个协程完成
 */
NCLASS()
class LIBNLIB_API NCoroutineWaitAwaiter : public NCoroutineAwaiter
{
    GENERATED_BODY()

public:
    explicit NCoroutineWaitAwaiter(const NCoroutineHandle& Handle);
    
    virtual bool IsReady() override;
    virtual void Await() override;

private:
    NCoroutineHandle TargetHandle;
};

/**
 * @brief 协程同步原语 - 信号量
 */
NCLASS()
class LIBNLIB_API NCoroutineSemaphore : public CObject
{
    GENERATED_BODY()

public:
    explicit NCoroutineSemaphore(int32_t InitialCount);
    virtual ~NCoroutineSemaphore();
    
    void Wait();
    bool TryWait();
    void Post();
    void Post(int32_t Count);
    
    int32_t GetCount() const { return Count; }

private:
    int32_t Count;
    CArray<uint64_t> WaitingCoroutines;
    
    void WakeupWaitingCoroutines();
};

/**
 * @brief 协程互斥锁
 */
NCLASS()
class LIBNLIB_API NCoroutineMutex : public CObject
{
    GENERATED_BODY()

public:
    NCoroutineMutex();
    virtual ~NCoroutineMutex();
    
    void Lock();
    bool TryLock();
    void Unlock();
    
    bool IsLocked() const { return bIsLocked; }
    uint64_t GetOwnerCoroutine() const { return OwnerCoroutine; }

private:
    bool bIsLocked;
    uint64_t OwnerCoroutine;
    CArray<uint64_t> WaitingCoroutines;
    
    void WakeupNextWaiter();
};

/**
 * @brief 协程通道 - 用于协程间通信
 */
template<typename TType>
class NCoroutineChannel : public NObject
{

public:
    explicit NCoroutineChannel(size_t Capacity = 0);
    virtual ~NCoroutineChannel();
    
    // 发送数据
    void Send(const TType& Value);
    void Send(TType&& Value);
    bool TrySend(const TType& Value);
    bool TrySend(TType&& Value);
    
    // 接收数据
    TType Receive();
    bool TryReceive(TType& OutValue);
    
    // 状态查询
    bool IsEmpty() const;
    bool IsFull() const;
    size_t GetSize() const;
    size_t GetCapacity() const;
    
    // 关闭通道
    void Close();
    bool IsClosed() const;

private:
    struct ChannelItem
    {
        TType Value;
        bool IsValid;
        
        ChannelItem() : IsValid(false) {}
        ChannelItem(const TType& InValue) : Value(InValue), IsValid(true) {}
        ChannelItem(TType&& InValue) : Value(NLib::Move(InValue)), IsValid(true) {}
    };
    
    CArray<ChannelItem> Buffer;
    size_t Capacity;
    size_t Head;
    size_t Tail;
    size_t Count;
    bool bIsClosed;
    
    CArray<uint64_t> SendWaiters;
    CArray<uint64_t> ReceiveWaiters;
    
    void WakeupSenders();
    void WakeupReceivers();
};

/**
 * @brief 协程生成器 - 用于创建可迭代的协程
 */
template<typename TType>
class NCoroutineGenerator : public NObject
{

public:
    using GeneratorFunction = void(*)(NCoroutineGenerator<TType>* Generator, void* UserData);
    
    NCoroutineGenerator(GeneratorFunction Function, void* UserData = nullptr);
    virtual ~NCoroutineGenerator();
    
    // 迭代器接口
    bool HasNext();
    TType Next();
    void Reset();
    
    // 生成器内部使用
    void Yield(const TType& Value);
    void Yield(TType&& Value);
    void Return();

private:
    GeneratorFunction Function;
    void* UserData;
    NCoroutineHandle CoroutineHandle;
    TType CurrentValue;
    bool bHasValue;
    bool bIsCompleted;
    
    static void GeneratorEntry(void* GeneratorPtr);
};

// === 协程工具函数 ===

/**
 * @brief 创建协程的便利函数
 */
NCoroutineHandle CreateCoroutine(CCoroutineScheduler::CoroutineFunction Function, void* UserData = nullptr);

/**
 * @brief 等待协程完成
 */
void AwaitCoroutine(const NCoroutineHandle& Handle);

/**
 * @brief 等待指定时间
 */
void AwaitTime(int32_t Milliseconds);

/**
 * @brief 等待所有协程完成
 */
void AwaitAll(const CArray<NCoroutineHandle>& Handles);

/**
 * @brief 等待任意一个协程完成
 */
NCoroutineHandle AwaitAny(const CArray<NCoroutineHandle>& Handles);

// === 协程宏定义 ===

/**
 * @brief 协程函数声明宏
 */
#define DECLARE_COROUTINE(FunctionName) \
    void FunctionName(void* UserData)

/**
 * @brief 协程让步宏
 */
#define COROUTINE_YIELD() \
    NLib::CCoroutineScheduler::Yield()

/**
 * @brief 协程睡眠宏
 */
#define COROUTINE_SLEEP(Milliseconds) \
    NLib::CCoroutineScheduler::Sleep(Milliseconds)

/**
 * @brief 协程等待宏
 */
#define COROUTINE_AWAIT(Awaiter) \
    do { (Awaiter).Await(); } while (false)

/**
 * @brief 协程返回宏
 */
#define COROUTINE_RETURN() \
    return

// === 模板实现 ===

template<typename TType>
NCoroutineChannel<TType>::NCoroutineChannel(size_t InCapacity)
    : Capacity(InCapacity), Head(0), Tail(0), Count(0), bIsClosed(false)
{
    if (Capacity > 0)
    {
        Buffer.Resize(Capacity);
    }
}

template<typename TType>
NCoroutineChannel<TType>::~NCoroutineChannel()
{
    Close();
}

template<typename TType>
void NCoroutineChannel<TType>::Send(const TType& Value)
{
    while (IsFull() && !IsClosed())
    {
        SendWaiters.PushBack(CCoroutineScheduler::GetGlobalScheduler().GetCurrentCoroutineId());
        CCoroutineScheduler::Yield();
    }
    
    if (IsClosed())
    {
        return;
    }
    
    if (Capacity == 0)
    {
        // 无缓冲通道，直接传递给等待的接收者
        if (!ReceiveWaiters.IsEmpty())
        {
            // 这里简化实现，实际需要更复杂的同步机制
            WakeupReceivers();
        }
    }
    else
    {
        // 有缓冲通道
        Buffer[Tail] = ChannelItem(Value);
        Tail = (Tail + 1) % Capacity;
        ++Count;
        
        WakeupReceivers();
    }
}

template<typename TType>
void NCoroutineChannel<TType>::Send(TType&& Value)
{
    while (IsFull() && !IsClosed())
    {
        SendWaiters.PushBack(CCoroutineScheduler::GetGlobalScheduler().GetCurrentCoroutineId());
        CCoroutineScheduler::Yield();
    }
    
    if (IsClosed())
    {
        return;
    }
    
    if (Capacity == 0)
    {
        if (!ReceiveWaiters.IsEmpty())
        {
            WakeupReceivers();
        }
    }
    else
    {
        Buffer[Tail] = ChannelItem(NLib::Move(Value));
        Tail = (Tail + 1) % Capacity;
        ++Count;
        
        WakeupReceivers();
    }
}

template<typename TType>
bool NCoroutineChannel<TType>::TrySend(const TType& Value)
{
    if (IsFull() || IsClosed())
    {
        return false;
    }
    
    Send(Value);
    return true;
}

template<typename TType>
bool NCoroutineChannel<TType>::TrySend(TType&& Value)
{
    if (IsFull() || IsClosed())
    {
        return false;
    }
    
    Send(NLib::Move(Value));
    return true;
}

template<typename TType>
TType NCoroutineChannel<TType>::Receive()
{
    while (IsEmpty() && !IsClosed())
    {
        ReceiveWaiters.PushBack(CCoroutineScheduler::GetGlobalScheduler().GetCurrentCoroutineId());
        CCoroutineScheduler::Yield();
    }
    
    if (IsEmpty() && IsClosed())
    {
        return TType{}; // 返回默认值
    }
    
    TType Result = NLib::Move(Buffer[Head].Value);
    Buffer[Head].IsValid = false;
    Head = (Head + 1) % Capacity;
    --Count;
    
    WakeupSenders();
    return Result;
}

template<typename TType>
bool NCoroutineChannel<TType>::TryReceive(TType& OutValue)
{
    if (IsEmpty())
    {
        return false;
    }
    
    OutValue = NLib::Move(Buffer[Head].Value);
    Buffer[Head].IsValid = false;
    Head = (Head + 1) % Capacity;
    --Count;
    
    WakeupSenders();
    return true;
}

template<typename TType>
bool NCoroutineChannel<TType>::IsEmpty() const
{
    return Count == 0;
}

template<typename TType>
bool NCoroutineChannel<TType>::IsFull() const
{
    return Capacity > 0 && Count >= Capacity;
}

template<typename TType>
size_t NCoroutineChannel<TType>::GetSize() const
{
    return Count;
}

template<typename TType>
size_t NCoroutineChannel<TType>::GetCapacity() const
{
    return Capacity;
}

template<typename TType>
void NCoroutineChannel<TType>::Close()
{
    bIsClosed = true;
    WakeupSenders();
    WakeupReceivers();
}

template<typename TType>
bool NCoroutineChannel<TType>::IsClosed() const
{
    return bIsClosed;
}

template<typename TType>
void NCoroutineChannel<TType>::WakeupSenders()
{
    for (uint64_t CoroutineId : SendWaiters)
    {
        CCoroutineScheduler::GetGlobalScheduler().ResumeCoroutine(CoroutineId);
    }
    SendWaiters.Clear();
}

template<typename TType>
void NCoroutineChannel<TType>::WakeupReceivers()
{
    for (uint64_t CoroutineId : ReceiveWaiters)
    {
        CCoroutineScheduler::GetGlobalScheduler().ResumeCoroutine(CoroutineId);
    }
    ReceiveWaiters.Clear();
}

template<typename TType>
NCoroutineGenerator<TType>::NCoroutineGenerator(GeneratorFunction Function, void* UserData)
    : Function(Function), UserData(UserData), bHasValue(false), bIsCompleted(false)
{
    CoroutineHandle = CCoroutineScheduler::GetGlobalScheduler().CreateCoroutine(GeneratorEntry, this);
}

template<typename TType>
NCoroutineGenerator<TType>::~NCoroutineGenerator()
{
    if (CoroutineHandle.IsValid() && !CoroutineHandle.IsCompleted())
    {
        CoroutineHandle.Abort();
    }
}

template<typename TType>
bool NCoroutineGenerator<TType>::HasNext()
{
    if (bIsCompleted)
    {
        return false;
    }
    
    if (!bHasValue)
    {
        CoroutineHandle.Resume();
    }
    
    return bHasValue && !bIsCompleted;
}

template<typename TType>
TType NCoroutineGenerator<TType>::Next()
{
    if (!HasNext())
    {
        return TType{}; // 返回默认值
    }
    
    TType Result = NLib::Move(CurrentValue);
    bHasValue = false;
    return Result;
}

template<typename TType>
void NCoroutineGenerator<TType>::Reset()
{
    if (CoroutineHandle.IsValid())
    {
        CoroutineHandle.Abort();
    }
    
    bHasValue = false;
    bIsCompleted = false;
    CoroutineHandle = CCoroutineScheduler::GetGlobalScheduler().CreateCoroutine(GeneratorEntry, this);
}

template<typename TType>
void NCoroutineGenerator<TType>::Yield(const TType& Value)
{
    CurrentValue = Value;
    bHasValue = true;
    CCoroutineScheduler::Yield();
}

template<typename TType>
void NCoroutineGenerator<TType>::Yield(TType&& Value)
{
    CurrentValue = NLib::Move(Value);
    bHasValue = true;
    CCoroutineScheduler::Yield();
}

template<typename TType>
void NCoroutineGenerator<TType>::Return()
{
    bIsCompleted = true;
    bHasValue = false;
}

template<typename TType>
void NCoroutineGenerator<TType>::GeneratorEntry(void* GeneratorPtr)
{
    NCoroutineGenerator<TType>* Generator = static_cast<NCoroutineGenerator<TType>*>(GeneratorPtr);
    Generator->Function(Generator, Generator->UserData);
    Generator->Return();
}

} // namespace NLib
