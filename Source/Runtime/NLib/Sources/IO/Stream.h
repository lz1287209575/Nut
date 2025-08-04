#pragma once

#include "Containers/TArray.h"
#include "Core/Object.h"
#include "Core/SmartPointers.h"
#include "Logging/LogCategory.h"
#include "Path.h"
#include "Stream.generate.h"

#include <memory>

namespace NLib
{
/**
 * @brief 流查找起始位置
 */
enum class ESeekOrigin : uint8_t
{
	Begin,   // 从流开始位置
	Current, // 从当前位置
	End      // 从流结束位置
};

/**
 * @brief 流访问模式
 */
enum class EStreamAccess : uint8_t
{
	Read = 1,     // 只读
	Write = 2,    // 只写
	ReadWrite = 3 // 读写
};

/**
 * @brief 流打开模式
 */
enum class EStreamMode : uint8_t
{
	Create,       // 创建新文件，如果存在则截断
	CreateNew,    // 创建新文件，如果存在则失败
	Open,         // 打开存在的文件
	OpenOrCreate, // 打开文件，如果不存在则创建
	Truncate,     // 打开并截断文件
	Append        // 追加模式
};

/**
 * @brief 缓冲模式
 */
enum class EBufferMode : uint8_t
{
	None, // 无缓冲
	Line, // 行缓冲
	Full  // 全缓冲
};

/**
 * @brief 流操作结果
 */
struct SStreamResult
{
	bool bSuccess = false;
	int32_t BytesProcessed = 0;
	TString ErrorMessage;

	SStreamResult() = default;
	SStreamResult(bool bInSuccess, int32_t InBytesProcessed = 0)
	    : bSuccess(bInSuccess),
	      BytesProcessed(InBytesProcessed)
	{}
	SStreamResult(bool bInSuccess, const TString& InErrorMessage)
	    : bSuccess(bInSuccess),
	      ErrorMessage(InErrorMessage)
	{}

	operator bool() const
	{
		return bSuccess;
	}
};

/**
 * @brief 流基类
 *
 * 提供所有流类型的基础接口
 */
NCLASS()
class NStream : public NObject
{
	GENERATED_BODY()

public:
	virtual ~NStream() = default;

public:
	// === 基本属性 ===

	/**
	 * @brief 检查流是否可以读取
	 */
	virtual bool CanRead() const = 0;

	/**
	 * @brief 检查流是否可以写入
	 */
	virtual bool CanWrite() const = 0;

	/**
	 * @brief 检查流是否可以寻址
	 */
	virtual bool CanSeek() const = 0;

	/**
	 * @brief 检查流是否已关闭
	 */
	virtual bool IsClosed() const = 0;

	/**
	 * @brief 检查流是否到达末尾
	 */
	virtual bool IsEOF() const = 0;

public:
	// === 流信息 ===

	/**
	 * @brief 获取流长度
	 */
	virtual int64_t GetLength() const = 0;

	/**
	 * @brief 获取当前位置
	 */
	virtual int64_t GetPosition() const = 0;

	/**
	 * @brief 设置当前位置
	 */
	virtual bool SetPosition(int64_t Position) = 0;

public:
	// === 流操作 ===

	/**
	 * @brief 读取数据
	 */
	virtual SStreamResult Read(uint8_t* Buffer, int32_t Size) = 0;

	/**
	 * @brief 写入数据
	 */
	virtual SStreamResult Write(const uint8_t* Buffer, int32_t Size) = 0;

	/**
	 * @brief 寻址
	 */
	virtual int64_t Seek(int64_t Offset, ESeekOrigin Origin) = 0;

	/**
	 * @brief 刷新缓冲区
	 */
	virtual bool Flush() = 0;

	/**
	 * @brief 关闭流
	 */
	virtual void Close() = 0;

public:
	// === 便捷方法 ===

	/**
	 * @brief 读取单个字节
	 */
	virtual int32_t ReadByte()
	{
		uint8_t Byte;
		auto Result = Read(&Byte, 1);
		return Result.bSuccess && Result.BytesProcessed > 0 ? Byte : -1;
	}

	/**
	 * @brief 写入单个字节
	 */
	virtual bool WriteByte(uint8_t Byte)
	{
		auto Result = Write(&Byte, 1);
		return Result.bSuccess && Result.BytesProcessed > 0;
	}

	/**
	 * @brief 读取所有数据到数组
	 */
	virtual TArray<uint8_t, CMemoryManager> ReadAll()
	{
		TArray<uint8_t, CMemoryManager> Data;

		int64_t Length = GetLength();
		if (Length > 0)
		{
			Data.Resize(static_cast<int32_t>(Length));
			auto Result = Read(Data.GetData(), Data.Size());
			if (!Result.bSuccess)
			{
				Data.Clear();
			}
			else if (Result.BytesProcessed < Data.Size())
			{
				Data.Resize(Result.BytesProcessed);
			}
		}

		return Data;
	}

