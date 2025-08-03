#pragma once

#include "Network/NIOEvent.h"
#include "Core/CObject.h"
#include "Threading/CThread.h"
#include "Containers/CHashMap.h"
#include "DateTime/NDateTime.h"

#ifdef PLATFORM_WINDOWS
#include <winsock2.h>
#include <mswsock.h>
#include <windows.h>
#elif defined(PLATFORM_LINUX)
#include <sys/epoll.h>
#elif defined(PLATFORM_MACOS) || defined(PLATFORM_BSD)
#include <sys/event.h>
#include <sys/time.h>
#endif

namespace NLib
{

/**
 * @brief 事件循环模式
 */
enum class EEventLoopMode : uint32_t
{
    Reactor,        // Reactor模式 (Linux epoll, BSD kqueue, Windows select)
    Proactor        // Proactor模式 (Windows IOCP, Linux io_uring)
};

/**
 * @brief 定时器信息
 */
struct LIBNLIB_API NTimerInfo
{
    uint64_t TimerId;               // 定时器ID
    NDateTime ExpireTime;           // 到期时间
    int32_t IntervalMs;             // 间隔时间(毫秒)，0表示单次定时器
    NFunction<void()> Callback;     // 回调函数
    void* UserData;                 // 用户数据
    bool bActive;                   // 是否激活
    
    NTimerInfo();
    NTimerInfo(uint64_t InTimerId, const NDateTime& InExpireTime, int32_t InIntervalMs, 
               const NFunction<void()>& InCallback, void* InUserData = nullptr);
    
    bool IsExpired() const;
    void UpdateExpireTime();
};

/**
 * @brief 套接字监听信息
 */
struct LIBNLIB_API NSocketWatchInfo
{
    SocketHandle Handle;            // 套接字句柄
    EIOEventType EventMask;         // 监听的事件类型
    IIOEventHandler* Handler;       // 事件处理器
    void* UserData;                 // 用户数据
    bool bActive;                   // 是否激活
    
    NSocketWatchInfo();
    NSocketWatchInfo(SocketHandle InHandle, EIOEventType InEventMask, 
                     IIOEventHandler* InHandler, void* InUserData = nullptr);
};

/**
 * @brief 抽象事件循环基类
 */
NCLASS()
class LIBNLIB_API NEventLoop : public CObject
{
    GENERATED_BODY()

public:
    NEventLoop();
    virtual ~NEventLoop();
    
    // 生命周期管理
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual bool IsInitialized() const { return bInitialized; }
    
    // 事件循环控制
    virtual void Run();                                 // 运行事件循环(阻塞)
    virtual void RunOnce(int32_t TimeoutMs = -1) = 0;  // 运行一次(可设置超时)
    virtual void Stop();                                // 停止事件循环
    virtual bool IsRunning() const { return bRunning; }
    
    // 套接字监听
    virtual bool AddSocket(SocketHandle Handle, EIOEventType EventMask, 
                          IIOEventHandler* Handler, void* UserData = nullptr) = 0;
    virtual bool ModifySocket(SocketHandle Handle, EIOEventType EventMask) = 0;
    virtual bool RemoveSocket(SocketHandle Handle) = 0;
    
    // 定时器管理
    virtual uint64_t AddTimer(int32_t DelayMs, const NFunction<void()>& Callback, 
                             void* UserData = nullptr);
    virtual uint64_t AddRepeatingTimer(int32_t IntervalMs, const NFunction<void()>& Callback, 
                                      void* UserData = nullptr);
    virtual bool RemoveTimer(uint64_t TimerId);
    virtual void ClearAllTimers();
    
    // 任务队列 (用于线程间通信)
    virtual void PostTask(const NFunction<void()>& Task);
    virtual void PostDelayedTask(int32_t DelayMs, const NFunction<void()>& Task);
    
    // 事件循环模式
    virtual EEventLoopMode GetMode() const = 0;
    
    // 统计信息
    struct Statistics
    {
        uint64_t EventsProcessed;       // 处理的事件数
        uint64_t TimersExecuted;        // 执行的定时器数
        uint64_t TasksExecuted;         // 执行的任务数
        double AverageLatency;          // 平均延迟(毫秒)
        uint64_t LastUpdateTime;        // 上次更新时间
        
        Statistics() : EventsProcessed(0), TimersExecuted(0), TasksExecuted(0), 
                      AverageLatency(0.0), LastUpdateTime(0) {}
        
        void Reset();
        CString ToString() const;
    };
    
    const Statistics& GetStatistics() const { return Stats; }
    void ResetStatistics() { Stats.Reset(); }
    
protected:
    // 内部方法
    virtual void ProcessTimers();
    virtual void ProcessTasks();
    virtual void UpdateStatistics();
    
