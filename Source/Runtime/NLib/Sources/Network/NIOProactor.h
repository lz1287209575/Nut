#pragma once

#include "Network/NEventLoop.h"
#include "Network/NIOEvent.h"
#include "Network/NSocket.h"
#include "Core/CObject.h"
#include "Threading/CThread.h"
#include "Containers/CArray.h"

namespace NLib
{

/**
 * @brief Proactor模式实现 - 基于异步I/O的模型
 * 
 * Proactor模式的核心思想：
 * 1. 应用程序发起异步I/O操作
 * 2. 操作系统执行实际的I/O操作
 * 3. 当I/O操作完成时，操作系统通知Proactor
 * 4. Proactor分发完成事件给相应的完成处理器
 * 5. 完成处理器处理I/O操作的结果
 */
NCLASS()
class LIBNLIB_API NIOProactor : public CObject
{
    GENERATED_BODY()

public:
    // 异步操作完成回调类型
    using AsyncAcceptCallback = NFunction<void(bool, TSharedPtr<NTcpSocket>, int32_t)>;
    using AsyncConnectCallback = NFunction<void(bool, int32_t)>;
    using AsyncSendCallback = NFunction<void(bool, int32_t, int32_t)>;           // success, bytes_sent, error_code
    using AsyncReceiveCallback = NFunction<void(bool, int32_t, int32_t)>;       // success, bytes_received, error_code
    using AsyncSendToCallback = NFunction<void(bool, int32_t, int32_t)>;        // success, bytes_sent, error_code
    using AsyncReceiveFromCallback = NFunction<void(bool, int32_t, const NSocketAddress&, int32_t)>; // success, bytes_received, from_address, error_code
    
    NIOProactor();
    explicit NIOProactor(TSharedPtr<NEventLoop> InEventLoop);
    virtual ~NIOProactor();
    
    // 初始化和清理
    bool Initialize();
    void Shutdown();
    bool IsInitialized() const { return bInitialized; }
    
    // 事件循环控制
    void Run();                                     // 阻塞运行
    void RunInBackground();                         // 在后台线程运行
    void Stop();                                    // 停止运行
    bool IsRunning() const;
    
    // 异步接受连接 (TCP Server)
    TSharedPtr<NAsyncIOHandle> AsyncAccept(TSharedPtr<NTcpSocket> ListenSocket,
                                          const AsyncAcceptCallback& Callback);
    
    // 异步连接 (TCP Client)
    TSharedPtr<NAsyncIOHandle> AsyncConnect(TSharedPtr<NTcpSocket> ClientSocket,
                                           const NSocketAddress& ServerAddress,
                                           const AsyncConnectCallback& Callback);
    
    // 异步发送数据 (TCP)
    TSharedPtr<NAsyncIOHandle> AsyncSend(TSharedPtr<NTcpSocket> Socket,
                                        const void* Data, int32_t Size,
                                        const AsyncSendCallback& Callback);
    
    // 异步接收数据 (TCP)
    TSharedPtr<NAsyncIOHandle> AsyncReceive(TSharedPtr<NTcpSocket> Socket,
                                           void* Buffer, int32_t BufferSize,
                                           const AsyncReceiveCallback& Callback);
    
    // 异步发送数据 (UDP)
    TSharedPtr<NAsyncIOHandle> AsyncSendTo(TSharedPtr<NUdpSocket> Socket,
                                          const void* Data, int32_t Size,
                                          const NSocketAddress& Address,
                                          const AsyncSendToCallback& Callback);
    
    // 异步接收数据 (UDP)
    TSharedPtr<NAsyncIOHandle> AsyncReceiveFrom(TSharedPtr<NUdpSocket> Socket,
                                               void* Buffer, int32_t BufferSize,
                                               const AsyncReceiveFromCallback& Callback);
    
    // 取消异步操作
    bool CancelOperation(TSharedPtr<NAsyncIOHandle> Handle);
    bool CancelAllOperations(SocketHandle Socket);
    
    // 定时器支持
    uint64_t RegisterTimer(int32_t DelayMs, const NFunction<void()>& Callback);
    uint64_t RegisterRepeatingTimer(int32_t IntervalMs, const NFunction<void()>& Callback);
    bool UnregisterTimer(uint64_t TimerId);
    
    // 任务投递
    void PostTask(const NFunction<void()>& Task);
    void PostDelayedTask(int32_t DelayMs, const NFunction<void()>& Task);
    
    // 获取底层事件循环
    TSharedPtr<NEventLoop> GetEventLoop() const { return EventLoop; }
    
