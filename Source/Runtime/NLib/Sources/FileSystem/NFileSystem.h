#pragma once

#include "Containers/CArray.h"
#include "Containers/CString.h"
#include "Core/CObject.h"
#include "DateTime/NDateTime.h"

#include <cstdint>
#include <functional>

namespace NLib
{

/**
 * @brief 文件属性枚举
 */
enum class EFileAttributes : uint32_t
{
	None = 0,
	ReadOnly = 1 << 0,
	Hidden = 1 << 1,
	System = 1 << 2,
	Directory = 1 << 3,
	Archive = 1 << 4,
	Device = 1 << 5,
	Normal = 1 << 6,
	Temporary = 1 << 7,
	SparseFile = 1 << 8,
	ReparsePoint = 1 << 9,
	Compressed = 1 << 10,
	Offline = 1 << 11,
	NotContentIndexed = 1 << 12,
	Encrypted = 1 << 13
};

/**
 * @brief 文件访问模式
 */
enum class EFileAccess : uint32_t
{
	Read = 1 << 0,
	Write = 1 << 1,
	ReadWrite = Read | Write,
	Execute = 1 << 2
};

/**
 * @brief 文件共享模式
 */
enum class EFileShare : uint32_t
{
	None = 0,
	Read = 1 << 0,
	Write = 1 << 1,
	Delete = 1 << 2,
	ReadWrite = Read | Write,
	All = Read | Write | Delete
};

/**
 * @brief 文件创建模式
 */
enum class EFileMode : uint32_t
{
	CreateNew,    // 创建新文件，如果存在则失败
	Create,       // 创建新文件，如果存在则覆盖
	Open,         // 打开现有文件，如果不存在则失败
	OpenOrCreate, // 打开现有文件，如果不存在则创建
	Truncate,     // 打开现有文件并截断为0字节
	Append        // 打开现有文件并定位到末尾
};

/**
 * @brief 文件搜索选项
 */
enum class ESearchOption : uint32_t
{
	TopDirectoryOnly, // 仅搜索顶级目录
	AllDirectories    // 递归搜索所有子目录
};

/**
 * @brief 文件信息结构
 */
class LIBNLIB_API NFileInfo : public CObject
{
	NCLASS()
class NFileInfo : public NObject
{
    GENERATED_BODY()

public:
	NFileInfo();
	NFileInfo(const CString& FilePath);

	// 基本属性
	CString GetFullName() const
	{
		return FullPath;
	}
	CString GetName() const;
	CString GetDirectoryName() const;
	CString GetExtension() const;

	// 时间属性
	NDateTime GetCreationTime() const
	{
		return CreationTime;
	}
	NDateTime GetLastAccessTime() const
	{
		return LastAccessTime;
	}
	NDateTime GetLastWriteTime() const
	{
		return LastWriteTime;
	}

	// 大小和属性
	int64_t GetLength() const
	{
		return Length;
	}
	EFileAttributes GetAttributes() const
	{
		return Attributes;
	}

	// 状态查询
	bool Exists() const;
	bool IsDirectory() const;
	bool IsFile() const;
	bool IsReadOnly() const;
	bool IsHidden() const;

	// 操作
	void Refresh();
	void Delete();
	void MoveTo(const CString& DestPath);
	void CopyTo(const CString& DestPath, bool Overwrite = false);

	virtual CString ToString() const override;

private:
	CString FullPath;
	NDateTime CreationTime;
	NDateTime LastAccessTime;
	NDateTime LastWriteTime;
	int64_t Length;
	EFileAttributes Attributes;

	void LoadFileInfo();
};

/**
 * @brief 目录信息结构
 */
class LIBNLIB_API NDirectoryInfo : public CObject
{
	NCLASS()
class NDirectoryInfo : public NObject
{
    GENERATED_BODY()

public:
	NDirectoryInfo();
	explicit NDirectoryInfo(const CString& DirectoryPath);

	// 基本属性
	CString GetFullName() const
	{
		return FullPath;
	}
	CString GetName() const;
	CString GetParent() const;

	// 状态查询
	bool Exists() const;

	// 操作
	void Create();
	void Delete(bool Recursive = false);
	void MoveTo(const CString& DestPath);

	// 枚举
	CArray<NFileInfo> GetFiles() const;
	CArray<NFileInfo> GetFiles(const CString& SearchPattern) const;
	CArray<NFileInfo> GetFiles(const CString& SearchPattern, ESearchOption SearchOption) const;

