#pragma once

#include "Containers/TArray.h"
#include "Core/Object.h"
#include "Core/SmartPointers.h"
#include "Events/Delegate.h"
#include "Path.h"
#include "Time/TimeTypes.h"

#include <filesystem>

#include "FileSystem.generate.h"

namespace NLib
{
/**
 * @brief 文件类型枚举
 */
enum class EFileType : uint8_t
{
	Unknown,      // 未知类型
	Regular,      // 普通文件
	Directory,    // 目录
	SymbolicLink, // 符号链接
	BlockDevice,  // 块设备
	CharDevice,   // 字符设备
	Fifo,         // 命名管道
	Socket        // Socket文件
};

/**
 * @brief 文件权限枚举
 */
enum class EFilePermissions : uint32_t
{
	None = 0,

	// 所有者权限
	OwnerRead = 0400,
	OwnerWrite = 0200,
	OwnerExec = 0100,
	OwnerAll = OwnerRead | OwnerWrite | OwnerExec,

	// 组权限
	GroupRead = 0040,
	GroupWrite = 0020,
	GroupExec = 0010,
	GroupAll = GroupRead | GroupWrite | GroupExec,

	// 其他用户权限
	OthersRead = 0004,
	OthersWrite = 0002,
	OthersExec = 0001,
	OthersAll = OthersRead | OthersWrite | OthersExec,

	// 常用组合
	All = OwnerAll | GroupAll | OthersAll,
	DefaultFile = OwnerRead | OwnerWrite | GroupRead | OthersRead,
	DefaultDirectory = OwnerAll | GroupRead | GroupExec | OthersRead | OthersExec
};

ENUM_CLASS_FLAGS(EFilePermissions)

/**
 * @brief 文件状态信息
 */
struct SFileStatus
{
	NPath Path;                                            // 文件路径
	EFileType Type = EFileType::Unknown;                   // 文件类型
	uint64_t Size = 0;                                     // 文件大小（字节）
	EFilePermissions Permissions = EFilePermissions::None; // 文件权限
	CDateTime CreationTime;                                // 创建时间
	CDateTime LastWriteTime;                               // 最后修改时间
	CDateTime LastAccessTime;                              // 最后访问时间
	bool bExists = false;                                  // 是否存在
	bool bIsReadOnly = false;                              // 是否只读
	bool bIsHidden = false;                                // 是否隐藏

	SFileStatus() = default;
	explicit SFileStatus(const NPath& InPath)
	    : Path(InPath)
	{}

	bool IsFile() const
	{
		return Type == EFileType::Regular;
	}
	bool IsDirectory() const
	{
		return Type == EFileType::Directory;
	}
	bool IsSymbolicLink() const
	{
		return Type == EFileType::SymbolicLink;
	}
};

/**
 * @brief 目录遍历选项
 */
struct SDirectoryIterationOptions
{
	bool bRecursive = false;         // 是否递归遍历子目录
	bool bIncludeDirectories = true; // 是否包含目录
	bool bIncludeFiles = true;       // 是否包含文件
	bool bIncludeHidden = false;     // 是否包含隐藏文件
	bool bFollowSymlinks = false;    // 是否跟随符号链接
	TString Pattern;                 // 文件名模式匹配（支持通配符）
	int32_t MaxDepth = -1;           // 最大递归深度（-1为无限制）

	SDirectoryIterationOptions() = default;
};

/**
 * @brief 文件复制选项
 */
enum class EFileCopyOptions : uint32_t
{
	None = 0,
	SkipExisting = 1,        // 跳过已存在的文件
	OverwriteExisting = 2,   // 覆盖已存在的文件
	UpdateExisting = 4,      // 只有当源文件更新时才覆盖
	Recursive = 8,           // 递归复制目录
	CopySymlinks = 16,       // 复制符号链接而不是目标文件
	PreserveAttributes = 32, // 保留文件属性
	PreservePermissions = 64 // 保留文件权限
};

ENUM_CLASS_FLAGS(EFileCopyOptions)

/**
 * @brief 文件系统操作结果
 */
struct SFileSystemResult
{
	bool bSuccess = false;
	TString ErrorMessage;
	int32_t ErrorCode = 0;