    // 统计信息
    const NEventLoop::Statistics& GetStatistics() const;
    void ResetStatistics();
    
    // 配置选项
    void SetMaxConcurrentOperations(int32_t MaxOps) { MaxConcurrentOps = MaxOps; }
    int32_t GetMaxConcurrentOperations() const { return MaxConcurrentOps; }
    
    void SetDefaultBufferSize(int32_t BufferSize) { DefaultBufferSize = BufferSize; }
    int32_t GetDefaultBufferSize() const { return DefaultBufferSize; }
    
protected:
    void RunLoop();
    
    // 内部方法
    virtual TSharedPtr<NAsyncIOHandle> CreateAsyncHandle(EIOOperationType OpType, SocketHandle Socket);
    virtual bool SubmitOperation(TSharedPtr<NAsyncIOHandle> Handle) = 0;
    
private:
    TSharedPtr<NEventLoop> EventLoop;
    TSharedPtr<CThread> BackgroundThread;
    
    bool bInitialized;
    int32_t MaxConcurrentOps;
    int32_t DefaultBufferSize;
    
    // 异步操作管理
    CHashMap<uint64_t, TSharedPtr<NAsyncIOHandle>> PendingOperations;
    CAtomic<uint64_t> NextOperationId;
    mutable NMutex OperationsMutex;
    
    mutable NMutex ProactorMutex;
};

#ifdef PLATFORM_WINDOWS
/**
 * @brief Windows IOCP Proactor实现
 */
NCLASS()
class LIBNLIB_API NIOCPProactor : public NIOProactor
{
    GENERATED_BODY()

public:
    NIOCPProactor();
    virtual ~NIOCPProactor();
    
    // 初始化IOCP
    virtual bool Initialize() override;
    virtual void Shutdown() override;
    
protected:
    virtual bool SubmitOperation(TSharedPtr<NAsyncIOHandle> Handle) override;
    
private:
    HANDLE IOCPHandle;
    
    // IOCP特定的操作处理
    bool SubmitAccept(TSharedPtr<NAsyncIOHandle> Handle);
    bool SubmitConnect(TSharedPtr<NAsyncIOHandle> Handle);
    bool SubmitSend(TSharedPtr<NAsyncIOHandle> Handle);
    bool SubmitReceive(TSharedPtr<NAsyncIOHandle> Handle);
    bool SubmitSendTo(TSharedPtr<NAsyncIOHandle> Handle);
    bool SubmitReceiveFrom(TSharedPtr<NAsyncIOHandle> Handle);
    
    // 完成处理
    void ProcessCompletion(DWORD BytesTransferred, ULONG_PTR CompletionKey, OVERLAPPED* Overlapped);
};

#elif defined(PLATFORM_LINUX)
/**
 * @brief Linux io_uring Proactor实现 (如果支持)
 */
NCLASS()
class LIBNLIB_API NIOUringProactor : public NIOProactor
{
    GENERATED_BODY()

public:
    NIOUringProactor();
    virtual ~NIOUringProactor();
    
    virtual bool Initialize() override;
    virtual void Shutdown() override;
    
    // 检查io_uring支持
    static bool IsSupported();
    
protected:
    virtual bool SubmitOperation(TSharedPtr<NAsyncIOHandle> Handle) override;
    
private:
    // io_uring相关成员 (需要liburing支持)
    void* IoUringPtr;       // struct io_uring*
    
    bool SubmitAccept(TSharedPtr<NAsyncIOHandle> Handle);
    bool SubmitConnect(TSharedPtr<NAsyncIOHandle> Handle);
    bool SubmitSend(TSharedPtr<NAsyncIOHandle> Handle);
    bool SubmitReceive(TSharedPtr<NAsyncIOHandle> Handle);
    bool SubmitSendTo(TSharedPtr<NAsyncIOHandle> Handle);
    bool SubmitReceiveFrom(TSharedPtr<NAsyncIOHandle> Handle);
};

/**
 * @brief Linux AIO Proactor实现 (备用方案)
 */
NCLASS()
class LIBNLIB_API NAIOProactor : public NIOProactor
{
    GENERATED_BODY()

public:
    NAIOProactor();
    virtual ~NAIOProactor();
    
    virtual bool Initialize() override;
    virtual void Shutdown() override;
    
protected:
    virtual bool SubmitOperation(TSharedPtr<NAsyncIOHandle> Handle) override;
    
private:
    // Linux AIO相关成员
    void* AIOContextPtr;    // aio_context_t
    