	/**
	 * @brief 写入数组数据
	 */
	virtual bool WriteAll(const TArray<uint8_t, CMemoryManager>& Data)
	{
		if (Data.IsEmpty())
		{
			return true;
		}

		auto Result = Write(Data.GetData(), Data.Size());
		return Result.bSuccess && Result.BytesProcessed == Data.Size();
	}

	/**
	 * @brief 复制流数据到另一个流
	 */
	virtual int64_t CopyTo(NStream& DestinationStream, int32_t BufferSize = 4096)
	{
		if (!CanRead() || !DestinationStream.CanWrite())
		{
			return 0;
		}

		TArray<uint8_t, CMemoryManager> Buffer(BufferSize);
		int64_t TotalCopied = 0;

		while (!IsEOF())
		{
			auto ReadResult = Read(Buffer.GetData(), BufferSize);
			if (!ReadResult.bSuccess || ReadResult.BytesProcessed <= 0)
			{
				break;
			}

			auto WriteResult = DestinationStream.Write(Buffer.GetData(), ReadResult.BytesProcessed);
			if (!WriteResult.bSuccess)
			{
				break;
			}

			TotalCopied += WriteResult.BytesProcessed;

			if (WriteResult.BytesProcessed < ReadResult.BytesProcessed)
			{
				break; // 目标流写入不完整
			}
		}

		return TotalCopied;
	}
};

/**
 * @brief 文件流
 *
 * 提供文件的流式读写访问
 */
class CFileStream : public NStream
{
	GENERATED_BODY()

public:
	// === 构造函数 ===

	CFileStream();
	explicit CFileStream(const CPath& FilePath,
	                     EStreamAccess Access = EStreamAccess::ReadWrite,
	                     EStreamMode Mode = EStreamMode::OpenOrCreate);
	~CFileStream() override;

public:
	// === 打开和关闭 ===

	/**
	 * @brief 打开文件
	 */
	bool Open(const CPath& FilePath,
	          EStreamAccess Access = EStreamAccess::ReadWrite,
	          EStreamMode Mode = EStreamMode::OpenOrCreate);

	/**
	 * @brief 关闭文件
	 */
	void Close() override;

public:
	// === NStream接口实现 ===

	bool CanRead() const override;
	bool CanWrite() const override;
	bool CanSeek() const override;
	bool IsClosed() const override;
	bool IsEOF() const override;

	int64_t GetLength() const override;
	int64_t GetPosition() const override;
	bool SetPosition(int64_t Position) override;

	SStreamResult Read(uint8_t* Buffer, int32_t Size) override;
	SStreamResult Write(const uint8_t* Buffer, int32_t Size) override;
	int64_t Seek(int64_t Offset, ESeekOrigin Origin) override;
	bool Flush() override;

public:
	// === 文件特定操作 ===

	/**
	 * @brief 获取文件路径
	 */
	const CPath& GetFilePath() const
	{
		return FilePath;
	}

	/**
	 * @brief 设置文件大小
	 */
	bool SetLength(int64_t Length);

	/**
	 * @brief 锁定文件区域
	 */
	bool Lock(int64_t Position, int64_t Length, bool bExclusive = true);

	/**
	 * @brief 解锁文件区域
	 */
	bool Unlock(int64_t Position, int64_t Length);

private:
	CPath FilePath;
	EStreamAccess Access;
	std::unique_ptr<std::fstream> FileHandle;
	bool bIsOpen;
};

/**
 * @brief 内存流
 *
 * 在内存中提供流式访问
 */
class CMemoryStream : public NStream
{
	GENERATED_BODY()

public:
	// === 构造函数 ===

	CMemoryStream();
	explicit CMemoryStream(int32_t InitialCapacity);
	explicit CMemoryStream(const TArray<uint8_t, CMemoryManager>& Data);
	explicit CMemoryStream(const uint8_t* Data, int32_t Size);
	~CMemoryStream() override = default;

public:
	// === NStream接口实现 ===

	bool CanRead() const override
	{
		return true;
	}
	bool CanWrite() const override
	{
		return true;
	}
	bool CanSeek() const override
	{
		return true;
	}
	bool IsClosed() const override
	{
		return false;
	}
	bool IsEOF() const override
	{
		return Position >= Buffer.Size();
	}

	int64_t GetLength() const override
	{
		return Buffer.Size();
	}
	int64_t GetPosition() const override
	{
		return Position;
	}
	bool SetPosition(int64_t InPosition) override;

	SStreamResult Read(uint8_t* OutBuffer, int32_t Size) override;
	SStreamResult Write(const uint8_t* InBuffer, int32_t Size) override;
	int64_t Seek(int64_t Offset, ESeekOrigin Origin) override;
	bool Flush() override
	{
		return true;
	}
	void Close() override
	{}

public:
	// === 内存流特定操作 ===

	/**
	 * @brief 获取内部缓冲区
	 */
	const TArray<uint8_t, CMemoryManager>& GetBuffer() const
	{
		return Buffer;
	}

	/**
	 * @brief 获取可写缓冲区
	 */
	TArray<uint8_t, CMemoryManager>& GetMutableBuffer()
	{
		return Buffer;
	}