	SFileSystemResult() = default;
	explicit SFileSystemResult(bool bInSuccess)
	    : bSuccess(bInSuccess)
	{}
	SFileSystemResult(bool bInSuccess, const TString& InErrorMessage, int32_t InErrorCode = 0)
	    : bSuccess(bInSuccess),
	      ErrorMessage(InErrorMessage),
	      ErrorCode(InErrorCode)
	{}

	operator bool() const
	{
		return bSuccess;
	}

	TString ToString() const
	{
		if (bSuccess)
		{
			return TString("Success");
		}
		else
		{
			return TString("Error: ") + ErrorMessage +
			       (ErrorCode != 0 ? (TString(" (Code: ") + TString::FromInt(ErrorCode) + TString(")")) : TString());
		}
	}
};

/**
 * @brief 文件监控事件类型
 */
enum class EFileWatchEvent : uint8_t
{
	Created,         // 文件创建
	Modified,        // 文件修改
	Deleted,         // 文件删除
	Renamed,         // 文件重命名
	AttributeChanged // 文件属性变更
};

/**
 * @brief 文件监控事件信息
 */
struct SFileWatchEventInfo
{
	EFileWatchEvent EventType;
	NPath FilePath;
	NPath OldPath; // 重命名事件的旧路径
	CDateTime Timestamp;

	SFileWatchEventInfo() = default;
	SFileWatchEventInfo(EFileWatchEvent InEventType, const NPath& InFilePath)
	    : EventType(InEventType),
	      FilePath(InFilePath),
	      Timestamp(CDateTime::Now())
	{}
};

/**
 * @brief 文件系统工具类
 *
 * 提供完整的文件系统操作功能：
 * - 文件和目录的创建、删除、移动、复制
 * - 文件状态查询和属性修改
 * - 目录遍历和搜索
 * - 文件权限管理
 * - 符号链接操作
 * - 文件监控
 */
NCLASS()
class NFileSystem : public NObject
{
	GENERATED_BODY()

public:
	// === 委托定义 ===
	DECLARE_MULTICAST_DELEGATE(FOnFileWatchEvent, const SFileWatchEventInfo&);

public:
	// === 文件存在性检查 ===

	/**
	 * @brief 检查文件或目录是否存在
	 */
	static bool Exists(const NPath& Path);

	/**
	 * @brief 检查是否是文件
	 */
	static bool IsFile(const NPath& Path);

	/**
	 * @brief 检查是否是目录
	 */
	static bool IsDirectory(const NPath& Path);

	/**
	 * @brief 检查是否是符号链接
	 */
	static bool IsSymbolicLink(const NPath& Path);

	/**
	 * @brief 检查文件是否为空
	 */
	static bool IsEmpty(const NPath& Path);

public:
	// === 文件状态查询 ===

	/**
	 * @brief 获取文件状态信息
	 */
	static SFileStatus GetFileStatus(const NPath& Path);

	/**
	 * @brief 获取文件大小
	 */
	static uint64_t GetFileSize(const NPath& Path);

	/**
	 * @brief 获取文件最后修改时间
	 */
	static CDateTime GetLastWriteTime(const NPath& Path);

	/**
	 * @brief 获取文件创建时间
	 */
	static CDateTime GetCreationTime(const NPath& Path);

	/**
	 * @brief 获取文件权限
	 */
	static EFilePermissions GetPermissions(const NPath& Path);

public:
	// === 文件和目录创建 ===

	/**
	 * @brief 创建目录
	 */
	static SFileSystemResult CreateDirectory(const NPath& Path, bool bCreateParents = true);

	/**
	 * @brief 创建文件
	 */
	static SFileSystemResult CreateFile(const NPath& Path, bool bOverwrite = false);

	/**
	 * @brief 创建符号链接
	 */
	static SFileSystemResult CreateSymbolicLink(const NPath& LinkPath, const NPath& TargetPath);

	/**
	 * @brief 创建硬链接
	 */
	static SFileSystemResult CreateHardLink(const NPath& LinkPath, const NPath& TargetPath);

public:
	// === 文件和目录删除 ===

	/**
	 * @brief 删除文件
	 */
	static SFileSystemResult DeleteFile(const NPath& Path);

	/**
	 * @brief 删除目录
	 */
	static SFileSystemResult DeleteDirectory(const NPath& Path, bool bRecursive = false);