    bool SubmitAIOOperation(TSharedPtr<NAsyncIOHandle> Handle);
};

#endif

/**
 * @brief 模拟Proactor - 在不支持真正异步I/O的平台上使用线程池模拟
 */
NCLASS()
class LIBNLIB_API NSimulatedProactor : public NIOProactor
{
    GENERATED_BODY()

public:
    NSimulatedProactor();
    explicit NSimulatedProactor(int32_t ThreadPoolSize);
    virtual ~NSimulatedProactor();
    
    virtual bool Initialize() override;
    virtual void Shutdown() override;
    
    // 线程池配置
    void SetThreadPoolSize(int32_t Size);
    int32_t GetThreadPoolSize() const { return ThreadPoolSize; }
    
protected:
    virtual bool SubmitOperation(TSharedPtr<NAsyncIOHandle> Handle) override;
    
private:
    int32_t ThreadPoolSize;
    CArray<TSharedPtr<CThread>> WorkerThreads;
    CArray<TSharedPtr<NAsyncIOHandle>> OperationQueue;
    mutable NMutex QueueMutex;
    NConditionVariable QueueCondition;
    bool bShuttingDown;
    
    // 工作线程函数
    void WorkerThreadFunction();
    
    // 操作执行
    void ExecuteOperation(TSharedPtr<NAsyncIOHandle> Handle);
    void ExecuteAccept(TSharedPtr<NAsyncIOHandle> Handle);
    void ExecuteConnect(TSharedPtr<NAsyncIOHandle> Handle);
    void ExecuteSend(TSharedPtr<NAsyncIOHandle> Handle);
    void ExecuteReceive(TSharedPtr<NAsyncIOHandle> Handle);
    void ExecuteSendTo(TSharedPtr<NAsyncIOHandle> Handle);
    void ExecuteReceiveFrom(TSharedPtr<NAsyncIOHandle> Handle);
};

/**
 * @brief TCP服务器Proactor - 专门处理TCP服务器的Proactor实现
 */
NCLASS()
class LIBNLIB_API NTcpServerProactor : public CObject
{
    GENERATED_BODY()

public:
    // 服务器事件回调类型
    using OnClientConnectedCallback = NFunction<void(TSharedPtr<NTcpSocket>)>;
    using OnClientDisconnectedCallback = NFunction<void(TSharedPtr<NTcpSocket>)>;
    using OnDataReceivedCallback = NFunction<void(TSharedPtr<NTcpSocket>, const void*, int32_t)>;
    using OnErrorCallback = NFunction<void(TSharedPtr<NTcpSocket>, int32_t)>;
    
    NTcpServerProactor();
    explicit NTcpServerProactor(TSharedPtr<NIOProactor> InProactor);
    virtual ~NTcpServerProactor();
    
    // 服务器控制
    bool StartServer(const NSocketAddress& BindAddress, int32_t Backlog = 128);
    void StopServer();
    bool IsServerRunning() const;
    
    // 事件回调设置
    void SetOnClientConnected(const OnClientConnectedCallback& Callback) { OnClientConnected = Callback; }
    void SetOnClientDisconnected(const OnClientDisconnectedCallback& Callback) { OnClientDisconnected = Callback; }
    void SetOnDataReceived(const OnDataReceivedCallback& Callback) { OnDataReceived = Callback; }
    void SetOnError(const OnErrorCallback& Callback) { OnError = Callback; }
    
    // 客户端管理
    void DisconnectClient(TSharedPtr<NTcpSocket> Client);
    void DisconnectAllClients();
    int32_t GetClientCount() const;
    CArray<TSharedPtr<NTcpSocket>> GetConnectedClients() const;
    
    // 异步数据发送
    bool SendToClient(TSharedPtr<NTcpSocket> Client, const void* Data, int32_t Size);
    bool SendToAllClients(const void* Data, int32_t Size);
    bool BroadcastExcept(TSharedPtr<NTcpSocket> ExceptClient, const void* Data, int32_t Size);
    
    // 获取底层Proactor
    TSharedPtr<NIOProactor> GetProactor() const { return Proactor; }
    
private:
    TSharedPtr<NIOProactor> Proactor;
    TSharedPtr<NTcpSocket> ServerSocket;
    CHashMap<SocketHandle, TSharedPtr<NTcpSocket>> ConnectedClients;
    mutable NMutex ClientsMutex;
    