    void SetInitialized(bool bInInitialized) { bInitialized = bInInitialized; }
    void UpdateEventCount() { Stats.EventsProcessed++; }
    void UpdateTimerCount() { Stats.TimersExecuted++; }
    void UpdateTaskCount() { Stats.TasksExecuted++; }
    
private:
    bool bInitialized;
    bool bRunning;
    bool bShouldStop;
    
    // 定时器管理
    CHashMap<uint64_t, NTimerInfo> Timers;
    CAtomic<uint64_t> NextTimerId;
    mutable NMutex TimerMutex;
    
    // 任务队列
    CArray<NFunction<void()>> TaskQueue;
    CArray<NTimerInfo> DelayedTasks;
    mutable NMutex TaskMutex;
    
    // 统计信息
    Statistics Stats;
    mutable NMutex StatsMutex;
};

#ifdef PLATFORM_WINDOWS
/**
 * @brief Windows IOCP事件循环 (Proactor模式)
 */
class LIBNLIB_API NIOCPEventLoop : public NEventLoop
{
    NCLASS()
class NIOCPEventLoop : public NObject
{
    GENERATED_BODY()

public:
    NIOCPEventLoop();
    virtual ~NIOCPEventLoop();
    
    // NEventLoop接口实现
    virtual bool Initialize() override;
    virtual void Shutdown() override;
    virtual void RunOnce(int32_t TimeoutMs = -1) override;
    
    virtual bool AddSocket(SocketHandle Handle, EIOEventType EventMask, 
                          IIOEventHandler* Handler, void* UserData = nullptr) override;
    virtual bool ModifySocket(SocketHandle Handle, EIOEventType EventMask) override;
    virtual bool RemoveSocket(SocketHandle Handle) override;
    
    virtual EEventLoopMode GetMode() const override { return EEventLoopMode::Proactor; }
    
    // IOCP特定方法
    bool PostAccept(SocketHandle ListenSocket, SocketHandle AcceptSocket, 
                   char* Buffer, int32_t BufferSize, TSharedPtr<NAsyncIOHandle> Handle);
    bool PostReceive(SocketHandle Socket, char* Buffer, int32_t BufferSize, 
                    TSharedPtr<NAsyncIOHandle> Handle);
    bool PostSend(SocketHandle Socket, const char* Buffer, int32_t BufferSize, 
                 TSharedPtr<NAsyncIOHandle> Handle);
    
    // 获取IOCP句柄
    HANDLE GetIOCPHandle() const { return IOCPHandle; }
    
private:
    HANDLE IOCPHandle;
    CHashMap<SocketHandle, NSocketWatchInfo> WatchedSockets;
    mutable NMutex SocketMutex;
    
    // IOCP相关结构
    struct IOCPOverlapped
    {
        OVERLAPPED Overlapped;
        EIOOperationType OpType;
        SocketHandle Socket;
        TSharedPtr<NAsyncIOHandle> AsyncHandle;
        
        IOCPOverlapped(EIOOperationType InOpType, SocketHandle InSocket, 
                      TSharedPtr<NAsyncIOHandle> InHandle);
    };
    
    void ProcessIOCompletion(DWORD BytesTransferred, ULONG_PTR CompletionKey, 
                           IOCPOverlapped* Overlapped);
};

/**
 * @brief Windows Select事件循环 (Reactor模式)
 */
NCLASS()
class LIBNLIB_API NSelectEventLoop : public NEventLoop
{
    GENERATED_BODY()
public:
    NSelectEventLoop();
    virtual ~NSelectEventLoop();
    
    // NEventLoop接口实现
    virtual bool Initialize() override;
    virtual void Shutdown() override;
    virtual void RunOnce(int32_t TimeoutMs = -1) override;
    
    virtual bool AddSocket(SocketHandle Handle, EIOEventType EventMask, 
                          IIOEventHandler* Handler, void* UserData = nullptr) override;
    virtual bool ModifySocket(SocketHandle Handle, EIOEventType EventMask) override;
    virtual bool RemoveSocket(SocketHandle Handle) override;
    
    virtual EEventLoopMode GetMode() const override { return EEventLoopMode::Reactor; }
    
private:
    CHashMap<SocketHandle, NSocketWatchInfo> WatchedSockets;
    mutable NMutex SocketMutex;
    
    void BuildFdSets(fd_set& ReadSet, fd_set& WriteSet, fd_set& ErrorSet, int& MaxFd);
    void ProcessFdSets(const fd_set& ReadSet, const fd_set& WriteSet, const fd_set& ErrorSet);
};

#elif defined(PLATFORM_LINUX)
/**
 * @brief Linux Epoll事件循环 (Reactor模式)
 */
NCLASS()
class LIBNLIB_API NEpollEventLoop : public NEventLoop
{
    GENERATED_BODY()

public:
    NEpollEventLoop();
    virtual ~NEpollEventLoop();
    
