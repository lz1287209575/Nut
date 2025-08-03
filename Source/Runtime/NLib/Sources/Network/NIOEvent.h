#pragma once

#include "Core/CObject.h"
#include "Network/NSocket.h"
#include "Containers/CArray.h"
#include "Threading/CThread.h"
#include "Delegates/CDelegate.h"

namespace NLib
{

/**
 * @brief IO事件类型
 */
enum class EIOEventType : uint32_t
{
    None = 0,
    Read = 1 << 0,      // 可读事件
    Write = 1 << 1,     // 可写事件
    Error = 1 << 2,     // 错误事件
    Close = 1 << 3,     // 连接关闭
    Accept = 1 << 4,    // 新连接到达
    Connect = 1 << 5,   // 连接完成
    All = Read | Write | Error | Close | Accept | Connect
};

ENUM_CLASS_FLAGS(EIOEventType)

/**
 * @brief IO操作类型 (用于Proactor模式)
 */
enum class EIOOperationType : uint32_t
{
    None,
    Accept,         // 接受连接
    Connect,        // 建立连接
    Send,           // 发送数据
    Receive,        // 接收数据
    SendTo,         // UDP发送
    ReceiveFrom     // UDP接收
};

/**
 * @brief IO事件数据
 */
struct LIBNLIB_API NIOEventData
{
    EIOEventType EventType;         // 事件类型
    EIOOperationType OpType;        // 操作类型(Proactor)
    SocketHandle Handle;            // 套接字句柄
    void* UserData;                 // 用户数据
    int32_t BytesTransferred;       // 传输字节数(Proactor)
    int32_t ErrorCode;              // 错误码
    
    NIOEventData();
    NIOEventData(EIOEventType InEventType, SocketHandle InHandle, void* InUserData = nullptr);
};

/**
 * @brief IO事件处理器接口
 */
class LIBNLIB_API IIOEventHandler
{
public:
    virtual ~IIOEventHandler() = default;
    
    // Reactor模式回调
    virtual void OnReadable(SocketHandle Handle, void* UserData) {}
    virtual void OnWritable(SocketHandle Handle, void* UserData) {}
    virtual void OnError(SocketHandle Handle, int32_t ErrorCode, void* UserData) {}
    virtual void OnClose(SocketHandle Handle, void* UserData) {}
    virtual void OnAccept(SocketHandle Handle, void* UserData) {}
    virtual void OnConnect(SocketHandle Handle, bool bSuccess, void* UserData) {}
    
    // Proactor模式回调
    virtual void OnOperationComplete(const NIOEventData& EventData) {}
};

/**
 * @brief IO事件 - 封装单个IO事件
 */
class LIBNLIB_API NIOEvent : public CObject
{
    NCLASS()
class NIOEvent : public NObject
{
    GENERATED_BODY()

public:
    NIOEvent();
    NIOEvent(EIOEventType InEventType, SocketHandle InHandle, void* InUserData = nullptr);
    virtual ~NIOEvent();
    
    // 事件属性
    EIOEventType GetEventType() const { return EventData.EventType; }
    EIOOperationType GetOperationType() const { return EventData.OpType; }
    SocketHandle GetHandle() const { return EventData.Handle; }
    void* GetUserData() const { return EventData.UserData; }
    int32_t GetBytesTransferred() const { return EventData.BytesTransferred; }
    int32_t GetErrorCode() const { return EventData.ErrorCode; }
    
    void SetEventType(EIOEventType Type) { EventData.EventType = Type; }
    void SetOperationType(EIOOperationType Type) { EventData.OpType = Type; }
    void SetHandle(SocketHandle Handle) { EventData.Handle = Handle; }
    void SetUserData(void* Data) { EventData.UserData = Data; }
    void SetBytesTransferred(int32_t Bytes) { EventData.BytesTransferred = Bytes; }
    void SetErrorCode(int32_t Code) { EventData.ErrorCode = Code; }
    