	/**
	 * @brief 设置容量
	 */
	void SetCapacity(int32_t Capacity);

	/**
	 * @brief 清空缓冲区
	 */
	void Clear();

	/**
	 * @brief 转换为数组
	 */
	TArray<uint8_t, CMemoryManager> ToArray() const;

private:
	TArray<uint8_t, CMemoryManager> Buffer;
	int64_t Position;
};

/**
 * @brief 缓冲流包装器
 *
 * 为任何流添加缓冲功能
 */
class CBufferedStream : public NStream
{
	GENERATED_BODY()

public:
	// === 构造函数 ===

	explicit CBufferedStream(TSharedPtr<NStream> InnerStream, int32_t BufferSize = 8192);
	~CBufferedStream() override;

public:
	// === NStream接口实现 ===

	bool CanRead() const override
	{
		return InnerStream && InnerStream->CanRead();
	}
	bool CanWrite() const override
	{
		return InnerStream && InnerStream->CanWrite();
	}
	bool CanSeek() const override
	{
		return InnerStream && InnerStream->CanSeek();
	}
	bool IsClosed() const override
	{
		return !InnerStream || InnerStream->IsClosed();
	}
	bool IsEOF() const override;

	int64_t GetLength() const override
	{
		return InnerStream ? InnerStream->GetLength() : 0;
	}
	int64_t GetPosition() const override;
	bool SetPosition(int64_t Position) override;

	SStreamResult Read(uint8_t* Buffer, int32_t Size) override;
	SStreamResult Write(const uint8_t* Buffer, int32_t Size) override;
	int64_t Seek(int64_t Offset, ESeekOrigin Origin) override;
	bool Flush() override;
	void Close() override;

public:
	// === 缓冲流特定操作 ===

	/**
	 * @brief 获取缓冲区大小
	 */
	int32_t GetBufferSize() const
	{
		return BufferSize;
	}

	/**
	 * @brief 获取内部流
	 */
	TSharedPtr<NStream> GetInnerStream() const
	{
		return InnerStream;
	}

private:
	// === 内部实现 ===

	void FlushReadBuffer();
	void FlushWriteBuffer();
	void InvalidateBuffer();

private:
	TSharedPtr<NStream> InnerStream;
	TArray<uint8_t, CMemoryManager> ReadBuffer;
	TArray<uint8_t, CMemoryManager> WriteBuffer;
	int32_t BufferSize;
	int32_t ReadBufferPos;
	int32_t ReadBufferSize;
	int32_t WriteBufferPos;
	bool bBufferDirty;
};

/**
 * @brief 流工厂类
 *
 * 提供创建各种流的便捷方法
 */
class NStreamFactory
{
public:
	/**
	 * @brief 创建文件流
	 */
	static TSharedPtr<CFileStream> CreateFileStream(const CPath& FilePath,
	                                                EStreamAccess Access = EStreamAccess::ReadWrite,
	                                                EStreamMode Mode = EStreamMode::OpenOrCreate);

	/**
	 * @brief 创建内存流
	 */
	static TSharedPtr<CMemoryStream> CreateMemoryStream(int32_t InitialCapacity = 0);

	/**
	 * @brief 从数据创建内存流
	 */
	static TSharedPtr<CMemoryStream> CreateMemoryStreamFromData(const TArray<uint8_t, CMemoryManager>& Data);

	/**
	 * @brief 创建缓冲流
	 */
	static TSharedPtr<CBufferedStream> CreateBufferedStream(TSharedPtr<NStream> InnerStream, int32_t BufferSize = 8192);

	/**
	 * @brief 创建只读文件流
	 */
	static TSharedPtr<CFileStream> OpenReadOnly(const CPath& FilePath);

	/**
	 * @brief 创建只写文件流
	 */
	static TSharedPtr<CFileStream> CreateWriteOnly(const CPath& FilePath, bool bOverwrite = true);

	/**
	 * @brief 创建追加文件流
	 */
	static TSharedPtr<CFileStream> OpenAppend(const CPath& FilePath);
};

// === 便捷函数 ===

/**
 * @brief 从文件读取所有数据
 */
inline TArray<uint8_t, CMemoryManager> ReadFileToArray(const CPath& FilePath)
{
	auto Stream = NStreamFactory::OpenReadOnly(FilePath);
	return Stream ? Stream->ReadAll() : TArray<uint8_t, CMemoryManager>();
}

/**
 * @brief 将数据写入文件
 */
inline bool WriteArrayToFile(const CPath& FilePath, const TArray<uint8_t, CMemoryManager>& Data)
{
	auto Stream = NStreamFactory::CreateWriteOnly(FilePath);
	return Stream && Stream->WriteAll(Data);
}

// === 类型别名 ===
using FStream = NStream;
using FFileStream = CFileStream;
using FMemoryStream = CMemoryStream;
using FBufferedStream = CBufferedStream;
using FStreamFactory = NStreamFactory;

} // namespace NLib