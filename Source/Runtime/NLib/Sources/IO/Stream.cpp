#include "Stream.h"

#include "FileSystem.h"
#include "Logging/LogCategory.h"

#include <algorithm>
#include <fstream>

namespace NLib
{
// === CFileStream 实现 ===

CFileStream::CFileStream()
    : bIsOpen(false),
      Access(EStreamAccess::ReadWrite)
{}

CFileStream::CFileStream(const CPath& InFilePath, EStreamAccess InAccess, EStreamMode Mode)
    : bIsOpen(false),
      Access(InAccess)
{
	Open(InFilePath, InAccess, Mode);
}

CFileStream::~CFileStream()
{
	Close();
}

bool CFileStream::Open(const CPath& InFilePath, EStreamAccess InAccess, EStreamMode Mode)
{
	Close(); // 关闭现有的文件

	FilePath = InFilePath;
	Access = InAccess;

	// 确保父目录存在
	auto ParentDir = FilePath.GetDirectoryName();
	if (!ParentDir.IsEmpty() && !CFileSystem::Exists(ParentDir))
	{
		auto Result = CFileSystem::CreateDirectory(ParentDir, true);
		if (!Result.bSuccess)
		{
			NLOG_IO(Error, "Failed to create parent directory for file: {}", FilePath.GetData());
			return false;
		}
	}

	// 确定打开模式
	std::ios_base::openmode OpenMode = std::ios_base::binary;

	// 设置读写模式
	if ((static_cast<uint8_t>(Access) & static_cast<uint8_t>(EStreamAccess::Read)) != 0)
	{
		OpenMode |= std::ios_base::in;
	}
	if ((static_cast<uint8_t>(Access) & static_cast<uint8_t>(EStreamAccess::Write)) != 0)
	{
		OpenMode |= std::ios_base::out;
	}

	// 设置文件模式
	switch (Mode)
	{
	case EStreamMode::Create:
		OpenMode |= std::ios_base::trunc;
		break;
	case EStreamMode::CreateNew:
		if (CFileSystem::Exists(FilePath))
		{
			NLOG_IO(Error, "File already exists: {}", FilePath.GetData());
			return false;
		}
		break;
	case EStreamMode::Open:
		if (!CFileSystem::Exists(FilePath))
		{
			NLOG_IO(Error, "File does not exist: {}", FilePath.GetData());
			return false;
		}
		break;
	case EStreamMode::OpenOrCreate:
		// 默认行为
		break;
	case EStreamMode::Truncate:
		OpenMode |= std::ios_base::trunc;
		if (!CFileSystem::Exists(FilePath))
		{
			NLOG_IO(Error, "File does not exist for truncation: {}", FilePath.GetData());
			return false;
		}
		break;
	case EStreamMode::Append:
		OpenMode |= std::ios_base::app;
		break;
	}

	try
	{
		FileHandle = std::make_unique<std::fstream>(FilePath.GetData(), OpenMode);
		if (FileHandle->is_open() && FileHandle->good())
		{
			bIsOpen = true;
			NLOG_IO(Debug, "Opened file stream: {}", FilePath.GetData());
			return true;
		}
		else
		{
			NLOG_IO(Error, "Failed to open file: {}", FilePath.GetData());
			FileHandle.reset();
			return false;
		}
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Exception opening file '{}': {}", FilePath.GetData(), e.what());
		FileHandle.reset();
		return false;
	}
}

void CFileStream::Close()
{
	if (FileHandle && bIsOpen)
	{
		try
		{
			FileHandle->close();
			NLOG_IO(Debug, "Closed file stream: {}", FilePath.GetData());
		}
		catch (const std::exception& e)
		{
			NLOG_IO(Error, "Exception closing file '{}': {}", FilePath.GetData(), e.what());
		}

		FileHandle.reset();
		bIsOpen = false;
	}
}

bool CFileStream::CanRead() const
{
	return bIsOpen && (static_cast<uint8_t>(Access) & static_cast<uint8_t>(EStreamAccess::Read)) != 0;
}

bool CFileStream::CanWrite() const
{
	return bIsOpen && (static_cast<uint8_t>(Access) & static_cast<uint8_t>(EStreamAccess::Write)) != 0;
}

bool CFileStream::CanSeek() const
{
	return bIsOpen;
}

bool CFileStream::IsClosed() const
{
	return !bIsOpen || !FileHandle || !FileHandle->is_open();
}

bool CFileStream::IsEOF() const
{
	return !bIsOpen || !FileHandle || FileHandle->eof();
}

int64_t CFileStream::GetLength() const
{
	if (!bIsOpen || !FileHandle)
	{
		return 0;
	}

	try
	{
		// 保存当前位置
		auto CurrentPos = FileHandle->tellg();

		// 移动到文件末尾
		FileHandle->seekg(0, std::ios_base::end);
		auto Length = FileHandle->tellg();

		// 恢复原位置
		FileHandle->seekg(CurrentPos);

		return static_cast<int64_t>(Length);
	}
	catch (...)
	{
		return 0;
	}
}

int64_t CFileStream::GetPosition() const
{
	if (!bIsOpen || !FileHandle)
	{
		return 0;
	}

	try
	{
		return static_cast<int64_t>(FileHandle->tellg());
	}
	catch (...)
	{
		return 0;
	}
}

bool CFileStream::SetPosition(int64_t Position)
{
	if (!bIsOpen || !FileHandle)
	{
		return false;
	}

	try
	{
		FileHandle->seekg(Position);
		FileHandle->seekp(Position);
		return FileHandle->good();
	}
	catch (...)
	{
		return false;
	}
}

SStreamResult CFileStream::Read(uint8_t* Buffer, int32_t Size)
{
	if (!CanRead() || !Buffer || Size <= 0)
	{
		return SStreamResult(false, "Invalid read parameters");
	}

	try
	{
		FileHandle->read(reinterpret_cast<char*>(Buffer), Size);
		int32_t BytesRead = static_cast<int32_t>(FileHandle->gcount());

		bool bSuccess = BytesRead > 0 || FileHandle->eof();
		return SStreamResult(bSuccess, BytesRead);
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Exception reading from file '{}': {}", FilePath.GetData(), e.what());
		return SStreamResult(false, e.what());
	}
}

SStreamResult CFileStream::Write(const uint8_t* Buffer, int32_t Size)
{
	if (!CanWrite() || !Buffer || Size <= 0)
	{
		return SStreamResult(false, "Invalid write parameters");
	}

	try
	{
		FileHandle->write(reinterpret_cast<const char*>(Buffer), Size);

		if (FileHandle->good())
		{
			return SStreamResult(true, Size);
		}
		else
		{
			return SStreamResult(false, "Write operation failed");
		}
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Exception writing to file '{}': {}", FilePath.GetData(), e.what());
		return SStreamResult(false, e.what());
	}
}

int64_t CFileStream::Seek(int64_t Offset, ESeekOrigin Origin)
{
	if (!CanSeek())
	{
		return -1;
	}

	try
	{
		std::ios_base::seekdir Dir;
		switch (Origin)
		{
		case ESeekOrigin::Begin:
			Dir = std::ios_base::beg;
			break;
		case ESeekOrigin::Current:
			Dir = std::ios_base::cur;
			break;
		case ESeekOrigin::End:
			Dir = std::ios_base::end;
			break;
		default:
			return -1;
		}

		FileHandle->seekg(Offset, Dir);
		FileHandle->seekp(Offset, Dir);

		if (FileHandle->good())
		{
			return static_cast<int64_t>(FileHandle->tellg());
		}
		else
		{
			return -1;
		}
	}
	catch (...)
	{
		return -1;
	}
}

bool CFileStream::Flush()
{
	if (!bIsOpen || !FileHandle)
	{
		return false;
	}

	try
	{
		FileHandle->flush();
		return FileHandle->good();
	}
	catch (...)
	{
		return false;
	}
}

bool CFileStream::SetLength(int64_t Length)
{
	if (!CanWrite())
	{
		return false;
	}

	// 这是一个简化实现，实际可能需要平台特定的代码
	try
	{
		auto CurrentPos = GetPosition();

		// 移动到指定长度位置
		if (SetPosition(Length))
		{
			// 在新位置写入一个字节然后删除（强制设置文件大小）
			// 这是一个hack，理想情况下应该使用系统调用如ftruncate
			if (Length > 0)
			{
				SetPosition(Length - 1);
				uint8_t Dummy = 0;
				Write(&Dummy, 1);
			}

			// 恢复原位置
			SetPosition(std::min(CurrentPos, Length));
			return true;
		}
	}
	catch (...)
	{}

	return false;
}

bool CFileStream::Lock(int64_t Position, int64_t Length, bool bExclusive)
{
	// 文件锁的实现需要平台特定的代码
	// 这里提供基础框架
	NLOG_IO(Warning, "File locking not fully implemented yet");
	return false;
}

bool CFileStream::Unlock(int64_t Position, int64_t Length)
{
	// 文件解锁的实现需要平台特定的代码
	NLOG_IO(Warning, "File unlocking not fully implemented yet");
	return false;
}

// === CMemoryStream 实现 ===

CMemoryStream::CMemoryStream()
    : Position(0)
{}

CMemoryStream::CMemoryStream(int32_t InitialCapacity)
    : Position(0)
{
	if (InitialCapacity > 0)
	{
		Buffer.Reserve(InitialCapacity);
	}
}

CMemoryStream::CMemoryStream(const TArray<uint8_t, CMemoryManager>& Data)
    : Buffer(Data),
      Position(0)
{}

CMemoryStream::CMemoryStream(const uint8_t* Data, int32_t Size)
    : Position(0)
{
	if (Data && Size > 0)
	{
		Buffer.Resize(Size);
		memcpy(Buffer.GetData(), Data, Size);
	}
}

bool CMemoryStream::SetPosition(int64_t InPosition)
{
	if (InPosition < 0)
	{
		return false;
	}

	Position = InPosition;
	return true;
}

SStreamResult CMemoryStream::Read(uint8_t* OutBuffer, int32_t Size)
{
	if (!OutBuffer || Size <= 0)
	{
		return SStreamResult(false, "Invalid read parameters");
	}

	if (Position >= Buffer.Size())
	{
		return SStreamResult(true, 0); // EOF
	}

	int32_t AvailableBytes = Buffer.Size() - static_cast<int32_t>(Position);
	int32_t BytesToRead = std::min(Size, AvailableBytes);

	if (BytesToRead > 0)
	{
		memcpy(OutBuffer, Buffer.GetData() + Position, BytesToRead);
		Position += BytesToRead;
	}

	return SStreamResult(true, BytesToRead);
}

SStreamResult CMemoryStream::Write(const uint8_t* InBuffer, int32_t Size)
{
	if (!InBuffer || Size <= 0)
	{
		return SStreamResult(false, "Invalid write parameters");
	}

	int64_t RequiredSize = Position + Size;

	// 扩展缓冲区如果需要
	if (RequiredSize > Buffer.Size())
	{
		Buffer.Resize(static_cast<int32_t>(RequiredSize));
	}

	memcpy(Buffer.GetData() + Position, InBuffer, Size);
	Position += Size;

	return SStreamResult(true, Size);
}

int64_t CMemoryStream::Seek(int64_t Offset, ESeekOrigin Origin)
{
	int64_t NewPosition;

	switch (Origin)
	{
	case ESeekOrigin::Begin:
		NewPosition = Offset;
		break;
	case ESeekOrigin::Current:
		NewPosition = Position + Offset;
		break;
	case ESeekOrigin::End:
		NewPosition = Buffer.Size() + Offset;
		break;
	default:
		return -1;
	}

	if (NewPosition < 0)
	{
		return -1;
	}

	Position = NewPosition;
	return Position;
}

void CMemoryStream::SetCapacity(int32_t Capacity)
{
	if (Capacity >= Buffer.Size())
	{
		Buffer.Reserve(Capacity);
	}
}

void CMemoryStream::Clear()
{
	Buffer.Clear();
	Position = 0;
}

TArray<uint8_t, CMemoryManager> CMemoryStream::ToArray() const
{
	return Buffer;
}

// === CBufferedStream 实现 ===

CBufferedStream::CBufferedStream(TSharedPtr<CStream> InInnerStream, int32_t InBufferSize)
    : InnerStream(InInnerStream),
      BufferSize(InBufferSize),
      ReadBufferPos(0),
      ReadBufferSize(0),
      WriteBufferPos(0),
      bBufferDirty(false)
{
	if (BufferSize > 0)
	{
		ReadBuffer.Resize(BufferSize);
		WriteBuffer.Resize(BufferSize);
	}
}

CBufferedStream::~CBufferedStream()
{
	Close();
}

bool CBufferedStream::IsEOF() const
{
	if (!InnerStream)
	{
		return true;
	}

	// 如果读缓冲区还有数据，就不是EOF
	if (ReadBufferPos < ReadBufferSize)
	{
		return false;
	}

	return InnerStream->IsEOF();
}

int64_t CBufferedStream::GetPosition() const
{
	if (!InnerStream)
	{
		return 0;
	}

	int64_t InnerPos = InnerStream->GetPosition();

	// 调整读缓冲区的影响
	if (ReadBufferSize > 0)
	{
		InnerPos -= (ReadBufferSize - ReadBufferPos);
	}

	// 调整写缓冲区的影响
	if (WriteBufferPos > 0)
	{
		InnerPos += WriteBufferPos;
	}

	return InnerPos;
}

bool CBufferedStream::SetPosition(int64_t Position)
{
	if (!InnerStream || !InnerStream->CanSeek())
	{
		return false;
	}

	// 刷新缓冲区
	FlushWriteBuffer();
	InvalidateBuffer();

	return InnerStream->SetPosition(Position);
}

SStreamResult CBufferedStream::Read(uint8_t* Buffer, int32_t Size)
{
	if (!InnerStream || !CanRead() || !Buffer || Size <= 0)
	{
		return SStreamResult(false, "Invalid read parameters");
	}

	// 先刷新写缓冲区
	if (WriteBufferPos > 0)
	{
		FlushWriteBuffer();
	}

	int32_t TotalBytesRead = 0;

	while (Size > 0 && !IsEOF())
	{
		// 如果读缓冲区为空，填充它
		if (ReadBufferPos >= ReadBufferSize)
		{
			auto ReadResult = InnerStream->Read(ReadBuffer.GetData(), BufferSize);
			if (!ReadResult.bSuccess)
			{
				break;
			}

			ReadBufferSize = ReadResult.BytesProcessed;
			ReadBufferPos = 0;

			if (ReadBufferSize == 0)
			{
				break; // EOF
			}
		}

		// 从读缓冲区复制数据
		int32_t AvailableBytes = ReadBufferSize - ReadBufferPos;
		int32_t BytesToCopy = std::min(Size, AvailableBytes);

		memcpy(Buffer + TotalBytesRead, ReadBuffer.GetData() + ReadBufferPos, BytesToCopy);

		ReadBufferPos += BytesToCopy;
		TotalBytesRead += BytesToCopy;
		Size -= BytesToCopy;
	}

	return SStreamResult(TotalBytesRead > 0, TotalBytesRead);
}

SStreamResult CBufferedStream::Write(const uint8_t* Buffer, int32_t Size)
{
	if (!InnerStream || !CanWrite() || !Buffer || Size <= 0)
	{
		return SStreamResult(false, "Invalid write parameters");
	}

	// 清空读缓冲区
	if (ReadBufferSize > 0)
	{
		InvalidateBuffer();
	}

	int32_t TotalBytesWritten = 0;

	while (Size > 0)
	{
		// 如果写缓冲区满了，刷新它
		if (WriteBufferPos >= BufferSize)
		{
			if (!FlushWriteBuffer())
			{
				break;
			}
		}

		// 向写缓冲区添加数据
		int32_t AvailableSpace = BufferSize - WriteBufferPos;
		int32_t BytesToCopy = std::min(Size, AvailableSpace);

		memcpy(WriteBuffer.GetData() + WriteBufferPos, Buffer + TotalBytesWritten, BytesToCopy);

		WriteBufferPos += BytesToCopy;
		TotalBytesWritten += BytesToCopy;
		Size -= BytesToCopy;
		bBufferDirty = true;
	}

	return SStreamResult(TotalBytesWritten > 0, TotalBytesWritten);
}

int64_t CBufferedStream::Seek(int64_t Offset, ESeekOrigin Origin)
{
	if (!InnerStream || !CanSeek())
	{
		return -1;
	}

	// 刷新写缓冲区
	FlushWriteBuffer();
	InvalidateBuffer();

	return InnerStream->Seek(Offset, Origin);
}

bool CBufferedStream::Flush()
{
	if (!InnerStream)
	{
		return false;
	}

	bool Result = FlushWriteBuffer();
	return Result && InnerStream->Flush();
}

void CBufferedStream::Close()
{
	if (InnerStream)
	{
		Flush();
		InnerStream->Close();
		InvalidateBuffer();
	}
}

void CBufferedStream::FlushReadBuffer()
{
	ReadBufferPos = 0;
	ReadBufferSize = 0;
}

bool CBufferedStream::FlushWriteBuffer()
{
	if (WriteBufferPos > 0 && InnerStream)
	{
		auto WriteResult = InnerStream->Write(WriteBuffer.GetData(), WriteBufferPos);
		bool bSuccess = WriteResult.bSuccess && WriteResult.BytesProcessed == WriteBufferPos;

		WriteBufferPos = 0;
		bBufferDirty = false;

		return bSuccess;
	}

	return true;
}

void CBufferedStream::InvalidateBuffer()
{
	FlushReadBuffer();
	WriteBufferPos = 0;
	bBufferDirty = false;
}

// === CStreamFactory 实现 ===

TSharedPtr<CFileStream> CStreamFactory::CreateFileStream(const CPath& FilePath, EStreamAccess Access, EStreamMode Mode)
{
	auto Stream = MakeShared<CFileStream>();
	if (Stream->Open(FilePath, Access, Mode))
	{
		return Stream;
	}
	return nullptr;
}

TSharedPtr<CMemoryStream> CStreamFactory::CreateMemoryStream(int32_t InitialCapacity)
{
	return MakeShared<CMemoryStream>(InitialCapacity);
}

TSharedPtr<CMemoryStream> CStreamFactory::CreateMemoryStreamFromData(const TArray<uint8_t, CMemoryManager>& Data)
{
	return MakeShared<CMemoryStream>(Data);
}

TSharedPtr<CBufferedStream> CStreamFactory::CreateBufferedStream(TSharedPtr<CStream> InnerStream, int32_t BufferSize)
{
	if (!InnerStream)
	{
		return nullptr;
	}

	return MakeShared<CBufferedStream>(InnerStream, BufferSize);
}

TSharedPtr<CFileStream> CStreamFactory::OpenReadOnly(const CPath& FilePath)
{
	return CreateFileStream(FilePath, EStreamAccess::Read, EStreamMode::Open);
}

TSharedPtr<CFileStream> CStreamFactory::CreateWriteOnly(const CPath& FilePath, bool bOverwrite)
{
	EStreamMode Mode = bOverwrite ? EStreamMode::Create : EStreamMode::CreateNew;
	return CreateFileStream(FilePath, EStreamAccess::Write, Mode);
}

TSharedPtr<CFileStream> CStreamFactory::OpenAppend(const CPath& FilePath)
{
	return CreateFileStream(FilePath, EStreamAccess::Write, EStreamMode::Append);
}

} // namespace NLib