    // 事件回调
    OnClientConnectedCallback OnClientConnected;
    OnClientDisconnectedCallback OnClientDisconnected;
    OnDataReceivedCallback OnDataReceived;
    OnErrorCallback OnError;
    
    // 内部处理
    void StartAccept();
    void HandleAcceptComplete(bool bSuccess, TSharedPtr<NTcpSocket> ClientSocket, int32_t ErrorCode);
    void StartReceive(TSharedPtr<NTcpSocket> ClientSocket);
    void HandleReceiveComplete(TSharedPtr<NTcpSocket> ClientSocket, bool bSuccess, int32_t BytesReceived, int32_t ErrorCode);
    void HandleSendComplete(TSharedPtr<NTcpSocket> ClientSocket, bool bSuccess, int32_t BytesSent, int32_t ErrorCode);
    
    void AddClient(TSharedPtr<NTcpSocket> Client);
    void RemoveClient(SocketHandle Handle);
};

/**
 * @brief TCP客户端Proactor - 专门处理TCP客户端的Proactor实现
 */
NCLASS()
class LIBNLIB_API NTcpClientProactor : public CObject
{
    GENERATED_BODY()

public:
    // 客户端事件回调类型
    using OnConnectedCallback = NFunction<void()>;
    using OnDisconnectedCallback = NFunction<void()>;
    using OnDataReceivedCallback = NFunction<void(const void*, int32_t)>;
    using OnErrorCallback = NFunction<void(int32_t)>;
    
    NTcpClientProactor();
    explicit NTcpClientProactor(TSharedPtr<NIOProactor> InProactor);
    virtual ~NTcpClientProactor();
    
    // 连接控制
    bool ConnectTo(const NSocketAddress& ServerAddress);
    void Disconnect();
    bool IsConnected() const;
    
    // 事件回调设置
    void SetOnConnected(const OnConnectedCallback& Callback) { OnConnected = Callback; }
    void SetOnDisconnected(const OnDisconnectedCallback& Callback) { OnDisconnected = Callback; }
    void SetOnDataReceived(const OnDataReceivedCallback& Callback) { OnDataReceived = Callback; }
    void SetOnError(const OnErrorCallback& Callback) { OnError = Callback; }
    
    // 异步数据发送
    bool SendData(const void* Data, int32_t Size);
    bool SendString(const CString& Message);
    
    // 获取底层Proactor
    TSharedPtr<NIOProactor> GetProactor() const { return Proactor; }
    
private:
    TSharedPtr<NIOProactor> Proactor;
    TSharedPtr<NTcpSocket> ClientSocket;
    
    // 事件回调
    OnConnectedCallback OnConnected;
    OnDisconnectedCallback OnDisconnected;
    OnDataReceivedCallback OnDataReceived;
    OnErrorCallback OnError;
    
    // 内部处理
    void HandleConnectComplete(bool bSuccess, int32_t ErrorCode);
    void StartReceive();
    void HandleReceiveComplete(bool bSuccess, int32_t BytesReceived, int32_t ErrorCode);
    void HandleSendComplete(bool bSuccess, int32_t BytesSent, int32_t ErrorCode);
};

/**
 * @brief Proactor工厂 - 创建不同类型的Proactor
 */
class LIBNLIB_API NProactorFactory
{
public:
    // 创建通用Proactor (自动选择最佳实现)
    static TSharedPtr<NIOProactor> CreateProactor();
    
    // 创建特定实现的Proactor
    static TSharedPtr<NIOProactor> CreateNativeProactor();      // 平台原生实现
    static TSharedPtr<NIOProactor> CreateSimulatedProactor();   // 模拟实现
    
    // 创建专用Proactor
    static TSharedPtr<NTcpServerProactor> CreateTcpServerProactor();
    static TSharedPtr<NTcpClientProactor> CreateTcpClientProactor();
    
    // 平台能力查询
    static bool SupportsNativeProactor();
    static CString GetNativeProactorName();
    
private:
    NProactorFactory() = delete;
};

/**
 * @brief Proactor配置选项
 */
struct LIBNLIB_API NProactorConfig
{
    int32_t MaxConcurrentOperations;    // 最大并发操作数
    int32_t DefaultBufferSize;          // 默认缓冲区大小
    int32_t ThreadPoolSize;             // 线程池大小(模拟模式)
    bool bUseNativeImplementation;      // 使用原生实现
    bool bEnableStatistics;             // 启用统计
    
    NProactorConfig();
    static NProactorConfig GetDefault();
    static NProactorConfig GetHighThroughput();
    static NProactorConfig GetLowLatency();
};

} // namespace NLib