    // NEventLoop接口实现
    virtual bool Initialize() override;
    virtual void Shutdown() override;
    virtual void RunOnce(int32_t TimeoutMs = -1) override;
    
    virtual bool AddSocket(SocketHandle Handle, EIOEventType EventMask, 
                          IIOEventHandler* Handler, void* UserData = nullptr) override;
    virtual bool ModifySocket(SocketHandle Handle, EIOEventType EventMask) override;
    virtual bool RemoveSocket(SocketHandle Handle) override;
    
    virtual EEventLoopMode GetMode() const override { return EEventLoopMode::Reactor; }
    
    // Epoll特定方法
    int GetEpollFd() const { return EpollFd; }
    void SetMaxEvents(int32_t MaxEvents) { MaxEpollEvents = MaxEvents; }
    
private:
    int EpollFd;
    int32_t MaxEpollEvents;
    CHashMap<SocketHandle, NSocketWatchInfo> WatchedSockets;
    mutable NMutex SocketMutex;
    
    uint32_t EventTypeToEpollEvents(EIOEventType EventType);
    EIOEventType EpollEventsToEventType(uint32_t EpollEvents);
};

#elif defined(PLATFORM_MACOS) || defined(PLATFORM_BSD)
/**
 * @brief BSD/macOS Kqueue事件循环 (Reactor模式)
 */
NCLASS()
class LIBNLIB_API NKqueueEventLoop : public NEventLoop
{
    GENERATED_BODY()

public:
    NKqueueEventLoop();
    virtual ~NKqueueEventLoop();
    
    // NEventLoop接口实现
    virtual bool Initialize() override;
    virtual void Shutdown() override;
    virtual void RunOnce(int32_t TimeoutMs = -1) override;
    
    virtual bool AddSocket(SocketHandle Handle, EIOEventType EventMask, 
                          IIOEventHandler* Handler, void* UserData = nullptr) override;
    virtual bool ModifySocket(SocketHandle Handle, EIOEventType EventMask) override;
    virtual bool RemoveSocket(SocketHandle Handle) override;
    
    virtual EEventLoopMode GetMode() const override { return EEventLoopMode::Reactor; }
    
    // Kqueue特定方法
    int GetKqueueFd() const { return KqueueFd; }
    void SetMaxEvents(int32_t MaxEvents) { MaxKqueueEvents = MaxEvents; }
    
private:
    int KqueueFd;
    int32_t MaxKqueueEvents;
    CHashMap<SocketHandle, NSocketWatchInfo> WatchedSockets;
    mutable NMutex SocketMutex;
    
    void EventTypeToKevents(EIOEventType EventType, SocketHandle Handle, 
                           struct kevent* Events, int& EventCount);
    EIOEventType KeventToEventType(const struct kevent& Event);
};
#endif

/**
 * @brief 事件循环工厂
 */
class LIBNLIB_API NEventLoopFactory
{
public:
    // 创建默认的事件循环（根据平台选择最优实现）
    static TSharedPtr<NEventLoop> CreateDefaultEventLoop();
    
    // 创建指定模式的事件循环
    static TSharedPtr<NEventLoop> CreateEventLoop(EEventLoopMode Mode);
    
    // 平台能力查询
    static bool SupportsReactor();
    static bool SupportsProactor();
    static EEventLoopMode GetRecommendedMode();
    
private:
    NEventLoopFactory() = delete;
};

/**
 * @brief 事件循环管理器 - 管理多个事件循环
 */
class LIBNLIB_API NEventLoopManager : public CObject
{
    NCLASS()
class NEventLoopManager : public NObject
{
    GENERATED_BODY()

public:
    NEventLoopManager();
    virtual ~NEventLoopManager();
    
    // 事件循环管理
    bool AddEventLoop(const CString& Name, TSharedPtr<NEventLoop> EventLoop);
    bool RemoveEventLoop(const CString& Name);
    TSharedPtr<NEventLoop> GetEventLoop(const CString& Name);
    TSharedPtr<NEventLoop> GetDefaultEventLoop();
    
    // 批量操作
    void StartAll();
    void StopAll();
    void ShutdownAll();
    
    // 线程分配
    void RunEventLoopInThread(const CString& Name);
    void RunAllInThreads();
    
    // 统计信息
    void PrintStatistics() const;
    
private:
    CHashMap<CString, TSharedPtr<NEventLoop>> EventLoops;
    CHashMap<CString, TSharedPtr<CThread>> EventLoopThreads;
    mutable NMutex ManagerMutex;
};

} // namespace NLib
