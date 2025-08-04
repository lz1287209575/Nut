#pragma once

/**
 * @file IO.h
 * @brief NLib IO模块主头文件
 *
 * 提供完整的输入输出功能：
 * - 跨平台路径操作和文件系统访问
 * - 高性能流式读写（文件流、内存流、缓冲流）
 * - 文件监控和热重载支持
 * - 异步IO操作（未来支持）
 * - 内存映射文件（未来支持）
 */

// === 核心IO组件 ===
#include "FileSystem.h" // 文件系统操作
#include "Path.h"       // 路径工具和操作
#include "Stream.h"     // 流式读写系统

namespace NLib
{
/**
 * @brief IO模块初始化
 *
 * 初始化IO系统和相关组件
 *
 * @return 是否初始化成功
 */
inline bool InitializeIOModule()
{
	NLOG_IO(Info, "Initializing IO module");

	// IO模块大多数功能是无状态的，不需要特殊初始化
	// 未来可能需要初始化文件监控系统等

	NLOG_IO(Info, "IO module initialized successfully");
	return true;
}

/**
 * @brief IO模块关闭
 *
 * 关闭IO系统并清理资源
 */
inline void ShutdownIOModule()
{
	NLOG_IO(Info, "Shutting down IO module");

	// 停止所有文件监控
	CFileSystem::StopAllWatching();

	NLOG_IO(Info, "IO module shutdown complete");
}

/**
 * @brief 检查IO模块是否可用
 *
 * @return 是否可用
 */
inline bool IsIOModuleAvailable()
{
	// IO模块的基本功能总是可用的
	return true;
}

// === 便捷文件操作函数 ===

/**
 * @brief 快速读取文本文件
 *
 * @param FilePath 文件路径
 * @return 文件内容，失败则返回空字符串
 */
inline TString ReadTextFile(const CPath& FilePath)
{
	return CFileSystem::ReadAllText(FilePath);
}

/**
 * @brief 快速写入文本文件
 *
 * @param FilePath 文件路径
 * @param Content 文件内容
 * @param bOverwrite 是否覆盖现有文件
 * @return 是否成功
 */
inline bool WriteTextFile(const CPath& FilePath, const TString& Content, bool bOverwrite = true)
{
	return CFileSystem::WriteAllText(FilePath, Content, bOverwrite).bSuccess;
}

/**
 * @brief 快速追加文本到文件
 *
 * @param FilePath 文件路径
 * @param Content 要追加的内容
 * @return 是否成功
 */
inline bool AppendTextFile(const CPath& FilePath, const TString& Content)
{
	return CFileSystem::AppendAllText(FilePath, Content).bSuccess;
}

/**
 * @brief 快速读取二进制文件
 *
 * @param FilePath 文件路径
 * @return 文件数据，失败则返回空数组
 */
inline TArray<uint8_t, CMemoryManager> ReadBinaryFile(const CPath& FilePath)
{
	return CFileSystem::ReadAllBytes(FilePath);
}

/**
 * @brief 快速写入二进制文件
 *
 * @param FilePath 文件路径
 * @param Data 文件数据
 * @param bOverwrite 是否覆盖现有文件
 * @return 是否成功
 */
inline bool WriteBinaryFile(const CPath& FilePath, const TArray<uint8_t, CMemoryManager>& Data, bool bOverwrite = true)
{
	return CFileSystem::WriteAllBytes(FilePath, Data, bOverwrite).bSuccess;
}

// === 便捷路径操作函数 ===

/**
 * @brief 获取应用程序数据目录
 *
 * @param AppName 应用程序名称
 * @return 应用程序数据目录路径
 */
inline CPath GetAppDataDirectory(const TString& AppName)
{
#if defined(_WIN32) || defined(_WIN64)
	// Windows: %APPDATA%/AppName
	const char* AppData = std::getenv("APPDATA");
	if (AppData)
	{
		return CPath(AppData) / AppName;
	}
	else
	{
		return CPath::GetUserDirectory() / "AppData" / "Roaming" / AppName;
	}
#elif defined(__APPLE__)
	// macOS: ~/Library/Application Support/AppName
	return CPath::GetUserDirectory() / "Library" / "Application Support" / AppName;
#else
	// Linux: ~/.local/share/AppName
	const char* XdgDataHome = std::getenv("XDG_DATA_HOME");
	if (XdgDataHome)
	{
		return CPath(XdgDataHome) / AppName;
	}
	else
	{
		return CPath::GetUserDirectory() / ".local" / "share" / AppName;
	}
#endif
}

/**
 * @brief 获取应用程序配置目录
 *
 * @param AppName 应用程序名称
 * @return 应用程序配置目录路径
 */
inline CPath GetAppConfigDirectory(const TString& AppName)
{
#if defined(_WIN32) || defined(_WIN64)
	// Windows使用AppData
	return GetAppDataDirectory(AppName);
#elif defined(__APPLE__)
	// macOS: ~/Library/Preferences/AppName
	return CPath::GetUserDirectory() / "Library" / "Preferences" / AppName;
#else
	// Linux: ~/.config/AppName
	const char* XdgConfigHome = std::getenv("XDG_CONFIG_HOME");
	if (XdgConfigHome)
	{
		return CPath(XdgConfigHome) / AppName;
	}
	else
	{
		return CPath::GetUserDirectory() / ".config" / AppName;
	}
#endif
}

/**
 * @brief 获取应用程序缓存目录
 *
 * @param AppName 应用程序名称
 * @return 应用程序缓存目录路径
 */
inline CPath GetAppCacheDirectory(const TString& AppName)
{
#if defined(_WIN32) || defined(_WIN64)
	// Windows: %LOCALAPPDATA%/AppName/Cache
	const char* LocalAppData = std::getenv("LOCALAPPDATA");
	if (LocalAppData)
	{
		return CPath(LocalAppData) / AppName / "Cache";
	}
	else
	{
		return CPath::GetUserDirectory() / "AppData" / "Local" / AppName / "Cache";
	}
#elif defined(__APPLE__)
	// macOS: ~/Library/Caches/AppName
	return CPath::GetUserDirectory() / "Library" / "Caches" / AppName;
#else
	// Linux: ~/.cache/AppName
	const char* XdgCacheHome = std::getenv("XDG_CACHE_HOME");
	if (XdgCacheHome)
	{
		return CPath(XdgCacheHome) / AppName;
	}
	else
	{
		return CPath::GetUserDirectory() / ".cache" / AppName;
	}
#endif
}

/**
 * @brief 确保目录存在
 *
 * @param DirectoryPath 目录路径
 * @return 是否成功（目录存在或创建成功）
 */
inline bool EnsureDirectoryExists(const CPath& DirectoryPath)
{
	if (CFileSystem::IsDirectory(DirectoryPath))
	{
		return true;
	}

	return CFileSystem::CreateDirectory(DirectoryPath, true).bSuccess;
}

/**
 * @brief 安全删除文件（移动到回收站或临时位置）
 *
 * @param FilePath 文件路径
 * @return 是否成功
 */
inline bool SafeDeleteFile(const CPath& FilePath)
{
	if (!CFileSystem::Exists(FilePath))
	{
		return true; // 文件不存在，认为删除成功
	}

	// 生成备份文件名
	CPath BackupPath = FilePath.WithExtension(FilePath.GetExtension() + ".deleted");

	// 先移动到备份位置
	auto MoveResult = CFileSystem::Move(FilePath, BackupPath);
	if (MoveResult.bSuccess)
	{
		// 可以考虑延迟删除或移动到回收站
		NLOG_IO(Info, "File safely moved to backup: {} -> {}", FilePath.GetData(), BackupPath.GetData());
		return true;
	}

	// 如果移动失败，直接删除
	return CFileSystem::DeleteFile(FilePath).bSuccess;
}

/**
 * @brief 复制目录树
 *
 * @param SourceDir 源目录
 * @param DestDir 目标目录
 * @param bOverwrite 是否覆盖现有文件
 * @return 是否成功
 */
inline bool CopyDirectoryTree(const CPath& SourceDir, const CPath& DestDir, bool bOverwrite = false)
{
	EFileCopyOptions Options = EFileCopyOptions::Recursive;
	if (bOverwrite)
	{
		Options = static_cast<EFileCopyOptions>(static_cast<uint32_t>(Options) |
		                                        static_cast<uint32_t>(EFileCopyOptions::OverwriteExisting));
	}
	else
	{
		Options = static_cast<EFileCopyOptions>(static_cast<uint32_t>(Options) |
		                                        static_cast<uint32_t>(EFileCopyOptions::SkipExisting));
	}

	return CFileSystem::CopyDirectory(SourceDir, DestDir, Options).bSuccess;
}

// === 便捷流操作函数 ===

/**
 * @brief 创建缓冲文件流
 *
 * @param FilePath 文件路径
 * @param Access 访问模式
 * @param Mode 打开模式
 * @param BufferSize 缓冲区大小
 * @return 缓冲流指针
 */
inline TSharedPtr<CBufferedStream> CreateBufferedFileStream(const CPath& FilePath,
                                                            EStreamAccess Access = EStreamAccess::ReadWrite,
                                                            EStreamMode Mode = EStreamMode::OpenOrCreate,
                                                            int32_t BufferSize = 8192)
{
	auto FileStream = CStreamFactory::CreateFileStream(FilePath, Access, Mode);
	if (FileStream)
	{
		return CStreamFactory::CreateBufferedStream(FileStream, BufferSize);
	}
	return nullptr;
}

/**
 * @brief 从字符串创建内存流
 *
 * @param Text 文本内容
 * @return 内存流指针
 */
inline TSharedPtr<CMemoryStream> CreateMemoryStreamFromText(const TString& Text)
{
	TArray<uint8_t, CMemoryManager> Data(Text.Length());
	memcpy(Data.GetData(), Text.GetData(), Text.Length());
	return CStreamFactory::CreateMemoryStreamFromData(Data);
}

/**
 * @brief 从内存流读取文本
 *
 * @param Stream 内存流
 * @return 文本内容
 */
inline TString ReadTextFromMemoryStream(TSharedPtr<CMemoryStream> Stream)
{
	if (!Stream)
	{
		return TString();
	}

	const auto& Buffer = Stream->GetBuffer();
	return TString(reinterpret_cast<const char*>(Buffer.GetData()), Buffer.Size());
}

// === 文件模式匹配和搜索 ===

/**
 * @brief 在目录中搜索文件
 *
 * @param Directory 搜索目录
 * @param Pattern 文件名模式（支持*和?通配符）
 * @param bRecursive 是否递归搜索
 * @param bIncludeDirectories 是否包含目录
 * @return 匹配的文件路径列表
 */
inline TArray<CPath, CMemoryManager> SearchFiles(const CPath& Directory,
                                                 const TString& Pattern = TString("*"),
                                                 bool bRecursive = false,
                                                 bool bIncludeDirectories = false)
{
	SDirectoryIterationOptions Options;
	Options.bRecursive = bRecursive;
	Options.bIncludeFiles = true;
	Options.bIncludeDirectories = bIncludeDirectories;
	Options.Pattern = Pattern;
	Options.bIncludeHidden = false;

	return CFileSystem::ListDirectory(Directory, Options);
}

/**
 * @brief 查找最新修改的文件
 *
 * @param Directory 搜索目录
 * @param Pattern 文件名模式
 * @param bRecursive 是否递归搜索
 * @return 最新文件路径，如果没有找到返回空路径
 */
inline CPath FindNewestFile(const CPath& Directory, const TString& Pattern = TString("*"), bool bRecursive = false)
{
	auto Files = SearchFiles(Directory, Pattern, bRecursive, false);

	CPath NewestFile;
	CDateTime NewestTime;

	for (const auto& File : Files)
	{
		auto LastWriteTime = CFileSystem::GetLastWriteTime(File);
		if (NewestFile.IsEmpty() || LastWriteTime > NewestTime)
		{
			NewestFile = File;
			NewestTime = LastWriteTime;
		}
	}

	return NewestFile;
}

// === 临时文件和目录管理 ===

/**
 * @brief 创建临时文件并返回流
 *
 * @param Prefix 文件名前缀
 * @param Extension 文件扩展名
 * @return 临时文件流
 */
inline TSharedPtr<CFileStream> CreateTempFileStream(const TString& Prefix = TString("temp"),
                                                    const TString& Extension = TString(".tmp"))
{
	CPath TempFile = CFileSystem::CreateTempFile(Prefix, Extension);
	if (!TempFile.IsEmpty())
	{
		return CStreamFactory::CreateFileStream(TempFile, EStreamAccess::ReadWrite, EStreamMode::Create);
	}
	return nullptr;
}

/**
 * @brief RAII临时目录管理器
 */
class CTempDirectoryManager
{
public:
	explicit CTempDirectoryManager(const TString& Prefix = TString("temp"))
	{
		TempDir = CFileSystem::CreateTempDirectory(Prefix);
		if (!TempDir.IsEmpty())
		{
			NLOG_IO(Debug, "Created temporary directory: {}", TempDir.GetData());
		}
	}

