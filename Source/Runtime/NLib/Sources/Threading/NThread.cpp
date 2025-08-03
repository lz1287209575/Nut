#include "Threading/CThread.h"
#include "Core/CLogger.h"
#include "Core/CAllocator.h"

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <process.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <signal.h>
#endif

namespace NLib
{

// === CThread 实现 ===

CAtomic<uint32_t> CThread::NextThreadId(1);

CThread::CThread()
    : ThreadId(NextThreadId++)
    , ThreadName(CString::Format("Thread_{}", ThreadId))
    , bIsRunning(false)
    , bShouldStop(false)
    , bIsJoined(false)
    , bIsDetached(false)
    , ThreadHandle(nullptr)
    , SystemThreadId(0)
{
}

CThread::CThread(const NFunction<void()>& InFunction, const CString& InName)
    : CThread()
{
    Function = InFunction;
    if (!InName.IsEmpty())
    {
        ThreadName = InName;
    }
}

CThread::CThread(ThreadFunction InFunction, void* InData, const CString& InName)
    : CThread()
{
    CFunction = InFunction;
    UserData = InData;
    if (!InName.IsEmpty())
    {
        ThreadName = InName;
    }
}

CThread::~CThread()
{
    if (IsRunning() && !bIsDetached)
    {
        Stop();
        if (IsJoinable())
        {
            Join();
        }
    }
    
    CleanupHandle();
}

bool CThread::Start()
{
    if (IsRunning())
    {
        CLogger::Get().LogWarning("Thread {} is already running", ThreadName);
        return false;
    }
    
    if (!Function && !CFunction)
    {
        CLogger::Get().LogError("Thread {} has no function to execute", ThreadName);
        return false;
    }
    
    bShouldStop = false;
    bIsJoined = false;
    bIsDetached = false;
    
#ifdef PLATFORM_WINDOWS
    ThreadHandle = (void*)_beginthreadex(
        nullptr,
        0,
        &CThread::ThreadEntryPoint,
        this,
        0,
        &SystemThreadId
    );
    
    if (!ThreadHandle)
    {
        CLogger::Get().LogError("Failed to create thread {}: {}", ThreadName, GetLastError());
        return false;
    }
#else
    pthread_t* PthreadHandle = CAllocator::New<pthread_t>();
    ThreadHandle = PthreadHandle;
    
    int Result = pthread_create(PthreadHandle, nullptr, &CThread::ThreadEntryPoint, this);
    if (Result != 0)
    {
        CLogger::Get().LogError("Failed to create thread {}: {}", ThreadName, Result);
        CAllocator::Delete(PthreadHandle);
        ThreadHandle = nullptr;
        return false;
    }
    
    SystemThreadId = static_cast<uint32_t>(syscall(SYS_gettid));
#endif
    
    bIsRunning = true;
    
    CLogger::Get().LogInfo("Started thread {} (ID: {})", ThreadName, ThreadId);
    return true;
}

void CThread::Stop()
{
    if (!IsRunning())
    {
        return;
    }
    
    bShouldStop = true;
    CLogger::Get().LogInfo("Requested stop for thread {}", ThreadName);
}

void CThread::Join()
{
    if (!IsRunning() || bIsDetached || bIsJoined || !ThreadHandle)
    {
        return;
    }
    
#ifdef PLATFORM_WINDOWS
    DWORD Result = WaitForSingleObject((HANDLE)ThreadHandle, INFINITE);
    if (Result != WAIT_OBJECT_0)
    {
        CLogger::Get().LogError("Failed to join thread {}: {}", ThreadName, GetLastError());
    }
#else
    pthread_t* PthreadHandle = static_cast<pthread_t*>(ThreadHandle);
    int Result = pthread_join(*PthreadHandle, nullptr);
    if (Result != 0)
    {
        CLogger::Get().LogError("Failed to join thread {}: {}", ThreadName, Result);
    }
#endif
    
    bIsJoined = true;
    CLogger::Get().LogInfo("Joined thread {}", ThreadName);
}

bool CThread::Join(int32_t TimeoutMs)
{
    if (!IsRunning() || bIsDetached || bIsJoined || !ThreadHandle)
    {
        return true;
    }
    
#ifdef PLATFORM_WINDOWS
    DWORD Result = WaitForSingleObject((HANDLE)ThreadHandle, TimeoutMs);
    if (Result == WAIT_OBJECT_0)
    {
        bIsJoined = true;
        return true;
    }
    else if (Result == WAIT_TIMEOUT)
    {
        return false;
    }
    else
    {
        CLogger::Get().LogError("Failed to join thread {} with timeout: {}", ThreadName, GetLastError());
        return false;
    }
#else
    // Linux doesn't have native timed join, use a polling approach
    int32_t ElapsedMs = 0;
    const int32_t PollIntervalMs = 10;
    
    while (ElapsedMs < TimeoutMs && IsRunning())
    {
        Sleep(PollIntervalMs);
        ElapsedMs += PollIntervalMs;
    }
    
    if (!IsRunning())
    {
        Join(); // Regular join if thread finished
        return true;
    }
    
    return false; // Timeout
#endif
}

void CThread::Detach()
{
    if (bIsDetached || !ThreadHandle)
    {
        return;
    }
    
#ifdef PLATFORM_WINDOWS
    CloseHandle((HANDLE)ThreadHandle);
    ThreadHandle = nullptr;
#else
    pthread_t* PthreadHandle = static_cast<pthread_t*>(ThreadHandle);
    pthread_detach(*PthreadHandle);
    CAllocator::Delete(PthreadHandle);
    ThreadHandle = nullptr;
#endif
    
    bIsDetached = true;
    CLogger::Get().LogInfo("Detached thread {}", ThreadName);
}

bool CThread::IsRunning() const
{
    return bIsRunning;
}

bool CThread::IsJoinable() const
{
    return IsRunning() && !bIsDetached && !bIsJoined && ThreadHandle != nullptr;
}

bool CThread::ShouldStop() const
{
    return bShouldStop;
}

uint32_t CThread::GetThreadId() const
{
    return ThreadId;
}

const CString& CThread::GetThreadName() const
{
    return ThreadName;
}

void CThread::SetThreadName(const CString& NewName)
{
    ThreadName = NewName;
    
    // 设置系统线程名称
#ifdef PLATFORM_WINDOWS
    // Windows 10 version 1607 and later
    typedef HRESULT(WINAPI* SetThreadDescriptionFunc)(HANDLE, PCWSTR);
    static SetThreadDescriptionFunc SetThreadDescriptionPtr = nullptr;
    static bool bChecked = false;
    
    if (!bChecked)
    {
        bChecked = true;
        HMODULE KernelBase = GetModuleHandleA("KernelBase.dll");
        if (KernelBase)
        {
            SetThreadDescriptionPtr = (SetThreadDescriptionFunc)GetProcAddress(KernelBase, "SetThreadDescription");
        }
    }
    
    if (SetThreadDescriptionPtr && ThreadHandle)
    {
        std::wstring WideName(ThreadName.GetCStr(), ThreadName.GetCStr() + ThreadName.GetLength());
        SetThreadDescriptionPtr((HANDLE)ThreadHandle, WideName.c_str());
    }
#else
    if (ThreadHandle)
    {
        pthread_t* PthreadHandle = static_cast<pthread_t*>(ThreadHandle);
        pthread_setname_np(*PthreadHandle, ThreadName.GetCStr());
    }
#endif
}

uint32_t CThread::GetCurrentThreadId()
{
#ifdef PLATFORM_WINDOWS
    return GetCurrentThreadId();
#else
    return static_cast<uint32_t>(syscall(SYS_gettid));
#endif
}

void CThread::Sleep(int32_t Milliseconds)
{
#ifdef PLATFORM_WINDOWS
    ::Sleep(Milliseconds);
#else
    usleep(Milliseconds * 1000);
#endif
}

void CThread::Yield()
{
#ifdef PLATFORM_WINDOWS
    SwitchToThread();
#else
    sched_yield();
#endif
}

uint32_t CThread::GetHardwareConcurrency()
{
#ifdef PLATFORM_WINDOWS
    SYSTEM_INFO SysInfo;
    GetSystemInfo(&SysInfo);
    return SysInfo.dwNumberOfProcessors;
#else
    return sysconf(_SC_NPROCESSORS_ONLN);
#endif
}

void CThread::ThreadMain()
{
    try
    {
        if (Function)
        {
            Function();
        }
        else if (CFunction)
        {
            CFunction(UserData);
        }
    }
    catch (...)
    {
        CLogger::Get().LogError("Exception in thread {}", ThreadName);
    }
    
    bIsRunning = false;
    CLogger::Get().LogInfo("Thread {} finished execution", ThreadName);
}

void CThread::CleanupHandle()
{
    if (ThreadHandle)
    {
#ifdef PLATFORM_WINDOWS
        if (!bIsDetached)
        {
            CloseHandle((HANDLE)ThreadHandle);
        }
#else
        if (!bIsDetached)
        {
            pthread_t* PthreadHandle = static_cast<pthread_t*>(ThreadHandle);
            CAllocator::Delete(PthreadHandle);
        }
#endif
        ThreadHandle = nullptr;
    }
}

#ifdef PLATFORM_WINDOWS
unsigned int __stdcall CThread::ThreadEntryPoint(void* ThreadPtr)
#else
void* CThread::ThreadEntryPoint(void* ThreadPtr)
#endif
{
    CThread* Thread = static_cast<CThread*>(ThreadPtr);
    if (Thread)
    {
        Thread->ThreadMain();
    }
    
#ifdef PLATFORM_WINDOWS
    return 0;
#else
    return nullptr;
#endif
}

// === NMutex 实现 ===

NMutex::NMutex()
{
#ifdef PLATFORM_WINDOWS
    MutexHandle = CreateMutexA(nullptr, FALSE, nullptr);
    if (!MutexHandle)
    {
        CLogger::Get().LogError("Failed to create mutex: {}", GetLastError());
    }
#else
    pthread_mutex_t* PthreadMutex = CAllocator::New<pthread_mutex_t>();
    MutexHandle = PthreadMutex;
    
    int Result = pthread_mutex_init(PthreadMutex, nullptr);
    if (Result != 0)
    {
        CLogger::Get().LogError("Failed to initialize mutex: {}", Result);
        CAllocator::Delete(PthreadMutex);
        MutexHandle = nullptr;
    }
#endif
}

NMutex::~NMutex()
{
    if (MutexHandle)
    {
#ifdef PLATFORM_WINDOWS
        CloseHandle((HANDLE)MutexHandle);
#else
        pthread_mutex_t* PthreadMutex = static_cast<pthread_mutex_t*>(MutexHandle);
        pthread_mutex_destroy(PthreadMutex);
        CAllocator::Delete(PthreadMutex);
#endif
    }
}

void NMutex::Lock()
{
    if (!MutexHandle)
    {
        return;
    }
    
#ifdef PLATFORM_WINDOWS
    WaitForSingleObject((HANDLE)MutexHandle, INFINITE);
#else
    pthread_mutex_t* PthreadMutex = static_cast<pthread_mutex_t*>(MutexHandle);
    pthread_mutex_lock(PthreadMutex);
#endif
}

void NMutex::Unlock()
{
    if (!MutexHandle)
    {
        return;
    }
    
#ifdef PLATFORM_WINDOWS
    ReleaseMutex((HANDLE)MutexHandle);
#else
    pthread_mutex_t* PthreadMutex = static_cast<pthread_mutex_t*>(MutexHandle);
    pthread_mutex_unlock(PthreadMutex);
#endif
}

bool NMutex::TryLock()
{
    if (!MutexHandle)
    {
        return false;
    }
    
#ifdef PLATFORM_WINDOWS
    DWORD Result = WaitForSingleObject((HANDLE)MutexHandle, 0);
    return Result == WAIT_OBJECT_0;
#else
    pthread_mutex_t* PthreadMutex = static_cast<pthread_mutex_t*>(MutexHandle);
    return pthread_mutex_trylock(PthreadMutex) == 0;
#endif
}

// === NEvent 实现 ===

NEvent::NEvent()
    : bSignaled(false)
    , bAutoReset(true)
{
#ifdef PLATFORM_WINDOWS
    EventHandle = CreateEventA(nullptr, !bAutoReset, FALSE, nullptr);
    if (!EventHandle)
    {
        CLogger::Get().LogError("Failed to create event: {}", GetLastError());
    }
#else
    EventData = CAllocator::New<EventDataStruct>();
    EventData->Signaled = false;
    
    int Result = pthread_mutex_init(&EventData->Mutex, nullptr);
    if (Result == 0)
    {
        Result = pthread_cond_init(&EventData->Condition, nullptr);
    }
    
    if (Result != 0)
    {
        CLogger::Get().LogError("Failed to initialize event: {}", Result);
        if (EventData)
        {
            pthread_mutex_destroy(&EventData->Mutex);
            CAllocator::Delete(EventData);
            EventData = nullptr;
        }
    }
#endif
}

NEvent::NEvent(bool bInAutoReset)
    : NEvent()
{
    bAutoReset = bInAutoReset;
    
#ifdef PLATFORM_WINDOWS
    if (EventHandle)
    {
        CloseHandle((HANDLE)EventHandle);
        EventHandle = CreateEventA(nullptr, !bAutoReset, FALSE, nullptr);
    }
#endif
}

NEvent::~NEvent()
{
#ifdef PLATFORM_WINDOWS
    if (EventHandle)
    {
        CloseHandle((HANDLE)EventHandle);
    }
#else
    if (EventData)
    {
        pthread_mutex_destroy(&EventData->Mutex);
        pthread_cond_destroy(&EventData->Condition);
        CAllocator::Delete(EventData);
    }
#endif
}

void NEvent::Set()
{
#ifdef PLATFORM_WINDOWS
    if (EventHandle)
    {
        SetEvent((HANDLE)EventHandle);
    }
#else
    if (EventData)
    {
        pthread_mutex_lock(&EventData->Mutex);
        EventData->Signaled = true;
        if (bAutoReset)
        {
            pthread_cond_signal(&EventData->Condition);
        }
        else
        {
            pthread_cond_broadcast(&EventData->Condition);
        }
        pthread_mutex_unlock(&EventData->Mutex);
    }
#endif
    
    bSignaled = true;
}

void NEvent::Reset()
{
#ifdef PLATFORM_WINDOWS
    if (EventHandle)
    {
        ResetEvent((HANDLE)EventHandle);
    }
#else
    if (EventData)
    {
        pthread_mutex_lock(&EventData->Mutex);
        EventData->Signaled = false;
        pthread_mutex_unlock(&EventData->Mutex);
    }
#endif
    
    bSignaled = false;
}

void NEvent::Wait()
{
#ifdef PLATFORM_WINDOWS
    if (EventHandle)
    {
        WaitForSingleObject((HANDLE)EventHandle, INFINITE);
    }
#else
    if (EventData)
    {
        pthread_mutex_lock(&EventData->Mutex);
        while (!EventData->Signaled)
        {
            pthread_cond_wait(&EventData->Condition, &EventData->Mutex);
        }
        
        if (bAutoReset)
        {
            EventData->Signaled = false;
        }
        
        pthread_mutex_unlock(&EventData->Mutex);
    }
#endif
}

bool NEvent::WaitFor(int32_t TimeoutMs)
{
#ifdef PLATFORM_WINDOWS
    if (!EventHandle)
    {
        return false;
    }
    
    DWORD Result = WaitForSingleObject((HANDLE)EventHandle, TimeoutMs);
    return Result == WAIT_OBJECT_0;
#else
    if (!EventData)
    {
        return false;
    }
    
    struct timespec AbsTime;
    clock_gettime(CLOCK_REALTIME, &AbsTime);
    
    AbsTime.tv_sec += TimeoutMs / 1000;
    AbsTime.tv_nsec += (TimeoutMs % 1000) * 1000000;
    if (AbsTime.tv_nsec >= 1000000000)
    {
        AbsTime.tv_sec++;
        AbsTime.tv_nsec -= 1000000000;
    }
    
    pthread_mutex_lock(&EventData->Mutex);
    
    bool Success = true;
    while (!EventData->Signaled && Success)
    {
        int Result = pthread_cond_timedwait(&EventData->Condition, &EventData->Mutex, &AbsTime);
        if (Result == ETIMEDOUT)
        {
            Success = false;
        }
        else if (Result != 0)
        {
            Success = false;
            break;
        }
    }
    
    if (Success && bAutoReset)
    {
        EventData->Signaled = false;
    }
    
    pthread_mutex_unlock(&EventData->Mutex);
    return Success;
#endif
}

bool NEvent::IsSignaled() const
{
    return bSignaled;
}

// === NConditionVariable 实现 ===

NConditionVariable::NConditionVariable()
{
#ifdef PLATFORM_WINDOWS
    InitializeConditionVariable((PCONDITION_VARIABLE)&ConditionHandle);
#else
    pthread_cond_t* PthreadCond = CAllocator::New<pthread_cond_t>();
    ConditionHandle = PthreadCond;
    
    int Result = pthread_cond_init(PthreadCond, nullptr);
    if (Result != 0)
    {
        CLogger::Get().LogError("Failed to initialize condition variable: {}", Result);
        CAllocator::Delete(PthreadCond);
        ConditionHandle = nullptr;
    }
#endif
}

NConditionVariable::~NConditionVariable()
{
#ifdef PLATFORM_WINDOWS
    // No cleanup needed for Windows condition variables
#else
    if (ConditionHandle)
    {
        pthread_cond_t* PthreadCond = static_cast<pthread_cond_t*>(ConditionHandle);
        pthread_cond_destroy(PthreadCond);
        CAllocator::Delete(PthreadCond);
    }
#endif
}

void NConditionVariable::Wait(NUniqueLock<NMutex>& Lock)
{
    if (!ConditionHandle)
    {
        return;
    }
    
#ifdef PLATFORM_WINDOWS
    SleepConditionVariableCS((PCONDITION_VARIABLE)&ConditionHandle, 
                           (PCRITICAL_SECTION)Lock.GetMutex()->MutexHandle, 
                           INFINITE);
#else
    pthread_cond_t* PthreadCond = static_cast<pthread_cond_t*>(ConditionHandle);
    pthread_mutex_t* PthreadMutex = static_cast<pthread_mutex_t*>(Lock.GetMutex()->MutexHandle);
    pthread_cond_wait(PthreadCond, PthreadMutex);
#endif
}

bool NConditionVariable::WaitFor(NUniqueLock<NMutex>& Lock, int32_t TimeoutMs)
{
    if (!ConditionHandle)
    {
        return false;
    }
    
#ifdef PLATFORM_WINDOWS
    BOOL Result = SleepConditionVariableCS((PCONDITION_VARIABLE)&ConditionHandle,
                                         (PCRITICAL_SECTION)Lock.GetMutex()->MutexHandle,
                                         TimeoutMs);
    return Result != 0;
#else
    struct timespec AbsTime;
    clock_gettime(CLOCK_REALTIME, &AbsTime);
    
    AbsTime.tv_sec += TimeoutMs / 1000;
    AbsTime.tv_nsec += (TimeoutMs % 1000) * 1000000;
    if (AbsTime.tv_nsec >= 1000000000)
    {
        AbsTime.tv_sec++;
        AbsTime.tv_nsec -= 1000000000;
    }
    
    pthread_cond_t* PthreadCond = static_cast<pthread_cond_t*>(ConditionHandle);
    pthread_mutex_t* PthreadMutex = static_cast<pthread_mutex_t*>(Lock.GetMutex()->MutexHandle);
    
    int Result = pthread_cond_timedwait(PthreadCond, PthreadMutex, &AbsTime);
    return Result == 0;
#endif
}

void NConditionVariable::NotifyOne()
{
    if (!ConditionHandle)
    {
        return;
    }
    
#ifdef PLATFORM_WINDOWS
    WakeConditionVariable((PCONDITION_VARIABLE)&ConditionHandle);
#else
    pthread_cond_t* PthreadCond = static_cast<pthread_cond_t*>(ConditionHandle);
    pthread_cond_signal(PthreadCond);
#endif
}

void NConditionVariable::NotifyAll()
{
    if (!ConditionHandle)
    {
        return;
    }
    
#ifdef PLATFORM_WINDOWS
    WakeAllConditionVariable((PCONDITION_VARIABLE)&ConditionHandle);
#else
    pthread_cond_t* PthreadCond = static_cast<pthread_cond_t*>(ConditionHandle);
    pthread_cond_broadcast(PthreadCond);
#endif
}

} // namespace NLib