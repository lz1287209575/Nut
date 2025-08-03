#include "NFileSystem.h"
#include "NFileSystem.generate.h"

#include "Logging/CLogger.h"
#include "Memory/CMemoryManager.h"

#include <algorithm>
#include <fstream>
#include <sstream>

// 平台特定头文件
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>

#include <sys/stat.h>
#else
#include <climits>
#include <cstring>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace NLib
{

// === 平台特定常量定义 ===

#ifdef _WIN32
const char NPath::DirectorySeparatorChar = '\\';
const char NPath::AltDirectorySeparatorChar = '/';
const char NPath::PathSeparator = ';';
const char NPath::VolumeSeparatorChar = ':';
#else
const char NPath::DirectorySeparatorChar = '/';
const char NPath::AltDirectorySeparatorChar = '/';
const char NPath::PathSeparator = ':';
const char NPath::VolumeSeparatorChar = '/';
#endif

// === 工具函数 ===

namespace
{
// 将EFileAttributes转换为平台特定属性
uint32_t ConvertAttributesToPlatform(EFileAttributes Attributes)
{
#ifdef _WIN32
	uint32_t Result = 0;
	if (static_cast<uint32_t>(Attributes) & static_cast<uint32_t>(EFileAttributes::ReadOnly))
		Result |= FILE_ATTRIBUTE_READONLY;
	if (static_cast<uint32_t>(Attributes) & static_cast<uint32_t>(EFileAttributes::Hidden))
		Result |= FILE_ATTRIBUTE_HIDDEN;
	if (static_cast<uint32_t>(Attributes) & static_cast<uint32_t>(EFileAttributes::System))
		Result |= FILE_ATTRIBUTE_SYSTEM;
	if (static_cast<uint32_t>(Attributes) & static_cast<uint32_t>(EFileAttributes::Directory))
		Result |= FILE_ATTRIBUTE_DIRECTORY;
	return Result;
#else
	// Unix/Linux没有直接的文件属性对应，使用权限位
	uint32_t Result = 0644; // 默认权限
	if (static_cast<uint32_t>(Attributes) & static_cast<uint32_t>(EFileAttributes::ReadOnly))
	{
		Result &= ~0200; // 移除写权限
	}
	return Result;
#endif
}

// 从平台特定属性转换为EFileAttributes
EFileAttributes ConvertAttributesFromPlatform(uint32_t PlatformAttributes)
{
#ifdef _WIN32
	uint32_t Result = 0;
	if (PlatformAttributes & FILE_ATTRIBUTE_READONLY)
		Result |= static_cast<uint32_t>(EFileAttributes::ReadOnly);
	if (PlatformAttributes & FILE_ATTRIBUTE_HIDDEN)
		Result |= static_cast<uint32_t>(EFileAttributes::Hidden);
	if (PlatformAttributes & FILE_ATTRIBUTE_SYSTEM)
		Result |= static_cast<uint32_t>(EFileAttributes::System);
	if (PlatformAttributes & FILE_ATTRIBUTE_DIRECTORY)
		Result |= static_cast<uint32_t>(EFileAttributes::Directory);
	if (PlatformAttributes & FILE_ATTRIBUTE_ARCHIVE)
		Result |= static_cast<uint32_t>(EFileAttributes::Archive);
	return static_cast<EFileAttributes>(Result);
#else
	uint32_t Result = 0;
	if (S_ISDIR(PlatformAttributes))
		Result |= static_cast<uint32_t>(EFileAttributes::Directory);
	if (!(PlatformAttributes & S_IWUSR))
		Result |= static_cast<uint32_t>(EFileAttributes::ReadOnly);
	return static_cast<EFileAttributes>(Result);
#endif
}

// 平台特定的文件时间转换
NDateTime ConvertFileTimeToNDateTime(int64_t FileTime)
{
#ifdef _WIN32
	return NDateTime::FromFileTime(FileTime);
#else
	return NDateTime::FromUnixTimestamp(FileTime);
#endif
}

int64_t ConvertNDateTimeToFileTime(const NDateTime& DateTime)
{
#ifdef _WIN32
	return DateTime.ToFileTime();
#else
	return DateTime.ToUnixTimestamp();
#endif
}

// 匹配通配符模式
bool MatchWildcard(const CString& Pattern, const CString& Text)
{
	const char* PatternPtr = Pattern.GetCStr();
	const char* TextPtr = Text.GetCStr();

	// 简化的通配符匹配实现
	while (*PatternPtr && *TextPtr)
	{
		if (*PatternPtr == '*')
		{
			if (*(PatternPtr + 1) == '\0')
				return true;

			while (*TextPtr)
			{
				if (MatchWildcard(CString(PatternPtr + 1), CString(TextPtr)))
					return true;
				TextPtr++;
			}
			return false;
		}
		else if (*PatternPtr == '?' || *PatternPtr == *TextPtr)
		{
			PatternPtr++;
			TextPtr++;
		}
		else
		{
			return false;
		}
	}

	while (*PatternPtr == '*')
		PatternPtr++;

	return *PatternPtr == '\0' && *TextPtr == '\0';
}
} // namespace

// === NFileInfo 实现 ===

NFileInfo::NFileInfo()
    : Length(0),
      Attributes(EFileAttributes::None)
{}

NFileInfo::NFileInfo(const CString& FilePath)
    : FullPath(FilePath),
      Length(0),
      Attributes(EFileAttributes::None)
{
	LoadFileInfo();
}

CString NFileInfo::GetName() const
{
	return NPath::GetFileName(FullPath);
}

CString NFileInfo::GetDirectoryName() const
{
	return NPath::GetDirectoryName(FullPath);
}

CString NFileInfo::GetExtension() const
{
	return NPath::GetExtension(FullPath);
}

bool NFileInfo::Exists() const
{
	return NFile::Exists(FullPath);
}

bool NFileInfo::IsDirectory() const
{
	return (static_cast<uint32_t>(Attributes) & static_cast<uint32_t>(EFileAttributes::Directory)) != 0;
}

bool NFileInfo::IsFile() const
{
	return Exists() && !IsDirectory();
}

bool NFileInfo::IsReadOnly() const
{
	return (static_cast<uint32_t>(Attributes) & static_cast<uint32_t>(EFileAttributes::ReadOnly)) != 0;
}

bool NFileInfo::IsHidden() const
{
	return (static_cast<uint32_t>(Attributes) & static_cast<uint32_t>(EFileAttributes::Hidden)) != 0;
}

void NFileInfo::Refresh()
{
	LoadFileInfo();
}

void NFileInfo::Delete()
{
	if (IsDirectory())
	{
		NDirectory::Delete(FullPath);
	}
	else
	{
		NFile::Delete(FullPath);
	}
}

void NFileInfo::MoveTo(const CString& DestPath)
{
	if (IsDirectory())
	{
		NDirectory::Move(FullPath, DestPath);
	}
	else
	{
		NFile::Move(FullPath, DestPath);
	}
	FullPath = DestPath;
	Refresh();
}

void NFileInfo::CopyTo(const CString& DestPath, bool Overwrite)
{
	if (!IsDirectory())
	{
		NFile::Copy(FullPath, DestPath, Overwrite);
	}
	else
	{
		CLogger::Error("NFileInfo::CopyTo: Cannot copy directory using this method");
	}
}

CString NFileInfo::ToString() const
{
	return FullPath;
}

void NFileInfo::LoadFileInfo()
{
#ifdef _WIN32
	WIN32_FILE_ATTRIBUTE_DATA FileData;
	if (GetFileAttributesExA(FullPath.GetCStr(), GetFileExInfoStandard, &FileData))
	{
		Attributes = ConvertAttributesFromPlatform(FileData.dwFileAttributes);

		LARGE_INTEGER FileSize;
		FileSize.LowPart = FileData.nFileSizeLow;
		FileSize.HighPart = FileData.nFileSizeHigh;
		Length = FileSize.QuadPart;

		// 转换文件时间
		LARGE_INTEGER CreationFileTime, AccessFileTime, WriteFileTime;
		CreationFileTime.LowPart = FileData.ftCreationTime.dwLowDateTime;
		CreationFileTime.HighPart = FileData.ftCreationTime.dwHighDateTime;
		AccessFileTime.LowPart = FileData.ftLastAccessTime.dwLowDateTime;
		AccessFileTime.HighPart = FileData.ftLastAccessTime.dwHighDateTime;
		WriteFileTime.LowPart = FileData.ftLastWriteTime.dwLowDateTime;
		WriteFileTime.HighPart = FileData.ftLastWriteTime.dwHighDateTime;

		CreationTime = ConvertFileTimeToNDateTime(CreationFileTime.QuadPart);
		LastAccessTime = ConvertFileTimeToNDateTime(AccessFileTime.QuadPart);
		LastWriteTime = ConvertFileTimeToNDateTime(WriteFileTime.QuadPart);
	}
	else
	{
		Attributes = EFileAttributes::None;
		Length = 0;
	}
#else
	struct stat FileStat;
	if (stat(FullPath.GetCStr(), &FileStat) == 0)
	{
		Attributes = ConvertAttributesFromPlatform(FileStat.st_mode);
		Length = FileStat.st_size;

		CreationTime = ConvertFileTimeToNDateTime(FileStat.st_ctime);
		LastAccessTime = ConvertFileTimeToNDateTime(FileStat.st_atime);
		LastWriteTime = ConvertFileTimeToNDateTime(FileStat.st_mtime);
	}
	else
	{
		Attributes = EFileAttributes::None;
		Length = 0;
	}
#endif
}

// === NDirectoryInfo 实现 ===

NDirectoryInfo::NDirectoryInfo()
{}

NDirectoryInfo::NDirectoryInfo(const CString& DirectoryPath)
    : FullPath(DirectoryPath)
{
	ValidatePath();
}

CString NDirectoryInfo::GetName() const
{
	return NPath::GetFileName(FullPath);
}

CString NDirectoryInfo::GetParent() const
{
	return NPath::GetDirectoryName(FullPath);
}

bool NDirectoryInfo::Exists() const
{
	return NDirectory::Exists(FullPath);
}

void NDirectoryInfo::Create()
{
	NDirectory::CreateDirectory(FullPath);
}

void NDirectoryInfo::Delete(bool Recursive)
{
	NDirectory::Delete(FullPath, Recursive);
}

void NDirectoryInfo::MoveTo(const CString& DestPath)
{
	NDirectory::Move(FullPath, DestPath);
	FullPath = DestPath;
}

CArray<NFileInfo> NDirectoryInfo::GetFiles() const
{
	return GetFiles(CString("*"));
}

CArray<NFileInfo> NDirectoryInfo::GetFiles(const CString& SearchPattern) const
{
	return GetFiles(SearchPattern, ESearchOption::TopDirectoryOnly);
}

CArray<NFileInfo> NDirectoryInfo::GetFiles(const CString& SearchPattern, ESearchOption SearchOption) const
{
	CArray<CString> FilePaths = NDirectory::GetFiles(FullPath, SearchPattern, SearchOption);
	CArray<NFileInfo> FileInfos;

	for (const auto& FilePath : FilePaths)
	{
		FileInfos.PushBack(NFileInfo(FilePath));
	}

	return FileInfos;
}

CArray<NDirectoryInfo> NDirectoryInfo::GetDirectories() const
{
	return GetDirectories(CString("*"));
}

CArray<NDirectoryInfo> NDirectoryInfo::GetDirectories(const CString& SearchPattern) const
{
	return GetDirectories(SearchPattern, ESearchOption::TopDirectoryOnly);
}

CArray<NDirectoryInfo> NDirectoryInfo::GetDirectories(const CString& SearchPattern, ESearchOption SearchOption) const
{
	CArray<CString> DirPaths = NDirectory::GetDirectories(FullPath, SearchPattern, SearchOption);
	CArray<NDirectoryInfo> DirInfos;

	for (const auto& DirPath : DirPaths)
	{
		DirInfos.PushBack(NDirectoryInfo(DirPath));
	}

	return DirInfos;
}

CString NDirectoryInfo::ToString() const
{
	return FullPath;
}

void NDirectoryInfo::ValidatePath() const
{
	if (FullPath.IsEmpty())
	{
		CLogger::Error("NDirectoryInfo: Directory path cannot be empty");
	}
}

// === NStream 实现 ===

CArray<uint8_t> NStream::ReadAllBytes()
{
	if (!CanRead())
	{
		CLogger::Error("NStream::ReadAllBytes: Stream is not readable");
		return CArray<uint8_t>();
	}

	int64_t Length = GetLength();
	if (Length <= 0)
	{
		return CArray<uint8_t>();
	}

	CArray<uint8_t> Buffer;
	Buffer.Resize(static_cast<size_t>(Length));

	int32_t BytesRead = Read(Buffer.GetData(), 0, static_cast<int32_t>(Length));
	if (BytesRead != Length)
	{
		Buffer.Resize(static_cast<size_t>(BytesRead));
	}

	return Buffer;
}

CString NStream::ReadAllText()
{
	CArray<uint8_t> Bytes = ReadAllBytes();
	if (Bytes.IsEmpty())
	{
		return CString();
	}

	return CString(reinterpret_cast<const char*>(Bytes.GetData()), Bytes.GetSize());
}

void NStream::WriteAllBytes(const CArray<uint8_t>& Data)
{
	if (!CanWrite())
	{
		CLogger::Error("NStream::WriteAllBytes: Stream is not writable");
		return;
	}

	if (!Data.IsEmpty())
	{
		Write(Data.GetData(), 0, static_cast<int32_t>(Data.GetSize()));
	}
}

void NStream::WriteAllText(const CString& Text)
{
	if (!Text.IsEmpty())
	{
		const char* TextData = Text.GetCStr();
		Write(reinterpret_cast<const uint8_t*>(TextData), 0, static_cast<int32_t>(Text.GetLength()));
	}
}

// === NFileStream 实现 ===

NFileStream::NFileStream()
    : Mode(EFileMode::Open),
      Access(EFileAccess::Read),
      Share(EFileShare::Read),
      FileHandle(nullptr)
{}

NFileStream::NFileStream(const CString& InFilePath, EFileMode InMode)
    : FilePath(InFilePath),
      Mode(InMode),
      Access(EFileAccess::ReadWrite),
      Share(EFileShare::Read),
      FileHandle(nullptr)
{
	OpenFile();
}

NFileStream::NFileStream(const CString& InFilePath, EFileMode InMode, EFileAccess InAccess)
    : FilePath(InFilePath),
      Mode(InMode),
      Access(InAccess),
      Share(EFileShare::Read),
      FileHandle(nullptr)
{
	OpenFile();
}

NFileStream::NFileStream(const CString& InFilePath, EFileMode InMode, EFileAccess InAccess, EFileShare InShare)
    : FilePath(InFilePath),
      Mode(InMode),
      Access(InAccess),
      Share(InShare),
      FileHandle(nullptr)
{
	OpenFile();
}

NFileStream::~NFileStream()
{
	CloseFile();
}

TSharedPtr<NFileStream> NFileStream::Create(const CString& InFilePath, EFileMode InMode)
{
	return NewNObject<NFileStream>(InFilePath, InMode);
}

TSharedPtr<NFileStream> NFileStream::OpenRead(const CString& InFilePath)
{
	return NewNObject<NFileStream>(InFilePath, EFileMode::Open, EFileAccess::Read);
}

TSharedPtr<NFileStream> NFileStream::OpenWrite(const CString& InFilePath)
{
	return NewNObject<NFileStream>(InFilePath, EFileMode::Create, EFileAccess::Write);
}

TSharedPtr<NFileStream> NFileStream::CreateText(const CString& InFilePath)
{
	return NewNObject<NFileStream>(InFilePath, EFileMode::CreateNew, EFileAccess::Write);
}

bool NFileStream::CanRead() const
{
	return IsOpen() && (static_cast<uint32_t>(Access) & static_cast<uint32_t>(EFileAccess::Read)) != 0;
}

bool NFileStream::CanWrite() const
{
	return IsOpen() && (static_cast<uint32_t>(Access) & static_cast<uint32_t>(EFileAccess::Write)) != 0;
}

bool NFileStream::CanSeek() const
{
	return IsOpen();
}

int64_t NFileStream::GetLength() const
{
	if (!IsOpen())
	{
		CLogger::Error("NFileStream::GetLength: File is not open");
		return 0;
	}

#ifdef _WIN32
	LARGE_INTEGER FileSize;
	if (GetFileSizeEx(static_cast<HANDLE>(FileHandle), &FileSize))
	{
		return FileSize.QuadPart;
	}
	return 0;
#else
	int FD = *static_cast<int*>(FileHandle);
	struct stat FileStat;
	if (fstat(FD, &FileStat) == 0)
	{
		return FileStat.st_size;
	}
	return 0;
#endif
}

int64_t NFileStream::GetPosition() const
{
	if (!IsOpen())
	{
		return 0;
	}

#ifdef _WIN32
	LARGE_INTEGER CurrentPos = {};
	LARGE_INTEGER NewPos;
	if (SetFilePointerEx(static_cast<HANDLE>(FileHandle), CurrentPos, &NewPos, FILE_CURRENT))
	{
		return NewPos.QuadPart;
	}
	return 0;
#else
	int FD = *static_cast<int*>(FileHandle);
	return lseek(FD, 0, SEEK_CUR);
#endif
}

void NFileStream::SetPosition(int64_t Position)
{
	Seek(Position, 0); // SEEK_SET
}

void NFileStream::Close()
{
	CloseFile();
}

void NFileStream::Flush()
{
	if (!IsOpen())
	{
		return;
	}

#ifdef _WIN32
	FlushFileBuffers(static_cast<HANDLE>(FileHandle));
#else
	int FD = *static_cast<int*>(FileHandle);
	fsync(FD);
#endif
}

int32_t NFileStream::ReadByte()
{
	uint8_t Byte;
	int32_t BytesRead = Read(&Byte, 0, 1);
	return BytesRead == 1 ? Byte : -1;
}

int32_t NFileStream::Read(uint8_t* Buffer, int32_t Offset, int32_t Count)
{
	if (!CanRead() || !Buffer || Count <= 0)
	{
		return 0;
	}

#ifdef _WIN32
	DWORD BytesRead;
	if (ReadFile(static_cast<HANDLE>(FileHandle), Buffer + Offset, static_cast<DWORD>(Count), &BytesRead, nullptr))
	{
		return static_cast<int32_t>(BytesRead);
	}
	return 0;
#else
	int FD = *static_cast<int*>(FileHandle);
	ssize_t BytesRead = read(FD, Buffer + Offset, static_cast<size_t>(Count));
	return BytesRead >= 0 ? static_cast<int32_t>(BytesRead) : 0;
#endif
}

void NFileStream::WriteByte(uint8_t Value)
{
	Write(&Value, 0, 1);
}

void NFileStream::Write(const uint8_t* Buffer, int32_t Offset, int32_t Count)
{
	if (!CanWrite() || !Buffer || Count <= 0)
	{
		return;
	}

#ifdef _WIN32
	DWORD BytesWritten;
	WriteFile(static_cast<HANDLE>(FileHandle), Buffer + Offset, static_cast<DWORD>(Count), &BytesWritten, nullptr);
#else
	int FD = *static_cast<int*>(FileHandle);
	write(FD, Buffer + Offset, static_cast<size_t>(Count));
#endif
}

int64_t NFileStream::Seek(int64_t Offset, int32_t Origin)
{
	if (!CanSeek())
	{
		return GetPosition();
	}

#ifdef _WIN32
	LARGE_INTEGER OffsetValue;
	OffsetValue.QuadPart = Offset;
	LARGE_INTEGER NewPos;

	DWORD MoveMethod = FILE_BEGIN;
	if (Origin == 1)
		MoveMethod = FILE_CURRENT; // SEEK_CUR
	else if (Origin == 2)
		MoveMethod = FILE_END; // SEEK_END

	if (SetFilePointerEx(static_cast<HANDLE>(FileHandle), OffsetValue, &NewPos, MoveMethod))
	{
		return NewPos.QuadPart;
	}
	return GetPosition();
#else
	int FD = *static_cast<int*>(FileHandle);
	off_t NewPos = lseek(FD, static_cast<off_t>(Offset), Origin);
	return NewPos >= 0 ? static_cast<int64_t>(NewPos) : GetPosition();
#endif
}

void NFileStream::SetLength(int64_t Length)
{
	if (!CanWrite())
	{
		CLogger::Error("NFileStream::SetLength: File is not writable");
		return;
	}

#ifdef _WIN32
	int64_t CurrentPos = GetPosition();
	SetPosition(Length);
	SetEndOfFile(static_cast<HANDLE>(FileHandle));
	SetPosition(CurrentPos);
#else
	int FD = *static_cast<int*>(FileHandle);
	ftruncate(FD, static_cast<off_t>(Length));
#endif
}

void NFileStream::OpenFile()
{
	CloseFile();

#ifdef _WIN32
	DWORD DesiredAccess = 0;
	if (static_cast<uint32_t>(Access) & static_cast<uint32_t>(EFileAccess::Read))
		DesiredAccess |= GENERIC_READ;
	if (static_cast<uint32_t>(Access) & static_cast<uint32_t>(EFileAccess::Write))
		DesiredAccess |= GENERIC_WRITE;

	DWORD ShareMode = 0;
	if (static_cast<uint32_t>(Share) & static_cast<uint32_t>(EFileShare::Read))
		ShareMode |= FILE_SHARE_READ;
	if (static_cast<uint32_t>(Share) & static_cast<uint32_t>(EFileShare::Write))
		ShareMode |= FILE_SHARE_WRITE;
	if (static_cast<uint32_t>(Share) & static_cast<uint32_t>(EFileShare::Delete))
		ShareMode |= FILE_SHARE_DELETE;

	DWORD CreationDisposition = OPEN_EXISTING;
	switch (Mode)
	{
	case EFileMode::CreateNew:
		CreationDisposition = CREATE_NEW;
		break;
	case EFileMode::Create:
		CreationDisposition = CREATE_ALWAYS;
		break;
	case EFileMode::Open:
		CreationDisposition = OPEN_EXISTING;
		break;
	case EFileMode::OpenOrCreate:
		CreationDisposition = OPEN_ALWAYS;
		break;
	case EFileMode::Truncate:
		CreationDisposition = TRUNCATE_EXISTING;
		break;
	case EFileMode::Append:
		CreationDisposition = OPEN_ALWAYS;
		break;
	}

	FileHandle = CreateFileA(
	    FilePath.GetCStr(), DesiredAccess, ShareMode, nullptr, CreationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (FileHandle == INVALID_HANDLE_VALUE)
	{
		FileHandle = nullptr;
		CLogger::Error("NFileStream::OpenFile: Failed to open file: {}", FilePath.GetCStr());
		return;
	}

	if (Mode == EFileMode::Append)
	{
		SetPosition(GetLength());
	}
#else
	int Flags = 0;

	if ((static_cast<uint32_t>(Access) & static_cast<uint32_t>(EFileAccess::Read)) &&
	    (static_cast<uint32_t>(Access) & static_cast<uint32_t>(EFileAccess::Write)))
	{
		Flags = O_RDWR;
	}
	else if (static_cast<uint32_t>(Access) & static_cast<uint32_t>(EFileAccess::Write))
	{
		Flags = O_WRONLY;
	}
	else
	{
		Flags = O_RDONLY;
	}

	switch (Mode)
	{
	case EFileMode::CreateNew:
		Flags |= O_CREAT | O_EXCL;
		break;
	case EFileMode::Create:
		Flags |= O_CREAT | O_TRUNC;
		break;
	case EFileMode::Open:
		break;
	case EFileMode::OpenOrCreate:
		Flags |= O_CREAT;
		break;
	case EFileMode::Truncate:
		Flags |= O_TRUNC;
		break;
	case EFileMode::Append:
		Flags |= O_CREAT | O_APPEND;
		break;
	}

	int FD = open(FilePath.GetCStr(), Flags, 0666);

	if (FD == -1)
	{
		CLogger::Error("NFileStream::OpenFile: Failed to open file: {} ({})", FilePath.GetCStr(), strerror(errno));
		return;
	}

	FileHandle = CMemoryManager::GetInstance().Allocate(sizeof(int), alignof(int));
	*static_cast<int*>(FileHandle) = FD;
#endif
}

void NFileStream::CloseFile()
{
	if (FileHandle)
	{
#ifdef _WIN32
		CloseHandle(static_cast<HANDLE>(FileHandle));
#else
		int FD = *static_cast<int*>(FileHandle);
		close(FD);
		CMemoryManager::GetInstance().Deallocate(FileHandle);
#endif
		FileHandle = nullptr;
	}
}

// === NPath 实现 ===

CString NPath::Combine(const CString& Path1, const CString& Path2)
{
	if (Path1.IsEmpty())
		return Path2;
	if (Path2.IsEmpty())
		return Path1;

	CString Result = Path1;
	if (Result.GetData()[Result.GetLength() - 1] != DirectorySeparatorChar &&
	    Result.GetData()[Result.GetLength() - 1] != AltDirectorySeparatorChar)
	{
		Result += DirectorySeparatorChar;
	}

	const char* Path2Start = Path2.GetCStr();
	if (*Path2Start == DirectorySeparatorChar || *Path2Start == AltDirectorySeparatorChar)
	{
		Path2Start++;
	}

	Result += Path2Start;
	return Result;
}

CString NPath::Combine(const CString& Path1, const CString& Path2, const CString& Path3)
{
	return Combine(Combine(Path1, Path2), Path3);
}

CString NPath::Combine(const CArray<CString>& Paths)
{
	if (Paths.IsEmpty())
		return CString();

	CString Result = Paths[0];
	for (size_t i = 1; i < Paths.GetSize(); ++i)
	{
		Result = Combine(Result, Paths[i]);
	}
	return Result;
}

CString NPath::GetDirectoryName(const CString& Path)
{
	if (Path.IsEmpty())
		return CString();

	const char* PathStr = Path.GetCStr();
	const char* LastSep = nullptr;

	for (const char* p = PathStr; *p; ++p)
	{
		if (*p == DirectorySeparatorChar || *p == AltDirectorySeparatorChar)
		{
			LastSep = p;
		}
	}

	if (LastSep)
	{
		return CString(PathStr, static_cast<size_t>(LastSep - PathStr));
	}

	return CString();
}

CString NPath::GetFileName(const CString& Path)
{
	if (Path.IsEmpty())
		return CString();

	const char* PathStr = Path.GetCStr();
	const char* LastSep = nullptr;

	for (const char* p = PathStr; *p; ++p)
	{
		if (*p == DirectorySeparatorChar || *p == AltDirectorySeparatorChar)
		{
			LastSep = p;
		}
	}

	return CString(LastSep ? LastSep + 1 : PathStr);
}

CString NPath::GetFileNameWithoutExtension(const CString& Path)
{
	CString FileName = GetFileName(Path);
	if (FileName.IsEmpty())
		return CString();

	const char* FileNameStr = FileName.GetCStr();
	const char* LastDot = nullptr;

	for (const char* p = FileNameStr; *p; ++p)
	{
		if (*p == '.')
		{
			LastDot = p;
		}
	}

	if (LastDot && LastDot != FileNameStr)
	{
		return CString(FileNameStr, static_cast<size_t>(LastDot - FileNameStr));
	}

	return FileName;
}

CString NPath::GetExtension(const CString& Path)
{
	if (Path.IsEmpty())
		return CString();

	const char* PathStr = Path.GetCStr();
	const char* LastDot = nullptr;
	const char* LastSep = nullptr;

	for (const char* p = PathStr; *p; ++p)
	{
		if (*p == '.')
		{
			LastDot = p;
		}
		else if (*p == DirectorySeparatorChar || *p == AltDirectorySeparatorChar)
		{
			LastSep = p;
			LastDot = nullptr; // Reset dot after separator
		}
	}

	if (LastDot && (!LastSep || LastDot > LastSep))
	{
		return CString(LastDot);
	}

	return CString();
}

CString NPath::GetFullPath(const CString& Path)
{
	if (IsPathRooted(Path))
		return NormalizePath(Path);

	CString CurrentDir = NDirectory::GetCurrentDirectory();
	return NormalizePath(Combine(CurrentDir, Path));
}

bool NPath::IsPathRooted(const CString& Path)
{
	if (Path.IsEmpty())
		return false;

#ifdef _WIN32
	const char* PathStr = Path.GetCStr();
	return (PathStr[0] == DirectorySeparatorChar || PathStr[0] == AltDirectorySeparatorChar) ||
	       (Path.GetLength() >= 2 && PathStr[1] == VolumeSeparatorChar);
#else
	return Path.GetCStr()[0] == DirectorySeparatorChar;
#endif
}

bool NPath::HasExtension(const CString& Path)
{
	return !GetExtension(Path).IsEmpty();
}

CArray<char> NPath::GetInvalidPathChars()
{
	CArray<char> InvalidChars;
#ifdef _WIN32
	const char* Chars = "<>:\"|?*\0\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
	                    "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F";
#else
	const char* Chars = "\0";
#endif

	for (const char* p = Chars; *p; ++p)
	{
		InvalidChars.PushBack(*p);
	}
	return InvalidChars;
}

CArray<char> NPath::GetInvalidFileNameChars()
{
	CArray<char> InvalidChars = GetInvalidPathChars();
	InvalidChars.PushBack(DirectorySeparatorChar);
	if (DirectorySeparatorChar != AltDirectorySeparatorChar)
	{
		InvalidChars.PushBack(AltDirectorySeparatorChar);
	}
	return InvalidChars;
}

CString NPath::GetTempPath()
{
#ifdef _WIN32
	char TempPath[MAX_PATH];
	DWORD Result = GetTempPathA(MAX_PATH, TempPath);
	if (Result > 0 && Result < MAX_PATH)
	{
		return CString(TempPath);
	}
	return CString("C:\\Temp\\");
#else
	const char* TempDir = getenv("TMPDIR");
	if (!TempDir)
		TempDir = getenv("TMP");
	if (!TempDir)
		TempDir = getenv("TEMP");
	if (!TempDir)
		TempDir = "/tmp";

	return CString(TempDir);
#endif
}

CString NPath::GetTempFileName()
{
	static int32_t Counter = 0;
	CString TempPath = GetTempPath();

	char FileName[64];
	snprintf(FileName, sizeof(FileName), "tmp%08X.tmp", ++Counter);

	return Combine(TempPath, CString(FileName));
}

CString NPath::ChangeExtension(const CString& Path, const CString& Extension)
{
	if (Path.IsEmpty())
		return CString();

	CString WithoutExt = GetFileNameWithoutExtension(Path);
	CString Directory = GetDirectoryName(Path);

	CString Result = Directory.IsEmpty() ? WithoutExt : Combine(Directory, WithoutExt);

	if (!Extension.IsEmpty())
	{
		if (Extension.GetCStr()[0] != '.')
		{
			Result += ".";
		}
		Result += Extension;
	}

	return Result;
}

char NPath::GetDirectorySeparatorChar()
{
	return DirectorySeparatorChar;
}

CString NPath::NormalizePath(const CString& Path)
{
	if (Path.IsEmpty())
		return CString();

	// 简化实现：替换所有分隔符为标准分隔符
	CString Result = Path;
	char* Data = const_cast<char*>(Result.GetData());

	for (size_t i = 0; i < Result.GetLength(); ++i)
	{
		if (Data[i] == AltDirectorySeparatorChar)
		{
			Data[i] = DirectorySeparatorChar;
		}
	}

	return Result;
}

// === NFile 实现 ===

bool NFile::Exists(const CString& Path)
{
#ifdef _WIN32
	DWORD Attributes = GetFileAttributesA(Path.GetCStr());
	return Attributes != INVALID_FILE_ATTRIBUTES && !(Attributes & FILE_ATTRIBUTE_DIRECTORY);
#else
	struct stat FileStat;
	return stat(Path.GetCStr(), &FileStat) == 0 && S_ISREG(FileStat.st_mode);
#endif
}

TSharedPtr<NFileStream> NFile::Create(const CString& Path)
{
	return NFileStream::Create(Path, EFileMode::Create);
}

void NFile::Delete(const CString& Path)
{
	if (!TryDelete(Path))
	{
		CLogger::Error("NFile::Delete: Failed to delete file: {}", Path.GetCStr());
	}
}

bool NFile::TryDelete(const CString& Path)
{
#ifdef _WIN32
	return DeleteFileA(Path.GetCStr()) != 0;
#else
	return unlink(Path.GetCStr()) == 0;
#endif
}

void NFile::Copy(const CString& SourcePath, const CString& DestPath)
{
	Copy(SourcePath, DestPath, false);
}

void NFile::Copy(const CString& SourcePath, const CString& DestPath, bool Overwrite)
{
	if (!Exists(SourcePath))
	{
		CLogger::Error("NFile::Copy: Source file does not exist: {}", SourcePath.GetCStr());
		return;
	}

	if (Exists(DestPath) && !Overwrite)
	{
		CLogger::Error("NFile::Copy: Destination file exists and overwrite is false: {}", DestPath.GetCStr());
		return;
	}

#ifdef _WIN32
	if (!CopyFileA(SourcePath.GetCStr(), DestPath.GetCStr(), !Overwrite))
	{
		CLogger::Error("NFile::Copy: Failed to copy file from {} to {}", SourcePath.GetCStr(), DestPath.GetCStr());
	}
#else
	std::ifstream Src(SourcePath.GetCStr(), std::ios::binary);
	std::ofstream Dst(DestPath.GetCStr(), std::ios::binary);

	if (Src && Dst)
	{
		Dst << Src.rdbuf();
	}
	else
	{
		CLogger::Error("NFile::Copy: Failed to copy file from {} to {}", SourcePath.GetCStr(), DestPath.GetCStr());
	}
#endif
}

void NFile::Move(const CString& SourcePath, const CString& DestPath)
{
#ifdef _WIN32
	if (!MoveFileA(SourcePath.GetCStr(), DestPath.GetCStr()))
	{
		CLogger::Error("NFile::Move: Failed to move file from {} to {}", SourcePath.GetCStr(), DestPath.GetCStr());
	}
#else
	if (rename(SourcePath.GetCStr(), DestPath.GetCStr()) != 0)
	{
		CLogger::Error("NFile::Move: Failed to move file from {} to {}", SourcePath.GetCStr(), DestPath.GetCStr());
	}
#endif
}

CArray<uint8_t> NFile::ReadAllBytes(const CString& Path)
{
	auto Stream = NFileStream::OpenRead(Path);
	if (!Stream || !Stream->IsOpen())
	{
		CLogger::Error("NFile::ReadAllBytes: Failed to open file: {}", Path.GetCStr());
		return CArray<uint8_t>();
	}

	return Stream->ReadAllBytes();
}

CString NFile::ReadAllText(const CString& Path)
{
	auto Stream = NFileStream::OpenRead(Path);
	if (!Stream || !Stream->IsOpen())
	{
		CLogger::Error("NFile::ReadAllText: Failed to open file: {}", Path.GetCStr());
		return CString();
	}

	return Stream->ReadAllText();
}

CArray<CString> NFile::ReadAllLines(const CString& Path)
{
	CString Content = ReadAllText(Path);
	CArray<CString> Lines;

	if (Content.IsEmpty())
		return Lines;

	// 简化的行分割实现
	const char* Start = Content.GetCStr();
	const char* Current = Start;

	while (*Current)
	{
		if (*Current == '\n' || *Current == '\r')
		{
			if (Current > Start)
			{
				Lines.PushBack(CString(Start, static_cast<size_t>(Current - Start)));
			}

			// 跳过\r\n组合
			if (*Current == '\r' && *(Current + 1) == '\n')
			{
				Current += 2;
			}
			else
			{
				Current++;
			}
			Start = Current;
		}
		else
		{
			Current++;
		}
	}

	// 添加最后一行
	if (Current > Start)
	{
		Lines.PushBack(CString(Start, static_cast<size_t>(Current - Start)));
	}

	return Lines;
}

void NFile::WriteAllBytes(const CString& Path, const CArray<uint8_t>& Bytes)
{
	auto Stream = NFileStream::OpenWrite(Path);
	if (!Stream || !Stream->IsOpen())
	{
		CLogger::Error("NFile::WriteAllBytes: Failed to create file: {}", Path.GetCStr());
		return;
	}

	Stream->WriteAllBytes(Bytes);
}

void NFile::WriteAllText(const CString& Path, const CString& Contents)
{
	auto Stream = NFileStream::OpenWrite(Path);
	if (!Stream || !Stream->IsOpen())
	{
		CLogger::Error("NFile::WriteAllText: Failed to create file: {}", Path.GetCStr());
		return;
	}

	Stream->WriteAllText(Contents);
}

void NFile::WriteAllLines(const CString& Path, const CArray<CString>& Contents)
{
	auto Stream = NFileStream::OpenWrite(Path);
	if (!Stream || !Stream->IsOpen())
	{
		CLogger::Error("NFile::WriteAllLines: Failed to create file: {}", Path.GetCStr());
		return;
	}

	for (size_t i = 0; i < Contents.GetSize(); ++i)
	{
		Stream->WriteAllText(Contents[i]);
		if (i < Contents.GetSize() - 1)
		{
#ifdef _WIN32
			Stream->WriteAllText(CString("\r\n"));
#else
			Stream->WriteAllText(CString("\n"));
#endif
		}
	}
}

void NFile::AppendAllText(const CString& Path, const CString& Contents)
{
	auto Stream = NewNObject<NFileStream>(Path, EFileMode::Append, EFileAccess::Write);
	if (!Stream || !Stream->IsOpen())
	{
		CLogger::Error("NFile::AppendAllText: Failed to open file for append: {}", Path.GetCStr());
		return;
	}

	Stream->WriteAllText(Contents);
}

void NFile::AppendAllLines(const CString& Path, const CArray<CString>& Contents)
{
	auto Stream = NewNObject<NFileStream>(Path, EFileMode::Append, EFileAccess::Write);
	if (!Stream || !Stream->IsOpen())
	{
		CLogger::Error("NFile::AppendAllLines: Failed to open file for append: {}", Path.GetCStr());
		return;
	}

	for (const auto& Line : Contents)
	{
		Stream->WriteAllText(Line);
#ifdef _WIN32
		Stream->WriteAllText(CString("\r\n"));
#else
		Stream->WriteAllText(CString("\n"));
#endif
	}
}

EFileAttributes NFile::GetAttributes(const CString& Path)
{
	NFileInfo Info(Path);
	return Info.GetAttributes();
}

void NFile::SetAttributes(const CString& Path, EFileAttributes Attributes)
{
#ifdef _WIN32
	DWORD PlatformAttribs = ConvertAttributesToPlatform(Attributes);
	if (!SetFileAttributesA(Path.GetCStr(), PlatformAttribs))
	{
		CLogger::Error("NFile::SetAttributes: Failed to set attributes for: {}", Path.GetCStr());
	}
#else
	mode_t Mode = ConvertAttributesToPlatform(Attributes);
	if (chmod(Path.GetCStr(), Mode) != 0)
	{
		CLogger::Error("NFile::SetAttributes: Failed to set attributes for: {}", Path.GetCStr());
	}
#endif
}

NDateTime NFile::GetCreationTime(const CString& Path)
{
	NFileInfo Info(Path);
	return Info.GetCreationTime();
}

NDateTime NFile::GetLastAccessTime(const CString& Path)
{
	NFileInfo Info(Path);
	return Info.GetLastAccessTime();
}

NDateTime NFile::GetLastWriteTime(const CString& Path)
{
	NFileInfo Info(Path);
	return Info.GetLastWriteTime();
}

void NFile::SetCreationTime(const CString& Path, const NDateTime& Time)
{
	// 平台特定实现，这里简化
	CLogger::Warning("NFile::SetCreationTime: Not fully implemented");
}

void NFile::SetLastAccessTime(const CString& Path, const NDateTime& Time)
{
	// 平台特定实现，这里简化
	CLogger::Warning("NFile::SetLastAccessTime: Not fully implemented");
}

void NFile::SetLastWriteTime(const CString& Path, const NDateTime& Time)
{
	// 平台特定实现，这里简化
	CLogger::Warning("NFile::SetLastWriteTime: Not fully implemented");
}

// === NDirectory 实现 ===

bool NDirectory::Exists(const CString& Path)
{
#ifdef _WIN32
	DWORD Attributes = GetFileAttributesA(Path.GetCStr());
	return Attributes != INVALID_FILE_ATTRIBUTES && (Attributes & FILE_ATTRIBUTE_DIRECTORY);
#else
	struct stat DirStat;
	return stat(Path.GetCStr(), &DirStat) == 0 && S_ISDIR(DirStat.st_mode);
#endif
}

NDirectoryInfo NDirectory::CreateDirectory(const CString& Path)
{
	if (Exists(Path))
	{
		return NDirectoryInfo(Path);
	}

	// 递归创建父目录
	CString Parent = NPath::GetDirectoryName(Path);
	if (!Parent.IsEmpty() && !Exists(Parent))
	{
		CreateDirectory(Parent);
	}

#ifdef _WIN32
	if (!CreateDirectoryA(Path.GetCStr(), nullptr))
	{
		CLogger::Error("NDirectory::CreateDirectory: Failed to create directory: {}", Path.GetCStr());
	}
#else
	if (mkdir(Path.GetCStr(), 0755) != 0 && errno != EEXIST)
	{
		CLogger::Error(
		    "NDirectory::CreateDirectory: Failed to create directory: {} ({})", Path.GetCStr(), strerror(errno));
	}
#endif

	return NDirectoryInfo(Path);
}

void NDirectory::Delete(const CString& Path)
{
	Delete(Path, false);
}

void NDirectory::Delete(const CString& Path, bool Recursive)
{
	if (!Exists(Path))
	{
		return;
	}

	if (Recursive)
	{
		// 递归删除所有内容
		CArray<CString> Files = GetFiles(Path);
		for (const auto& File : Files)
		{
			NFile::Delete(File);
		}

		CArray<CString> SubDirs = GetDirectories(Path);
		for (const auto& SubDir : SubDirs)
		{
			Delete(SubDir, true);
		}
	}

#ifdef _WIN32
	if (!RemoveDirectoryA(Path.GetCStr()))
	{
		CLogger::Error("NDirectory::Delete: Failed to delete directory: {}", Path.GetCStr());
	}
#else
	if (rmdir(Path.GetCStr()) != 0)
	{
		CLogger::Error("NDirectory::Delete: Failed to delete directory: {} ({})", Path.GetCStr(), strerror(errno));
	}
#endif
}

void NDirectory::Move(const CString& SourcePath, const CString& DestPath)
{
#ifdef _WIN32
	if (!MoveFileA(SourcePath.GetCStr(), DestPath.GetCStr()))
	{
		CLogger::Error(
		    "NDirectory::Move: Failed to move directory from {} to {}", SourcePath.GetCStr(), DestPath.GetCStr());
	}
#else
	if (rename(SourcePath.GetCStr(), DestPath.GetCStr()) != 0)
	{
		CLogger::Error(
		    "NDirectory::Move: Failed to move directory from {} to {}", SourcePath.GetCStr(), DestPath.GetCStr());
	}
#endif
}

CArray<CString> NDirectory::GetFiles(const CString& Path)
{
	return GetFiles(Path, CString("*"));
}

CArray<CString> NDirectory::GetFiles(const CString& Path, const CString& SearchPattern)
{
	return GetFiles(Path, SearchPattern, ESearchOption::TopDirectoryOnly);
}

CArray<CString> NDirectory::GetFiles(const CString& Path, const CString& SearchPattern, ESearchOption SearchOption)
{
	CArray<CString> Result;

	auto Visitor = [&Result, &SearchPattern](const CString& ItemPath, bool IsDirectory) -> bool {
		if (!IsDirectory)
		{
			CString FileName = NPath::GetFileName(ItemPath);
			if (MatchWildcard(SearchPattern, FileName))
			{
				Result.PushBack(ItemPath);
			}
		}
		return true;
	};

	EnumerateFileSystemEntries(Path, Visitor, SearchOption == ESearchOption::AllDirectories);
	return Result;
}

CArray<CString> NDirectory::GetDirectories(const CString& Path)
{
	return GetDirectories(Path, CString("*"));
}

CArray<CString> NDirectory::GetDirectories(const CString& Path, const CString& SearchPattern)
{
	return GetDirectories(Path, SearchPattern, ESearchOption::TopDirectoryOnly);
}

CArray<CString> NDirectory::GetDirectories(const CString& Path,
                                           const CString& SearchPattern,
                                           ESearchOption SearchOption)
{
	CArray<CString> Result;

	auto Visitor = [&Result, &SearchPattern](const CString& ItemPath, bool IsDirectory) -> bool {
		if (IsDirectory)
		{
			CString DirName = NPath::GetFileName(ItemPath);
			if (MatchWildcard(SearchPattern, DirName))
			{
				Result.PushBack(ItemPath);
			}
		}
		return true;
	};

	EnumerateFileSystemEntries(Path, Visitor, SearchOption == ESearchOption::AllDirectories);
	return Result;
}

CArray<CString> NDirectory::GetFileSystemEntries(const CString& Path)
{
	return GetFileSystemEntries(Path, CString("*"));
}

CArray<CString> NDirectory::GetFileSystemEntries(const CString& Path, const CString& SearchPattern)
{
	CArray<CString> Result;

	auto Visitor = [&Result, &SearchPattern](const CString& ItemPath, bool IsDirectory) -> bool {
		CString ItemName = NPath::GetFileName(ItemPath);
		if (MatchWildcard(SearchPattern, ItemName))
		{
			Result.PushBack(ItemPath);
		}
		return true;
	};

	EnumerateFileSystemEntries(Path, Visitor, false);
	return Result;
}

CString NDirectory::GetCurrentDirectory()
{
#ifdef _WIN32
	char CurrentDir[MAX_PATH];
	DWORD Result = GetCurrentDirectoryA(MAX_PATH, CurrentDir);
	if (Result > 0 && Result < MAX_PATH)
	{
		return CString(CurrentDir);
	}
	return CString();
#else
	char CurrentDir[PATH_MAX];
	if (getcwd(CurrentDir, PATH_MAX))
	{
		return CString(CurrentDir);
	}
	return CString();
#endif
}

void NDirectory::SetCurrentDirectory(const CString& Path)
{
#ifdef _WIN32
	if (!SetCurrentDirectoryA(Path.GetCStr()))
	{
		CLogger::Error("NDirectory::SetCurrentDirectory: Failed to set current directory to: {}", Path.GetCStr());
	}
#else
	if (chdir(Path.GetCStr()) != 0)
	{
		CLogger::Error("NDirectory::SetCurrentDirectory: Failed to set current directory to: {} ({})",
		               Path.GetCStr(),
		               strerror(errno));
	}
#endif
}

CString NDirectory::GetDirectoryRoot(const CString& Path)
{
#ifdef _WIN32
	if (Path.GetLength() >= 2 && Path.GetCStr()[1] == ':')
	{
		return CString(Path.GetCStr(), 3); // "C:\\"
	}
	return CString("\\");
#else
	return CString("/");
#endif
}

CArray<CString> NDirectory::GetLogicalDrives()
{
	CArray<CString> Drives;

#ifdef _WIN32
	DWORD DrivesMask = GetLogicalDrives();
	for (char Drive = 'A'; Drive <= 'Z'; ++Drive)
	{
		if (DrivesMask & (1 << (Drive - 'A')))
		{
			char DriveStr[4] = {Drive, ':', '\\', '\0'};
			Drives.PushBack(CString(DriveStr));
		}
	}
#else
	// Unix/Linux系统通常只有根文件系统
	Drives.PushBack(CString("/"));
#endif

	return Drives;
}

void NDirectory::EnumerateFileSystemEntries(const CString& Path, const DirectoryVisitor& Visitor, bool Recursive)
{
	if (!Exists(Path))
	{
		return;
	}

#ifdef _WIN32
	CString SearchPath = NPath::Combine(Path, CString("*"));
	WIN32_FIND_DATAA FindData;
	HANDLE FindHandle = FindFirstFileA(SearchPath.GetCStr(), &FindData);

	if (FindHandle != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (strcmp(FindData.cFileName, ".") == 0 || strcmp(FindData.cFileName, "..") == 0)
				continue;

			CString ItemPath = NPath::Combine(Path, CString(FindData.cFileName));
			bool IsDirectory = (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

			if (!Visitor(ItemPath, IsDirectory))
				break;

			if (Recursive && IsDirectory)
			{
				EnumerateFileSystemEntries(ItemPath, Visitor, true);
			}
		} while (FindNextFileA(FindHandle, &FindData));

		FindClose(FindHandle);
	}
#else
	DIR* Dir = opendir(Path.GetCStr());
	if (Dir)
	{
		struct dirent* Entry;
		while ((Entry = readdir(Dir)) != nullptr)
		{
			if (strcmp(Entry->d_name, ".") == 0 || strcmp(Entry->d_name, "..") == 0)
				continue;

			CString ItemPath = NPath::Combine(Path, CString(Entry->d_name));
			bool IsDirectory = (Entry->d_type == DT_DIR);

			// 如果d_type不可用，使用stat
			if (Entry->d_type == DT_UNKNOWN)
			{
				struct stat ItemStat;
				if (stat(ItemPath.GetCStr(), &ItemStat) == 0)
				{
					IsDirectory = S_ISDIR(ItemStat.st_mode);
				}
			}

			if (!Visitor(ItemPath, IsDirectory))
				break;

			if (Recursive && IsDirectory)
			{
				EnumerateFileSystemEntries(ItemPath, Visitor, true);
			}
		}

		closedir(Dir);
	}
#endif
}

} // namespace NLib