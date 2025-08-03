#pragma once

#include "Core/CObject.h"
#include "Containers/CString.h"
#include "Threading/CThread.h"

#ifdef PLATFORM_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET SocketHandle;
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
typedef int SocketHandle;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

namespace NLib
{

/**
 * @brief 套接字类型
 */
enum class ESocketType : uint32_t
{
    TCP,            // TCP套接字
    UDP,            // UDP套接字
    Unknown         // 未知类型
};

/**
 * @brief 套接字状态
 */
enum class ESocketState : uint32_t
{
    Closed,         // 已关闭
    Connecting,     // 连接中
    Connected,      // 已连接
    Listening,      // 监听中
    Error           // 错误状态
};

/**
 * @brief 网络地址信息
 */
struct LIBNLIB_API NSocketAddress
{
    CString Host;           // 主机地址
    uint16_t Port;          // 端口号
    
    NSocketAddress();
    NSocketAddress(const CString& InHost, uint16_t InPort);
    
    // 转换为系统地址结构
    sockaddr_in ToSockAddrIn() const;
    void FromSockAddrIn(const sockaddr_in& SockAddr);
    
    // 字符串表示
    CString ToString() const;
    
    // 比较操作
    bool operator==(const NSocketAddress& Other) const;
    bool operator!=(const NSocketAddress& Other) const;
};

/**
 * @brief 基础套接字类 - 跨平台套接字封装
 */
NCLASS()
class LIBNLIB_API NSocket : public CObject
{
    GENERATED_BODY()

public:
    NSocket();
    explicit NSocket(ESocketType InType);
    NSocket(SocketHandle InHandle, ESocketType InType);
    virtual ~NSocket();
    
    // 套接字创建和销毁
    bool Create(ESocketType Type);
    void Close();
    bool IsValid() const;
    
    // 套接字属性
    ESocketType GetSocketType() const { return SocketType; }
    ESocketState GetState() const { return State; }
    SocketHandle GetHandle() const { return Handle; }
    
    // 地址信息
    const NSocketAddress& GetLocalAddress() const { return LocalAddress; }
    const NSocketAddress& GetRemoteAddress() const { return RemoteAddress; }
    
    // 套接字选项
    bool SetNonBlocking(bool bNonBlocking = true);
    bool SetReuseAddress(bool bReuse = true);
    bool SetReusePort(bool bReuse = true);
    bool SetKeepAlive(bool bKeepAlive = true);
    bool SetNoDelay(bool bNoDelay = true);  // TCP_NODELAY
    bool SetSendBufferSize(int32_t Size);
    bool SetReceiveBufferSize(int32_t Size);
    bool SetSendTimeout(int32_t TimeoutMs);
    bool SetReceiveTimeout(int32_t TimeoutMs);
    
    // 获取套接字选项
    bool IsNonBlocking() const;
    int32_t GetSendBufferSize() const;
    int32_t GetReceiveBufferSize() const;
    
    // TCP操作
    bool Bind(const NSocketAddress& Address);
    bool Listen(int32_t Backlog = 128);
    TSharedPtr<NSocket> Accept();
    bool Connect(const NSocketAddress& Address);
    
    // 数据传输
    int32_t Send(const void* Data, int32_t Size);
    int32_t Receive(void* Buffer, int32_t Size);
    int32_t SendTo(const void* Data, int32_t Size, const NSocketAddress& Address);
    int32_t ReceiveFrom(void* Buffer, int32_t Size, NSocketAddress& FromAddress);
    
    // 异步操作支持
    bool WouldBlock() const;
    bool HasPendingData() const;
    
    // 错误处理
    int32_t GetLastError() const;
    CString GetLastErrorString() const;
    static CString GetErrorString(int32_t ErrorCode);
    
    // 静态工具函数
    static bool InitializeNetworking();
    static void CleanupNetworking();
    static CString GetHostName();
    static CArray<CString> GetLocalIPs();
    static bool ResolveHostname(const CString& Hostname, CArray<CString>& OutIPs);
    
protected:
    void UpdateLocalAddress();
    void UpdateRemoteAddress();
    void SetState(ESocketState NewState);
    
private:
    SocketHandle Handle;
    ESocketType SocketType;
    ESocketState State;
    NSocketAddress LocalAddress;
    NSocketAddress RemoteAddress;
    
    mutable NMutex SocketMutex;
    
    // 禁止拷贝
    NSocket(const NSocket&) = delete;
    NSocket& operator=(const NSocket&) = delete;
};

/**
 * @brief TCP套接字 - 专门用于TCP连接
 */
NCLASS()
class LIBNLIB_API NTcpSocket : public NSocket
{
    GENERATED_BODY()

public:
    NTcpSocket();
    explicit NTcpSocket(SocketHandle InHandle);
    virtual ~NTcpSocket();
    
    // TCP特定操作
    bool ConnectAsync(const NSocketAddress& Address);
    bool IsConnectComplete();
    
    // 优雅关闭
    void Shutdown(bool bReceive = true, bool bSend = true);
    
    // 数据传输增强
    int32_t SendAll(const void* Data, int32_t Size);
    int32_t ReceiveExact(void* Buffer, int32_t Size);
    
    // 连接状态检查
    bool IsConnected() const;
    bool IsConnectionAlive() const;
};

/**
 * @brief UDP套接字 - 专门用于UDP通信
 */
NCLASS()
class LIBNLIB_API NUdpSocket : public NSocket
{
    GENERATED_BODY()

public:
    NUdpSocket();
    virtual ~NUdpSocket();
    
    // UDP特定操作
    bool EnableBroadcast(bool bEnable = true);
    bool JoinMulticastGroup(const CString& GroupAddress);
    bool LeaveMulticastGroup(const CString& GroupAddress);
    bool SetMulticastTTL(int32_t TTL);
    
    // 连接模式UDP（可选）
    bool ConnectTo(const NSocketAddress& Address);
    void Disconnect();
};

/**
 * @brief 套接字工厂 - 创建不同类型的套接字
 */
class LIBNLIB_API NSocketFactory
{
public:
    static TSharedPtr<NTcpSocket> CreateTcpSocket();
    static TSharedPtr<NUdpSocket> CreateUdpSocket();
    static TSharedPtr<NSocket> CreateSocket(ESocketType Type);
    
    // 从现有句柄创建
    static TSharedPtr<NTcpSocket> CreateTcpSocket(SocketHandle Handle);
    static TSharedPtr<NUdpSocket> CreateUdpSocket(SocketHandle Handle);
    
private:
    NSocketFactory() = delete;
};

/**
 * @brief 网络初始化器 - RAII方式管理网络库初始化
 */
class LIBNLIB_API NNetworkInitializer
{
public:
    NNetworkInitializer();
    ~NNetworkInitializer();
    
    bool IsInitialized() const { return bInitialized; }
    
private:
    bool bInitialized;
    static CAtomic<int32_t> InitCount;
};

// 便捷宏
#define ENSURE_NETWORK_INIT() \
    static NLib::NNetworkInitializer NetworkInit; \
    if (!NetworkInit.IsInitialized()) { \
        CLogger::Get().LogError("Failed to initialize network subsystem"); \
        return false; \
    }

} // namespace NLib