	~CTempDirectoryManager()
	{
		if (!TempDir.IsEmpty() && CFileSystem::Exists(TempDir))
		{
			auto Result = CFileSystem::DeleteDirectory(TempDir, true);
			if (Result.bSuccess)
			{
				NLOG_IO(Debug, "Cleaned up temporary directory: {}", TempDir.GetData());
			}
			else
			{
				NLOG_IO(Warning, "Failed to clean up temporary directory: {}", TempDir.GetData());
			}
		}
	}

	const CPath& GetPath() const
	{
		return TempDir;
	}
	bool IsValid() const
	{
		return !TempDir.IsEmpty();
	}

	// 禁用拷贝
	CTempDirectoryManager(const CTempDirectoryManager&) = delete;
	CTempDirectoryManager& operator=(const CTempDirectoryManager&) = delete;

	// 支持移动
	CTempDirectoryManager(CTempDirectoryManager&& Other) noexcept
	    : TempDir(std::move(Other.TempDir))
	{
		Other.TempDir.Clear();
	}

	CTempDirectoryManager& operator=(CTempDirectoryManager&& Other) noexcept
	{
		if (this != &Other)
		{
			TempDir = std::move(Other.TempDir);
			Other.TempDir.Clear();
		}
		return *this;
	}

private:
	CPath TempDir;
};

// === 模块导出 ===

// 主要类的类型别名
using FIO = CFileSystem;
using FPath = CPath;
using FFileSystem = CFileSystem;
using FStream = CStream;
using FFileStream = CFileStream;
using FMemoryStream = CMemoryStream;
using FBufferedStream = CBufferedStream;
using FStreamFactory = CStreamFactory;
using FTempDirectoryManager = CTempDirectoryManager;

// 枚举类型别名
using ESeekOrigin = ESeekOrigin;
using EStreamAccess = EStreamAccess;
using EStreamMode = EStreamMode;
using EFileType = EFileType;
using EFilePermissions = EFilePermissions;
using EFileCopyOptions = EFileCopyOptions;

// 结构体类型别名
using FFileStatus = SFileStatus;
using FStreamResult = SStreamResult;
using FFileSystemResult = SFileSystemResult;
using FDirectoryIterationOptions = SDirectoryIterationOptions;

} // namespace NLib