	/**
	 * @brief 删除文件或目录
	 */
	static SFileSystemResult Delete(const NPath& Path, bool bRecursive = false);

public:
	// === 文件和目录移动/重命名 ===

	/**
	 * @brief 移动/重命名文件或目录
	 */
	static SFileSystemResult Move(const NPath& SourcePath, const NPath& DestinationPath);

	/**
	 * @brief 重命名文件或目录
	 */
	static SFileSystemResult Rename(const NPath& Path, const TString& NewName);

public:
	// === 文件和目录复制 ===

	/**
	 * @brief 复制文件
	 */
	static SFileSystemResult CopyFile(const NPath& SourcePath,
	                                  const NPath& DestinationPath,
	                                  EFileCopyOptions Options = EFileCopyOptions::None);

	/**
	 * @brief 复制目录
	 */
	static SFileSystemResult CopyDirectory(const NPath& SourcePath,
	                                       const NPath& DestinationPath,
	                                       EFileCopyOptions Options = EFileCopyOptions::Recursive);

public:
	// === 目录遍历 ===

	/**
	 * @brief 列出目录内容
	 */
	static TArray<NPath, CMemoryManager> ListDirectory(
	    const NPath& DirectoryPath, const SDirectoryIterationOptions& Options = SDirectoryIterationOptions{});

	/**
	 * @brief 查找文件
	 */
	static TArray<NPath, CMemoryManager> FindFiles(const NPath& DirectoryPath,
	                                               const TString& Pattern,
	                                               bool bRecursive = false);

	/**
	 * @brief 查找目录
	 */
	static TArray<NPath, CMemoryManager> FindDirectories(const NPath& DirectoryPath,
	                                                     const TString& Pattern,
	                                                     bool bRecursive = false);

public:
	// === 文件权限和属性 ===

	/**
	 * @brief 设置文件权限
	 */
	static SFileSystemResult SetPermissions(const NPath& Path, EFilePermissions Permissions);

	/**
	 * @brief 设置文件只读属性
	 */
	static SFileSystemResult SetReadOnly(const NPath& Path, bool bReadOnly);

	/**
	 * @brief 设置文件隐藏属性
	 */
	static SFileSystemResult SetHidden(const NPath& Path, bool bHidden);

	/**
	 * @brief 设置文件时间
	 */
	static SFileSystemResult SetFileTime(const NPath& Path,
	                                     const CDateTime& LastWriteTime,
	                                     const CDateTime& LastAccessTime = CDateTime::Now());

public:
	// === 符号链接操作 ===

	/**
	 * @brief 读取符号链接目标
	 */
	static NPath ReadSymbolicLink(const NPath& LinkPath);

	/**
	 * @brief 解析符号链接的最终目标
	 */
	static NPath ResolveSymbolicLink(const NPath& LinkPath);

public:
	// === 路径操作 ===

	/**
	 * @brief 获取绝对路径
	 */
	static NPath GetAbsolutePath(const NPath& Path);

	/**
	 * @brief 获取相对路径
	 */
	static NPath GetRelativePath(const NPath& Path, const NPath& BasePath);

	/**
	 * @brief 规范化路径
	 */
	static NPath CanonicalizePath(const NPath& Path);

public:
	// === 磁盘空间查询 ===

	/**
	 * @brief 获取磁盘空间信息
	 */
	struct SDiskSpaceInfo
	{
		uint64_t Capacity = 0;  // 总容量
		uint64_t Free = 0;      // 可用空间
		uint64_t Available = 0; // 当前用户可用空间

		SDiskSpaceInfo() = default;
	};

	static SDiskSpaceInfo GetDiskSpaceInfo(const NPath& Path);

public:
	// === 临时文件操作 ===

	/**
	 * @brief 创建临时文件
	 */
	static NPath CreateTempFile(const TString& Prefix = TString("temp"), const TString& Extension = TString(".tmp"));

	/**
	 * @brief 创建临时目录
	 */
	static NPath CreateTempDirectory(const TString& Prefix = TString("temp"));

public:
	// === 文件内容快速操作 ===

	/**
	 * @brief 读取整个文件到字符串
	 */
	static TString ReadAllText(const NPath& Path);

