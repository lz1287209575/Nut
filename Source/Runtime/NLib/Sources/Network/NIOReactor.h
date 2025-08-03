#pragma once

#include "Network/NEventLoop.h"
#include "Network/NSocket.h"
#include "Core/CObject.h"
#include "Threading/CThread.h"

namespace NLib
{

/**
 * @brief Reactor模式实现 - 基于事件驱动的同步I/O模型
 * 
 * Reactor模式的核心思想：
 * 1. 应用程序注册感兴趣的事件和事件处理器
 * 2. Reactor等待事件发生
 * 3. 当事件发生时，Reactor分发事件给相应的处理器
 * 4. 事件处理器处理事件（通常是同步I/O操作）
 */
NCLASS()
class LIBNLIB_API NIOReactor : public CObject
{
    GENERATED_BODY()
public:
    NIOReactor();
    explicit NIOReactor(TSharedPtr<NEventLoop> InEventLoop);
    virtual ~NIOReactor();
    
    // 初始化和清理
    bool Initialize();
    void Shutdown();
    bool IsInitialized() const { return bInitialized; }
    
    // 事件循环控制
    void Run();                                     // 阻塞运行
    void RunInBackground();                         // 在后台线程运行
    void Stop();                                    // 停止运行
    bool IsRunning() const;
    
    // 事件处理器注册 (Reactor核心功能)
    bool RegisterHandler(SocketHandle Handle, EIOEventType EventMask, 
                        IIOEventHandler* Handler, void* UserData = nullptr);
    bool UnregisterHandler(SocketHandle Handle);
    bool ModifyHandler(SocketHandle Handle, EIOEventType EventMask);
    
    // 便捷的Lambda处理器注册
    bool RegisterReadHandler(SocketHandle Handle, 
                           const NFunction<void(SocketHandle, void*)>& Handler, 
                           void* UserData = nullptr);
    bool RegisterWriteHandler(SocketHandle Handle, 
                            const NFunction<void(SocketHandle, void*)>& Handler, 
                            void* UserData = nullptr);
    bool RegisterAcceptHandler(SocketHandle Handle, 
                             const NFunction<void(SocketHandle, void*)>& Handler, 
                             void* UserData = nullptr);
    bool RegisterConnectHandler(SocketHandle Handle, 
                              const NFunction<void(SocketHandle, bool, void*)>& Handler, 
                              void* UserData = nullptr);
    bool RegisterErrorHandler(SocketHandle Handle, 
                            const NFunction<void(SocketHandle, int32_t, void*)>& Handler, 
                            void* UserData = nullptr);
    
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
    void SetMaxEvents(int32_t MaxEvents) { MaxEventsPerLoop = MaxEvents; }
    int32_t GetMaxEvents() const { return MaxEventsPerLoop; }
    
protected:
    void RunLoop();
    
private:
    TSharedPtr<NEventLoop> EventLoop;
    TSharedPtr<CThread> BackgroundThread;
    CHashMap<SocketHandle, TSharedPtr<NLambdaIOEventHandler>> LambdaHandlers;
    
    bool bInitialized;
    int32_t MaxEventsPerLoop;
    mutable NMutex ReactorMutex;
};

/**
 * @brief TCP服务器Reactor - 专门处理TCP服务器的Reactor实现
 */
NCLASS()
class LIBNLIB_API NTcpServerReactor : public NIOReactor
{
    GENERATED_BODY()

public:
    // 连接事件回调类型
    using OnClientConnectedCallback = NFunction<void(TSharedPtr<NTcpSocket>)>;
    using OnClientDisconnectedCallback = NFunction<void(TSharedPtr<NTcpSocket>)>;
    using OnDataReceivedCallback = NFunction<void(TSharedPtr<NTcpSocket>, const void*, int32_t)>;
    using OnErrorCallback = NFunction<void(TSharedPtr<NTcpSocket>, int32_t)>;
    
    NTcpServerReactor();
    virtual ~NTcpServerReactor();
    
    // 服务器控制
    bool StartServer(const NSocketAddress& BindAddress, int32_t Backlog = 128);
    void StopServer();
    bool IsServerRunning() const { return ServerSocket && ServerSocket->GetState() == ESocketState::Listening; }
    
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
    
    // 数据发送
    bool SendToClient(TSharedPtr<NTcpSocket> Client, const void* Data, int32_t Size);
    bool SendToAllClients(const void* Data, int32_t Size);
    bool BroadcastExcept(TSharedPtr<NTcpSocket> ExceptClient, const void* Data, int32_t Size);
    
    // 获取服务器套接字
    TSharedPtr<NTcpSocket> GetServerSocket() const { return ServerSocket; }
    
private:
    TSharedPtr<NTcpSocket> ServerSocket;
    CHashMap<SocketHandle, TSharedPtr<NTcpSocket>> ConnectedClients;
    mutable NMutex ClientsMutex;
    
    // 事件回调
    OnClientConnectedCallback OnClientConnected;
    OnClientDisconnectedCallback OnClientDisconnected;
    OnDataReceivedCallback OnDataReceived;
    OnErrorCallback OnError;
    
    // 内部事件处理
    void HandleAccept(SocketHandle Handle, void* UserData);
    void HandleClientRead(SocketHandle Handle, void* UserData);
    void HandleClientError(SocketHandle Handle, int32_t ErrorCode, void* UserData);
    void HandleClientClose(SocketHandle Handle, void* UserData);
    