    const NIOEventData& GetEventData() const { return EventData; }
    
    // 事件检查
    bool IsReadEvent() const { return (EventData.EventType & EIOEventType::Read) != EIOEventType::None; }
    bool IsWriteEvent() const { return (EventData.EventType & EIOEventType::Write) != EIOEventType::None; }
    bool IsErrorEvent() const { return (EventData.EventType & EIOEventType::Error) != EIOEventType::None; }
    bool IsCloseEvent() const { return (EventData.EventType & EIOEventType::Close) != EIOEventType::None; }
    bool IsAcceptEvent() const { return (EventData.EventType & EIOEventType::Accept) != EIOEventType::None; }
    bool IsConnectEvent() const { return (EventData.EventType & EIOEventType::Connect) != EIOEventType::None; }
    
protected:
    NIOEventData EventData;
};

/**
 * @brief IO完成数据 (用于Proactor模式)
 */
struct LIBNLIB_API NIOCompletion
{
    EIOOperationType OpType;        // 操作类型
    SocketHandle Handle;            // 套接字句柄
    void* Buffer;                   // 数据缓冲区
    int32_t BufferSize;             // 缓冲区大小
    int32_t BytesTransferred;       // 实际传输字节数
    int32_t ErrorCode;              // 错误码
    void* UserData;                 // 用户数据
    NSocketAddress RemoteAddress;   // 远程地址(UDP)
    
    NIOCompletion();
    NIOCompletion(EIOOperationType InOpType, SocketHandle InHandle, void* InBuffer, int32_t InBufferSize);
};

/**
 * @brief 异步IO操作句柄
 */
NCLASS()
class LIBNLIB_API NAsyncIOHandle : public CObject
{
    GENERATED_BODY()

public:
    NAsyncIOHandle();
    explicit NAsyncIOHandle(const NIOCompletion& InCompletion);
    virtual ~NAsyncIOHandle();
    
    // 操作属性
    EIOOperationType GetOperationType() const { return Completion.OpType; }
    SocketHandle GetHandle() const { return Completion.Handle; }
    void* GetBuffer() const { return Completion.Buffer; }
    int32_t GetBufferSize() const { return Completion.BufferSize; }
    void* GetUserData() const { return Completion.UserData; }
    
    void SetOperationType(EIOOperationType Type) { Completion.OpType = Type; }
    void SetHandle(SocketHandle Handle) { Completion.Handle = Handle; }
    void SetBuffer(void* Buffer, int32_t Size) { Completion.Buffer = Buffer; Completion.BufferSize = Size; }
    void SetUserData(void* Data) { Completion.UserData = Data; }
    void SetRemoteAddress(const NSocketAddress& Address) { Completion.RemoteAddress = Address; }
    
    const NIOCompletion& GetCompletion() const { return Completion; }
    NIOCompletion& GetCompletion() { return Completion; }
    
    // 操作状态
    bool IsCompleted() const { return bCompleted; }
    bool IsCancelled() const { return bCancelled; }
    bool HasError() const { return Completion.ErrorCode != 0; }
    
    void SetCompleted(int32_t BytesTransferred, int32_t ErrorCode = 0);
    void Cancel();
    
protected:
    NIOCompletion Completion;
    bool bCompleted;
    bool bCancelled;
};

/**
 * @brief IO事件批处理器 - 用于批量处理事件
 */
NCLASS()
class LIBNLIB_API NIOEventBatch : public CObject
{
    GENERATED_BODY()

public:
    NIOEventBatch();
    explicit NIOEventBatch(int32_t ReserveSize);
    virtual ~NIOEventBatch();
    
    // 事件管理
    void AddEvent(TSharedPtr<NIOEvent> Event);
    void AddEvent(const NIOEventData& EventData);
    void Clear();
    bool IsEmpty() const;
    int32_t GetEventCount() const;
    