	/**
	 * @brief 读取整个文件到字节数组
	 */
	static TArray<uint8_t, CMemoryManager> ReadAllBytes(const NPath& Path);

	/**
	 * @brief 写入字符串到文件
	 */
	static SFileSystemResult WriteAllText(const NPath& Path, const TString& Content, bool bOverwrite = true);

	/**
	 * @brief 写入字节数组到文件
	 */
	static SFileSystemResult WriteAllBytes(const NPath& Path,
	                                       const TArray<uint8_t, CMemoryManager>& Data,
	                                       bool bOverwrite = true);

	/**
	 * @brief 追加文本到文件
	 */
	static SFileSystemResult AppendAllText(const NPath& Path, const TString& Content);

public:
	// === 文件监控 ===

	/**
	 * @brief 开始监控目录
	 */
	static bool StartWatchingDirectory(const NPath& DirectoryPath, bool bRecursive = true);

	/**
	 * @brief 停止监控目录
	 */
	static void StopWatchingDirectory(const NPath& DirectoryPath);

	/**
	 * @brief 停止所有文件监控
	 */
	static void StopAllWatching();

	/**
	 * @brief 文件监控事件
	 */
	static FOnFileWatchEvent OnFileWatchEvent;

public:
	// === 工具函数 ===

	/**
	 * @brief 匹配文件名模式（支持*和?通配符）
	 */
	static bool MatchPattern(const TString& FileName, const TString& Pattern);

	/**
	 * @brief 计算目录大小
	 */
	static uint64_t CalculateDirectorySize(const NPath& DirectoryPath, bool bRecursive = true);

	/**
	 * @brief 计算文件或目录的校验和
	 */
	static TString CalculateChecksum(const NPath& Path, const TString& Algorithm = TString("MD5"));

private:
	// === 内部实现 ===

	/**
	 * @brief 从std::filesystem::file_status转换文件类型
	 */
	static EFileType ConvertFileType(const std::filesystem::file_status& Status);

	/**
	 * @brief 从std::filesystem::perms转换权限
	 */
	static EFilePermissions ConvertPermissions(std::filesystem::perms Perms);

	/**
	 * @brief 转换为std::filesystem::perms
	 */
	static std::filesystem::perms ConvertToStdPermissions(EFilePermissions Permissions);

	/**
	 * @brief 转换为std::filesystem::copy_options
	 */
	static std::filesystem::copy_options ConvertCopyOptions(EFileCopyOptions Options);

	/**
	 * @brief 创建文件系统错误结果
	 */
	static SFileSystemResult CreateErrorResult(const std::exception& Exception);

	/**
	 * @brief 递归遍历目录的内部实现
	 */
	static void ListDirectoryRecursive(const NPath& DirectoryPath,
	                                   const SDirectoryIterationOptions& Options,
	                                   TArray<NPath, CMemoryManager>& OutPaths,
	                                   int32_t CurrentDepth = 0);
};

// === 便捷函数 ===

/**
 * @brief 文件是否存在的便捷函数
 */
inline bool FileExists(const NPath& Path)
{
	return NFileSystem::Exists(Path) && NFileSystem::IsFile(Path);
}

/**
 * @brief 目录是否存在的便捷函数
 */
inline bool DirectoryExists(const NPath& Path)
{
	return NFileSystem::Exists(Path) && NFileSystem::IsDirectory(Path);
}

/**
 * @brief 创建目录的便捷函数
 */
inline bool CreateDirectory(const NPath& Path)
{
	return NFileSystem::CreateDirectory(Path, true).bSuccess;
}

/**
 * @brief 删除文件的便捷函数
 */
inline bool DeleteFile(const NPath& Path)
{
	return NFileSystem::DeleteFile(Path).bSuccess;
}

/**
 * @brief 删除目录的便捷函数
 */
inline bool DeleteDirectory(const NPath& Path)
{
	return NFileSystem::DeleteDirectory(Path, true).bSuccess;
}

// === 类型别名 ===
using FFileSystem = NFileSystem;
using FFileStatus = SFileStatus;
using FFileWatchEventInfo = SFileWatchEventInfo;
using FDiskSpaceInfo = NFileSystem::SDiskSpaceInfo;

} // namespace NLib