/**
 * @brief IO模块使用示例
 *
 * @code
 * // 1. 初始化IO模块
 * NLib::InitializeIOModule();
 *
 * // 2. 基础文件操作
 * NLib::CPath ConfigFile = NLib::GetAppConfigDirectory("MyApp") / "config.json";
 * NLib::EnsureDirectoryExists(ConfigFile.GetDirectoryName());
 *
 * TString Config = "{ \"setting\": \"value\" }";
 * if (NLib::WriteTextFile(ConfigFile, Config)) {
 *     NLOG_IO(Info, "Config saved");
 * }
 *
 * // 3. 流式操作
 * auto Stream = NLib::CreateBufferedFileStream("data.bin", NLib::EStreamAccess::ReadWrite);
 * if (Stream) {
 *     uint32_t Magic = 0x12345678;
 *     Stream->Write(reinterpret_cast<const uint8_t*>(&Magic), sizeof(Magic));
 *     Stream->Flush();
 * }
 *
 * // 4. 文件搜索
 * auto LogFiles = NLib::SearchFiles("logs", "*.log", true);
 * auto NewestLog = NLib::FindNewestFile("logs", "*.log", true);
 *
 * // 5. 临时文件管理
 * {
 *     NLib::CTempDirectoryManager TempMgr("myapp");
 *     if (TempMgr.IsValid()) {
 *         NLib::CPath TempFile = TempMgr.GetPath() / "data.tmp";
 *         // 使用临时文件...
 *     }
 *     // 析构时自动清理
 * }
 *
 * // 6. 文件监控
 * NLib::CFileSystem::StartWatchingDirectory("config", true);
 * NLib::CFileSystem::OnFileWatchEvent.BindLambda([](const auto& Event) {
 *     NLOG_IO(Info, "File changed: {}", Event.FilePath.GetData());
 * });
 *
 * // 7. 关闭IO模块
 * NLib::ShutdownIOModule();
 * @endcode
 */