    void AddClient(TSharedPtr<NTcpSocket> Client);
    void RemoveClient(SocketHandle Handle);
};

/**
 * @brief TCP客户端Reactor - 专门处理TCP客户端的Reactor实现
 */
NCLASS()
class LIBNLIB_API NTcpClientReactor : public NIOReactor
{
    GENERATED_BODY()

public:
    // 客户端事件回调类型
    using OnConnectedCallback = NFunction<void()>;
    using OnDisconnectedCallback = NFunction<void()>;
    using OnDataReceivedCallback = NFunction<void(const void*, int32_t)>;
    using OnErrorCallback = NFunction<void(int32_t)>;
    
    NTcpClientReactor();
    virtual ~NTcpClientReactor();
    
    // 连接控制
    bool ConnectTo(const NSocketAddress& ServerAddress);
    void Disconnect();
    bool IsConnected() const;
    
    // 事件回调设置
    void SetOnConnected(const OnConnectedCallback& Callback) { OnConnected = Callback; }
    void SetOnDisconnected(const OnDisconnectedCallback& Callback) { OnDisconnected = Callback; }
    void SetOnDataReceived(const OnDataReceivedCallback& Callback) { OnDataReceived = Callback; }
    void SetOnError(const OnErrorCallback& Callback) { OnError = Callback; }
    
    // 数据发送
    bool SendData(const void* Data, int32_t Size);
    bool SendString(const CString& Message);
    
    // 获取客户端套接字
    TSharedPtr<NTcpSocket> GetClientSocket() const { return ClientSocket; }
    
private:
    TSharedPtr<NTcpSocket> ClientSocket;
    
    // 事件回调
    OnConnectedCallback OnConnected;
    OnDisconnectedCallback OnDisconnected;
    OnDataReceivedCallback OnDataReceived;
    OnErrorCallback OnError;
    
    // 内部事件处理
    void HandleConnect(SocketHandle Handle, bool bSuccess, void* UserData);
    void HandleRead(SocketHandle Handle, void* UserData);
    void HandleWrite(SocketHandle Handle, void* UserData);
    void HandleError(SocketHandle Handle, int32_t ErrorCode, void* UserData);
    void HandleClose(SocketHandle Handle, void* UserData);
};

/**
 * @brief UDP Reactor - 处理UDP通信的Reactor实现
 */
class LIBNLIB_API NUdpReactor : public NIOReactor
{
    NCLASS()
class NUdpReactor : public NIOReactor
{
    GENERATED_BODY()

public:
    // UDP事件回调类型
    using OnDataReceivedCallback = NFunction<void(const void*, int32_t, const NSocketAddress&)>;
    using OnErrorCallback = NFunction<void(int32_t)>;
    
    NUdpReactor();
    virtual ~NUdpReactor();
    
    // UDP套接字控制
    bool BindTo(const NSocketAddress& BindAddress);
    void Close();
    bool IsBound() const;
    
    // 事件回调设置
    void SetOnDataReceived(const OnDataReceivedCallback& Callback) { OnDataReceived = Callback; }
    void SetOnError(const OnErrorCallback& Callback) { OnError = Callback; }
    
    // 数据发送
    bool SendTo(const void* Data, int32_t Size, const NSocketAddress& Address);
    bool SendString(const CString& Message, const NSocketAddress& Address);
    
    // 广播和组播
    bool EnableBroadcast(bool bEnable = true);
    bool SendBroadcast(const void* Data, int32_t Size, uint16_t Port);
    bool JoinMulticastGroup(const CString& GroupAddress);
    bool LeaveMulticastGroup(const CString& GroupAddress);
    
    // 获取UDP套接字
    TSharedPtr<NUdpSocket> GetUdpSocket() const { return UdpSocket; }
    
private:
    TSharedPtr<NUdpSocket> UdpSocket;
    
    // 事件回调
    OnDataReceivedCallback OnDataReceived;
    OnErrorCallback OnError;
    
    // 内部事件处理
    void HandleRead(SocketHandle Handle, void* UserData);
    void HandleError(SocketHandle Handle, int32_t ErrorCode, void* UserData);
};

/**
 * @brief Reactor工厂 - 创建不同类型的Reactor
 */
class LIBNLIB_API NReactorFactory
{
public:
    // 创建通用Reactor
    static TSharedPtr<NIOReactor> CreateReactor();
    static TSharedPtr<NIOReactor> CreateReactor(EEventLoopMode Mode);
    
    // 创建专用Reactor
    static TSharedPtr<NTcpServerReactor> CreateTcpServerReactor();
    static TSharedPtr<NTcpClientReactor> CreateTcpClientReactor();
    static TSharedPtr<NUdpReactor> CreateUdpReactor();
    
private:
    NReactorFactory() = delete;
};

/**
 * @brief Reactor配置选项
 */
struct LIBNLIB_API NReactorConfig
{
    EEventLoopMode EventLoopMode;       // 事件循环模式
    int32_t MaxEventsPerLoop;           // 每次循环最大事件数
    int32_t MaxConnections;             // 最大连接数
    int32_t ReceiveBufferSize;          // 接收缓冲区大小
    int32_t SendBufferSize;             // 发送缓冲区大小
    bool bReuseAddress;                 // 重用地址
    bool bReusePort;                    // 重用端口
    bool bKeepAlive;                    // 保持连接
    bool bNoDelay;                      // 禁用Nagle算法
    
    NReactorConfig();
    static NReactorConfig GetDefault();
    static NReactorConfig GetHighPerformance();
    static NReactorConfig GetLowLatency();
};

} // namespace NLib