    // 事件访问
    TSharedPtr<NIOEvent> GetEvent(int32_t Index) const;
    const CArray<TSharedPtr<NIOEvent>>& GetEvents() const { return Events; }
    
    // 事件处理
    void ProcessEvents(IIOEventHandler* Handler);
    
    // 事件过滤
    void FilterEvents(EIOEventType TypeMask);
    CArray<TSharedPtr<NIOEvent>> GetEventsByType(EIOEventType Type) const;
    
private:
    CArray<TSharedPtr<NIOEvent>> Events;
};

/**
 * @brief Lambda事件处理器 - 便于使用Lambda表达式
 */
class LIBNLIB_API NLambdaIOEventHandler : public IIOEventHandler
{
public:
    using ReadableCallback = NFunction<void(SocketHandle, void*)>;
    using WritableCallback = NFunction<void(SocketHandle, void*)>;
    using ErrorCallback = NFunction<void(SocketHandle, int32_t, void*)>;
    using CloseCallback = NFunction<void(SocketHandle, void*)>;
    using AcceptCallback = NFunction<void(SocketHandle, void*)>;
    using ConnectCallback = NFunction<void(SocketHandle, bool, void*)>;
    using CompletionCallback = NFunction<void(const NIOEventData&)>;
    
    NLambdaIOEventHandler() = default;
    virtual ~NLambdaIOEventHandler() = default;
    
    // 设置回调函数
    void SetReadableCallback(const ReadableCallback& Callback) { OnReadableCallback = Callback; }
    void SetWritableCallback(const WritableCallback& Callback) { OnWritableCallback = Callback; }
    void SetErrorCallback(const ErrorCallback& Callback) { OnErrorCallback = Callback; }
    void SetCloseCallback(const CloseCallback& Callback) { OnCloseCallback = Callback; }
    void SetAcceptCallback(const AcceptCallback& Callback) { OnAcceptCallback = Callback; }
    void SetConnectCallback(const ConnectCallback& Callback) { OnConnectCallback = Callback; }
    void SetCompletionCallback(const CompletionCallback& Callback) { OnCompletionCallback = Callback; }
    
    // IIOEventHandler接口实现
    virtual void OnReadable(SocketHandle Handle, void* UserData) override;
    virtual void OnWritable(SocketHandle Handle, void* UserData) override;
    virtual void OnError(SocketHandle Handle, int32_t ErrorCode, void* UserData) override;
    virtual void OnClose(SocketHandle Handle, void* UserData) override;
    virtual void OnAccept(SocketHandle Handle, void* UserData) override;
    virtual void OnConnect(SocketHandle Handle, bool bSuccess, void* UserData) override;
    virtual void OnOperationComplete(const NIOEventData& EventData) override;
    
private:
    ReadableCallback OnReadableCallback;
    WritableCallback OnWritableCallback;
    ErrorCallback OnErrorCallback;
    CloseCallback OnCloseCallback;
    AcceptCallback OnAcceptCallback;
    ConnectCallback OnConnectCallback;
    CompletionCallback OnCompletionCallback;
};

// 事件类型操作符重载
inline EIOEventType operator|(EIOEventType A, EIOEventType B)
{
    return static_cast<EIOEventType>(static_cast<uint32_t>(A) | static_cast<uint32_t>(B));
}

inline EIOEventType operator&(EIOEventType A, EIOEventType B)
{
    return static_cast<EIOEventType>(static_cast<uint32_t>(A) & static_cast<uint32_t>(B));
}

inline EIOEventType operator~(EIOEventType A)
{
    return static_cast<EIOEventType>(~static_cast<uint32_t>(A));
}

inline EIOEventType& operator|=(EIOEventType& A, EIOEventType B)
{
    A = A | B;
    return A;
}

inline EIOEventType& operator&=(EIOEventType& A, EIOEventType B)
{
    A = A & B;
    return A;
}

} // namespace NLib