	CArray<NDirectoryInfo> GetDirectories() const;
	CArray<NDirectoryInfo> GetDirectories(const CString& SearchPattern) const;
	CArray<NDirectoryInfo> GetDirectories(const CString& SearchPattern, ESearchOption SearchOption) const;

	virtual CString ToString() const override;

private:
	CString FullPath;

	void ValidatePath() const;
};

/**
 * @brief 文件流基类
 */
class LIBNLIB_API NStream : public CObject
{
	NCLASS()
class NStream : public NObject
{
    GENERATED_BODY()

public:
	virtual ~NStream() = default;

	// 基本属性
	virtual bool CanRead() const = 0;
	virtual bool CanWrite() const = 0;
	virtual bool CanSeek() const = 0;
	virtual int64_t GetLength() const = 0;
	virtual int64_t GetPosition() const = 0;
	virtual void SetPosition(int64_t Position) = 0;

	// 基本操作
	virtual void Close() = 0;
	virtual void Flush() = 0;

	// 读取操作
	virtual int32_t ReadByte() = 0;
	virtual int32_t Read(uint8_t* Buffer, int32_t Offset, int32_t Count) = 0;
	virtual CArray<uint8_t> ReadAllBytes();
	virtual CString ReadAllText();

	// 写入操作
	virtual void WriteByte(uint8_t Value) = 0;
	virtual void Write(const uint8_t* Buffer, int32_t Offset, int32_t Count) = 0;
	virtual void WriteAllBytes(const CArray<uint8_t>& Data);
	virtual void WriteAllText(const CString& Text);

	// 定位操作
	virtual int64_t Seek(int64_t Offset, int32_t Origin) = 0;
	virtual void SetLength(int64_t Length) = 0;
};

/**
 * @brief 文件流实现
 */
class LIBNLIB_API NFileStream : public NStream
{
	NCLASS()
class NFileStream : public NStream
{
    GENERATED_BODY()

public:
	NFileStream();
	NFileStream(const CString& FilePath, EFileMode Mode);
	NFileStream(const CString& FilePath, EFileMode Mode, EFileAccess Access);
	NFileStream(const CString& FilePath, EFileMode Mode, EFileAccess Access, EFileShare Share);
	virtual ~NFileStream();

	// 工厂方法
	static TSharedPtr<NFileStream> Create(const CString& FilePath, EFileMode Mode);
	static TSharedPtr<NFileStream> OpenRead(const CString& FilePath);
	static TSharedPtr<NFileStream> OpenWrite(const CString& FilePath);
	static TSharedPtr<NFileStream> CreateText(const CString& FilePath);

	// Stream接口实现
	virtual bool CanRead() const override;
	virtual bool CanWrite() const override;
	virtual bool CanSeek() const override;
	virtual int64_t GetLength() const override;
	virtual int64_t GetPosition() const override;
	virtual void SetPosition(int64_t Position) override;

	virtual void Close() override;
	virtual void Flush() override;

	virtual int32_t ReadByte() override;
	virtual int32_t Read(uint8_t* Buffer, int32_t Offset, int32_t Count) override;

	virtual void WriteByte(uint8_t Value) override;
	virtual void Write(const uint8_t* Buffer, int32_t Offset, int32_t Count) override;

	virtual int64_t Seek(int64_t Offset, int32_t Origin) override;
	virtual void SetLength(int64_t Length) override;

	// 文件特定方法
	CString GetName() const
	{
		return FilePath;
	}
	bool IsOpen() const
	{
		return FileHandle != nullptr;
	}

private:
	CString FilePath;
	EFileMode Mode;
	EFileAccess Access;
	EFileShare Share;
	void* FileHandle; // 平台特定的文件句柄

	void OpenFile();
	void CloseFile();
};

/**
 * @brief 路径工具类
 */
class LIBNLIB_API NPath : public CObject
{
	NCLASS()
class NPath : public NObject
{
    GENERATED_BODY()

public:
	// 路径分隔符
	static const char DirectorySeparatorChar;
	static const char AltDirectorySeparatorChar;
	static const char PathSeparator;
	static const char VolumeSeparatorChar;

	// 路径操作
	static CString Combine(const CString& Path1, const CString& Path2);
	static CString Combine(const CString& Path1, const CString& Path2, const CString& Path3);
	static CString Combine(const CArray<CString>& Paths);

	static CString GetDirectoryName(const CString& Path);
	static CString GetFileName(const CString& Path);
	static CString GetFileNameWithoutExtension(const CString& Path);
	static CString GetExtension(const CString& Path);
	static CString GetFullPath(const CString& Path);
	static CString GetRelativePath(const CString& RelativeTo, const CString& Path);

