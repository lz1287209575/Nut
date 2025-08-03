#include "NCoroutine.h"
#include "NCoroutine.generate.h"

#include "Logging/CLogger.h"

#include <cstring>
#include <chrono>

// 平台特定头文件
#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
    #include <signal.h>
#endif

namespace NLib
{

// === 协程句柄实现 ===

NCoroutineHandle::NCoroutineHandle()
    : CoroutineId(0)
{}

NCoroutineHandle::NCoroutineHandle(uint64_t InCoroutineId)
    : CoroutineId(InCoroutineId)
{}

bool NCoroutineHandle::IsValid() const
{
    return CoroutineId != 0 && CCoroutineScheduler::GetGlobalScheduler().FindCoroutine(CoroutineId) != nullptr;
}

bool NCoroutineHandle::IsCompleted() const
{
    if (!IsValid())
    {
        return true;
    }
    
    ECoroutineState State = GetState();
    return State == ECoroutineState::Completed || State == ECoroutineState::Aborted;
}

bool NCoroutineHandle::IsSuspended() const
{
    return IsValid() && GetState() == ECoroutineState::Suspended;
}

void NCoroutineHandle::Resume()
{
    if (IsValid() && IsSuspended())
    {
        CCoroutineScheduler::GetGlobalScheduler().ResumeCoroutine(CoroutineId);
    }
}

void NCoroutineHandle::Abort()
{
    if (IsValid() && !IsCompleted())
    {
        CCoroutineScheduler::GetGlobalScheduler().AbortCoroutine(CoroutineId);
    }
}

ECoroutineState NCoroutineHandle::GetState() const
{
    auto* Scheduler = &CCoroutineScheduler::GetGlobalScheduler();
    auto* Coroutine = Scheduler->FindCoroutine(CoroutineId);
    return Coroutine ? Coroutine->Context.State : ECoroutineState::Aborted;
}

bool NCoroutineHandle::operator==(const NCoroutineHandle& Other) const
{
    return CoroutineId == Other.CoroutineId;
}

bool NCoroutineHandle::operator!=(const NCoroutineHandle& Other) const
{
    return CoroutineId != Other.CoroutineId;
}

NCoroutineHandle NCoroutineHandle::Invalid()
{
    return NCoroutineHandle(0);
}

// === 协程调度器实现 ===

// 全局调度器实例
static CCoroutineScheduler* GlobalScheduler = nullptr;

CCoroutineScheduler::CCoroutineScheduler()
    : NextCoroutineId(1), CurrentCoroutineId(0), CurrentCoroutine(nullptr)
{
    // 初始化主上下文
    memset(&MainContext, 0, sizeof(jmp_buf));
}

CCoroutineScheduler::~CCoroutineScheduler()
{
    // 清理所有协程
    for (auto* Coroutine : Coroutines)
    {
        if (Coroutine)
        {
            delete Coroutine;
        }
    }
    Coroutines.Clear();
    ReadyQueue.Clear();
    SuspendedQueue.Clear();
}

NCoroutineHandle CCoroutineScheduler::CreateCoroutine(CoroutineFunction Function, void* UserData)
{
    return CreateCoroutine(Function, UserData, DefaultStackSize, ECoroutinePriority::Normal);
}

NCoroutineHandle CCoroutineScheduler::CreateCoroutine(CoroutineFunction Function, void* UserData, size_t StackSize)
{
    return CreateCoroutine(Function, UserData, StackSize, ECoroutinePriority::Normal);
}

NCoroutineHandle CCoroutineScheduler::CreateCoroutine(CoroutineFunction Function, void* UserData, size_t StackSize, ECoroutinePriority Priority)
{
    if (!Function)
    {
        CLogger::Error("CCoroutineScheduler::CreateCoroutine: Function cannot be null");
        return NCoroutineHandle::Invalid();
    }
    
    auto* NewCoroutine = new CoroutineInfo();
    NewCoroutine->Id = NextCoroutineId++;
    NewCoroutine->Function = Function;
    NewCoroutine->UserData = UserData;
    NewCoroutine->Priority = Priority;
    NewCoroutine->SleepUntil = 0;
    NewCoroutine->Context.State = ECoroutineState::Created;
    
    // 设置协程栈
    if (!SetupCoroutineStack(NewCoroutine, StackSize))
    {
        CLogger::Error("CCoroutineScheduler::CreateCoroutine: Failed to setup coroutine stack");
        delete NewCoroutine;
        return NCoroutineHandle::Invalid();
    }
    
    // 添加到协程列表和就绪队列
    Coroutines.PushBack(NewCoroutine);
    ReadyQueue.PushBack(NewCoroutine);
    
    CLogger::Info("CCoroutineScheduler: Created coroutine {}", NewCoroutine->Id);
    return NCoroutineHandle(NewCoroutine->Id);
}

void CCoroutineScheduler::ResumeCoroutine(uint64_t CoroutineId)
{
    auto* Coroutine = FindCoroutine(CoroutineId);
    if (!Coroutine)
    {
        return;
    }
    
    if (Coroutine->Context.State == ECoroutineState::Suspended)
    {
        Coroutine->Context.State = ECoroutineState::Created;
        
        // 从挂起队列移除，添加到就绪队列
        for (auto It = SuspendedQueue.Begin(); It != SuspendedQueue.End(); ++It)
        {
            if ((*It)->Id == CoroutineId)
            {
                SuspendedQueue.Erase(It);
                break;
            }
        }
        
        ReadyQueue.PushBack(Coroutine);
        CLogger::Debug("CCoroutineScheduler: Resumed coroutine {}", CoroutineId);
    }
}

void CCoroutineScheduler::SuspendCoroutine(uint64_t CoroutineId)
{
    auto* Coroutine = FindCoroutine(CoroutineId);
    if (!Coroutine)
    {
        return;
    }
    
    if (Coroutine->Context.State == ECoroutineState::Running)
    {
        Coroutine->Context.State = ECoroutineState::Suspended;
        SuspendedQueue.PushBack(Coroutine);
        CLogger::Debug("CCoroutineScheduler: Suspended coroutine {}", CoroutineId);
        
        if (CurrentCoroutineId == CoroutineId)
        {
            // 如果是当前协程，切换回主线程
            SwitchToMain();
        }
    }
}

void CCoroutineScheduler::AbortCoroutine(uint64_t CoroutineId)
{
    auto* Coroutine = FindCoroutine(CoroutineId);
    if (!Coroutine)
    {
        return;
    }
    
    Coroutine->Context.State = ECoroutineState::Aborted;
    CLogger::Debug("CCoroutineScheduler: Aborted coroutine {}", CoroutineId);
    
    if (CurrentCoroutineId == CoroutineId)
    {
        SwitchToMain();
    }
    
    RemoveCoroutine(CoroutineId);
}

void CCoroutineScheduler::YieldCoroutine()
{
    if (CurrentCoroutine && CurrentCoroutine->Context.State == ECoroutineState::Running)
    {
        CurrentCoroutine->Context.State = ECoroutineState::Suspended;
        SwitchToMain();
    }
}

void CCoroutineScheduler::Update()
{
    UpdateFor(-1); // 无时间限制
}

void CCoroutineScheduler::UpdateFor(int32_t MaxExecutionTimeMs)
{
    int64_t StartTime = GetCurrentTimeMs();
    int64_t EndTime = (MaxExecutionTimeMs > 0) ? StartTime + MaxExecutionTimeMs : INT64_MAX;
    
    while (HasPendingCoroutines() && GetCurrentTimeMs() < EndTime)
    {
        // 检查睡眠的协程
        int64_t CurrentTime = GetCurrentTimeMs();
        for (auto It = SuspendedQueue.Begin(); It != SuspendedQueue.End();)
        {
            auto* Coroutine = *It;
            if (Coroutine->SleepUntil > 0 && CurrentTime >= Coroutine->SleepUntil)
            {
                Coroutine->SleepUntil = 0;
                Coroutine->Context.State = ECoroutineState::Created;
                ReadyQueue.PushBack(Coroutine);
                It = SuspendedQueue.Erase(It);
            }
            else
            {
                ++It;
            }
        }
        
        ScheduleNextCoroutine();
    }
}

bool CCoroutineScheduler::HasPendingCoroutines() const
{
    return !ReadyQueue.IsEmpty() || 
           (SuspendedQueue.GetSize() > 0 && SuspendedQueue.FindIf([](const CoroutineInfo* Info) { 
               return Info->SleepUntil > 0; 
           }) != SuspendedQueue.End());
}

size_t CCoroutineScheduler::GetCoroutineCount() const
{
    return Coroutines.GetSize();
}

size_t CCoroutineScheduler::GetActiveCoroutineCount() const
{
    return ReadyQueue.GetSize() + (CurrentCoroutine ? 1 : 0);
}

size_t CCoroutineScheduler::GetSuspendedCoroutineCount() const
{
    return SuspendedQueue.GetSize();
}

uint64_t CCoroutineScheduler::GetCurrentCoroutineId() const
{
    return CurrentCoroutineId;
}

bool CCoroutineScheduler::IsInCoroutine() const
{
    return CurrentCoroutine != nullptr;
}

CCoroutineScheduler& CCoroutineScheduler::GetGlobalScheduler()
{
    if (!GlobalScheduler)
    {
        GlobalScheduler = NewNObject<CCoroutineScheduler>();
    }
    return *GlobalScheduler;
}

void CCoroutineScheduler::Yield()
{
    GetGlobalScheduler().YieldCoroutine();
}

void CCoroutineScheduler::YieldFor(int32_t Milliseconds)
{
    if (Milliseconds > 0)
    {
        Sleep(Milliseconds);
    }
    else
    {
        Yield();
    }
}

void CCoroutineScheduler::Sleep(int32_t Milliseconds)
{
    auto& Scheduler = GetGlobalScheduler();
    if (Scheduler.CurrentCoroutine)
    {
        Scheduler.CurrentCoroutine->SleepUntil = Scheduler.GetCurrentTimeMs() + Milliseconds;
        Scheduler.SuspendCoroutine(Scheduler.CurrentCoroutineId);
    }
}

CCoroutineScheduler::CoroutineInfo* CCoroutineScheduler::FindCoroutine(uint64_t CoroutineId)
{
    auto It = Coroutines.FindIf([CoroutineId](const CoroutineInfo* Info) {
        return Info->Id == CoroutineId;
    });
    
    return It != Coroutines.End() ? *It : nullptr;
}

void CCoroutineScheduler::RemoveCoroutine(uint64_t CoroutineId)
{
    // 从就绪队列移除
    for (auto It = ReadyQueue.Begin(); It != ReadyQueue.End(); ++It)
    {
        if ((*It)->Id == CoroutineId)
        {
            ReadyQueue.Erase(It);
            break;
        }
    }
    
    // 从挂起队列移除
    for (auto It = SuspendedQueue.Begin(); It != SuspendedQueue.End(); ++It)
    {
        if ((*It)->Id == CoroutineId)
        {
            SuspendedQueue.Erase(It);
            break;
        }
    }
    
    // 从协程列表移除并释放内存
    for (auto It = Coroutines.Begin(); It != Coroutines.End(); ++It)
    {
        if ((*It)->Id == CoroutineId)
        {
            delete *It;
            Coroutines.Erase(It);
            break;
        }
    }
}

void CCoroutineScheduler::ScheduleNextCoroutine()
{
    if (ReadyQueue.IsEmpty())
    {
        return;
    }
    
    // 简单的优先级调度：按优先级排序
    ReadyQueue.Sort([](const CoroutineInfo* A, const CoroutineInfo* B) {
        return static_cast<uint32_t>(A->Priority) > static_cast<uint32_t>(B->Priority);
    });
    
    auto* NextCoroutine = ReadyQueue[0];
    ReadyQueue.Erase(ReadyQueue.Begin());
    
    SwitchToCoroutine(NextCoroutine);
}

bool CCoroutineScheduler::SetupCoroutineStack(CoroutineInfo* Coroutine, size_t StackSize)
{
    if (!Coroutine)
    {
        return false;
    }
    
    // 对齐栈大小到页边界
    size_t PageSize = 4096; // 简化实现，实际应该查询系统页大小
    StackSize = ((StackSize + PageSize - 1) / PageSize) * PageSize;
    
#ifdef _WIN32
    Coroutine->Context.Stack.StackMemory = VirtualAlloc(nullptr, StackSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!Coroutine->Context.Stack.StackMemory)
    {
        CLogger::Error("CCoroutineScheduler: Failed to allocate stack memory (Windows)");
        return false;
    }
#else
    Coroutine->Context.Stack.StackMemory = mmap(nullptr, StackSize, PROT_READ | PROT_WRITE, 
                                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (Coroutine->Context.Stack.StackMemory == MAP_FAILED)
    {
        CLogger::Error("CCoroutineScheduler: Failed to allocate stack memory (Unix)");
        return false;
    }
#endif
    
    Coroutine->Context.Stack.StackSize = StackSize;
    Coroutine->Context.Stack.StackBottom = Coroutine->Context.Stack.StackMemory;
    Coroutine->Context.Stack.StackTop = static_cast<char*>(Coroutine->Context.Stack.StackMemory) + StackSize;
    
    return true;
}

void CCoroutineScheduler::SwitchToCoroutine(CoroutineInfo* Coroutine)
{
    if (!Coroutine)
    {
        return;
    }
    
    CurrentCoroutine = Coroutine;
    CurrentCoroutineId = Coroutine->Id;
    Coroutine->Context.State = ECoroutineState::Running;
    
    // 保存当前上下文
    if (setjmp(MainContext) == 0)
    {
        // 首次调用或从longjmp返回
        if (Coroutine->Context.State == ECoroutineState::Running)
        {
            if (setjmp(Coroutine->Context.JumpBuffer) == 0)
            {
                // 设置协程栈并跳转到协程入口
                CoroutineEntry(Coroutine);
            }
        }
    }
    
    // 协程执行完毕或被挂起后返回到这里
    if (CurrentCoroutine && CurrentCoroutine->Context.State == ECoroutineState::Running)
    {
        CurrentCoroutine->Context.State = ECoroutineState::Completed;
        RemoveCoroutine(CurrentCoroutine->Id);
    }
    
    CurrentCoroutine = nullptr;
    CurrentCoroutineId = 0;
}

void CCoroutineScheduler::SwitchToMain()
{
    if (CurrentCoroutine)
    {
        // 保存协程上下文
        if (setjmp(CurrentCoroutine->Context.JumpBuffer) == 0)
        {
            // 跳转回主上下文
            longjmp(MainContext, 1);
        }
    }
}

int64_t CCoroutineScheduler::GetCurrentTimeMs() const
{
    auto Now = std::chrono::steady_clock::now();
    auto Duration = Now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(Duration).count();
}

void CCoroutineScheduler::CoroutineEntry(CoroutineInfo* Coroutine)
{
    if (!Coroutine || !Coroutine->Function)
    {
        return;
    }
    
    // 设置栈指针（平台特定实现）
#ifdef _WIN32
    // Windows下的栈设置（简化版本）
    __asm {
        mov esp, Coroutine->Context.Stack.StackTop
    }
#elif defined(__x86_64__)
    // x86_64下的栈设置
    asm volatile("movq %0, %%rsp" : : "r"(Coroutine->Context.Stack.StackTop));
#elif defined(__i386__)
    // x86下的栈设置
    asm volatile("movl %0, %%esp" : : "r"(Coroutine->Context.Stack.StackTop));
#endif
    
    try
    {
        // 调用协程函数
        Coroutine->Function(Coroutine->UserData);
    }
    catch (...)
    {
        CLogger::Error("CCoroutineScheduler: Exception in coroutine {}", Coroutine->Id);
    }
    
    // 协程正常结束
    Coroutine->Context.State = ECoroutineState::Completed;
    
    // 跳转回调度器
    auto& Scheduler = GetGlobalScheduler();
    longjmp(Scheduler.MainContext, 1);
}

// === 等待器实现 ===

NTimeAwaiter::NTimeAwaiter(int32_t DelayMs)
{
    EndTime = GetCurrentTimeMs() + DelayMs;
}

bool NTimeAwaiter::IsReady()
{
    return GetCurrentTimeMs() >= EndTime;
}

void NTimeAwaiter::Await()
{
    while (!IsReady())
    {
        CCoroutineScheduler::Yield();
    }
}

int64_t NTimeAwaiter::GetCurrentTimeMs() const
{
    auto Now = std::chrono::steady_clock::now();
    auto Duration = Now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(Duration).count();
}

NCoroutineWaitAwaiter::NCoroutineWaitAwaiter(const NCoroutineHandle& Handle)
    : TargetHandle(Handle)
{}

bool NCoroutineWaitAwaiter::IsReady()
{
    return !TargetHandle.IsValid() || TargetHandle.IsCompleted();
}

void NCoroutineWaitAwaiter::Await()
{
    while (!IsReady())
    {
        CCoroutineScheduler::Yield();
    }
}

// === 协程同步原语实现 ===

NCoroutineSemaphore::NCoroutineSemaphore(int32_t InitialCount)
    : Count(InitialCount)
{}

NCoroutineSemaphore::~NCoroutineSemaphore()
{
    WakeupWaitingCoroutines();
}

void NCoroutineSemaphore::Wait()
{
    while (Count <= 0)
    {
        uint64_t CurrentId = CCoroutineScheduler::GetGlobalScheduler().GetCurrentCoroutineId();
        if (CurrentId != 0)
        {
            WaitingCoroutines.PushBack(CurrentId);
        }
        CCoroutineScheduler::Yield();
    }
    --Count;
}

bool NCoroutineSemaphore::TryWait()
{
    if (Count > 0)
    {
        --Count;
        return true;
    }
    return false;
}

void NCoroutineSemaphore::Post()
{
    ++Count;
    WakeupWaitingCoroutines();
}

void NCoroutineSemaphore::Post(int32_t PostCount)
{
    Count += PostCount;
    WakeupWaitingCoroutines();
}

void NCoroutineSemaphore::WakeupWaitingCoroutines()
{
    for (uint64_t CoroutineId : WaitingCoroutines)
    {
        CCoroutineScheduler::GetGlobalScheduler().ResumeCoroutine(CoroutineId);
    }
    WaitingCoroutines.Clear();
}

NCoroutineMutex::NCoroutineMutex()
    : bIsLocked(false), OwnerCoroutine(0)
{}

NCoroutineMutex::~NCoroutineMutex()
{
    WakeupNextWaiter();
}

void NCoroutineMutex::Lock()
{
    uint64_t CurrentId = CCoroutineScheduler::GetGlobalScheduler().GetCurrentCoroutineId();
    
    while (bIsLocked && OwnerCoroutine != CurrentId)
    {
        WaitingCoroutines.PushBack(CurrentId);
        CCoroutineScheduler::Yield();
    }
    
    bIsLocked = true;
    OwnerCoroutine = CurrentId;
}

bool NCoroutineMutex::TryLock()
{
    if (!bIsLocked)
    {
        bIsLocked = true;
        OwnerCoroutine = CCoroutineScheduler::GetGlobalScheduler().GetCurrentCoroutineId();
        return true;
    }
    return false;
}

void NCoroutineMutex::Unlock()
{
    uint64_t CurrentId = CCoroutineScheduler::GetGlobalScheduler().GetCurrentCoroutineId();
    
    if (bIsLocked && OwnerCoroutine == CurrentId)
    {
        bIsLocked = false;
        OwnerCoroutine = 0;
        WakeupNextWaiter();
    }
}

void NCoroutineMutex::WakeupNextWaiter()
{
    if (!WaitingCoroutines.IsEmpty())
    {
        uint64_t NextWaiter = WaitingCoroutines[0];
        WaitingCoroutines.Erase(WaitingCoroutines.Begin());
        CCoroutineScheduler::GetGlobalScheduler().ResumeCoroutine(NextWaiter);
    }
}

// === 协程工具函数实现 ===

NCoroutineHandle CreateCoroutine(CCoroutineScheduler::CoroutineFunction Function, void* UserData)
{
    return CCoroutineScheduler::GetGlobalScheduler().CreateCoroutine(Function, UserData);
}

void AwaitCoroutine(const NCoroutineHandle& Handle)
{
    NCoroutineWaitAwaiter Awaiter(Handle);
    Awaiter.Await();
}

void AwaitTime(int32_t Milliseconds)
{
    NTimeAwaiter Awaiter(Milliseconds);
    Awaiter.Await();
}

void AwaitAll(const CArray<NCoroutineHandle>& Handles)
{
    for (const auto& Handle : Handles)
    {
        AwaitCoroutine(Handle);
    }
}

NCoroutineHandle AwaitAny(const CArray<NCoroutineHandle>& Handles)
{
    while (true)
    {
        for (const auto& Handle : Handles)
        {
            if (Handle.IsCompleted())
            {
                return Handle;
            }
        }
        CCoroutineScheduler::Yield();
    }
}

} // namespace NLib