	// 路径验证
	static bool IsPathRooted(const CString& Path);
	static bool HasExtension(const CString& Path);
	static CArray<char> GetInvalidPathChars();
	static CArray<char> GetInvalidFileNameChars();

	// 路径规范化
	static CString GetTempPath();
	static CString GetTempFileName();
	static CString ChangeExtension(const CString& Path, const CString& Extension);

	// 平台相关
	static char GetDirectorySeparatorChar();
	static CString NormalizePath(const CString& Path);

private:
	NPath() = default; // 静态类，不允许实例化
};

/**
 * @brief 文件系统静态工具类
 */
class LIBNLIB_API NFile : public CObject
{
	NCLASS()
class NFile : public NObject
{
    GENERATED_BODY()

public:
	// 文件存在性检查
	static bool Exists(const CString& Path);

	// 文件创建和删除
	static TSharedPtr<NFileStream> Create(const CString& Path);
	static void Delete(const CString& Path);
	static bool TryDelete(const CString& Path);

	// 文件复制和移动
	static void Copy(const CString& SourcePath, const CString& DestPath);
	static void Copy(const CString& SourcePath, const CString& DestPath, bool Overwrite);
	static void Move(const CString& SourcePath, const CString& DestPath);

	// 文件读取
	static CArray<uint8_t> ReadAllBytes(const CString& Path);
	static CString ReadAllText(const CString& Path);
	static CArray<CString> ReadAllLines(const CString& Path);

	// 文件写入
	static void WriteAllBytes(const CString& Path, const CArray<uint8_t>& Bytes);
	static void WriteAllText(const CString& Path, const CString& Contents);
	static void WriteAllLines(const CString& Path, const CArray<CString>& Contents);

	// 文件追加
	static void AppendAllText(const CString& Path, const CString& Contents);
	static void AppendAllLines(const CString& Path, const CArray<CString>& Contents);

	// 文件属性
	static EFileAttributes GetAttributes(const CString& Path);
	static void SetAttributes(const CString& Path, EFileAttributes Attributes);

	static NDateTime GetCreationTime(const CString& Path);
	static NDateTime GetLastAccessTime(const CString& Path);
	static NDateTime GetLastWriteTime(const CString& Path);

	static void SetCreationTime(const CString& Path, const NDateTime& Time);
	static void SetLastAccessTime(const CString& Path, const NDateTime& Time);
	static void SetLastWriteTime(const CString& Path, const NDateTime& Time);

private:
	NFile() = default; // 静态类，不允许实例化
};

/**
 * @brief 目录系统静态工具类
 */
class LIBNLIB_API NDirectory : public CObject
{
	NCLASS()
class NDirectory : public NObject
{
    GENERATED_BODY()

public:
	// 目录存在性检查
	static bool Exists(const CString& Path);

	// 目录创建和删除
	static NDirectoryInfo CreateDirectory(const CString& Path);
	static void Delete(const CString& Path);
	static void Delete(const CString& Path, bool Recursive);

	// 目录移动
	static void Move(const CString& SourcePath, const CString& DestPath);

	// 目录枚举
	static CArray<CString> GetFiles(const CString& Path);
	static CArray<CString> GetFiles(const CString& Path, const CString& SearchPattern);
	static CArray<CString> GetFiles(const CString& Path, const CString& SearchPattern, ESearchOption SearchOption);

	static CArray<CString> GetDirectories(const CString& Path);
	static CArray<CString> GetDirectories(const CString& Path, const CString& SearchPattern);
	static CArray<CString> GetDirectories(const CString& Path,
	                                      const CString& SearchPattern,
	                                      ESearchOption SearchOption);

	static CArray<CString> GetFileSystemEntries(const CString& Path);
	static CArray<CString> GetFileSystemEntries(const CString& Path, const CString& SearchPattern);

	// 特殊目录
	static CString GetCurrentDirectory();
	static void SetCurrentDirectory(const CString& Path);

	static CString GetDirectoryRoot(const CString& Path);
	static CArray<CString> GetLogicalDrives();

	// 目录遍历回调
	using DirectoryVisitor = std::function<bool(const CString& Path, bool IsDirectory)>;
	static void EnumerateFileSystemEntries(const CString& Path,
	                                       const DirectoryVisitor& Visitor,
	                                       bool Recursive = false);

private:
	NDirectory() = default; // 静态类，不允许实例化
};

} // namespace NLib