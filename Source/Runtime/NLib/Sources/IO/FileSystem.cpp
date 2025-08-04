#include "FileSystem.h"

#include "Logging/LogCategory.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <random>
#include <regex>

namespace NLib
{
// === 静态成员初始化 ===
CFileSystem::FOnFileWatchEvent CFileSystem::OnFileWatchEvent;

// === 文件存在性检查 ===

bool CFileSystem::Exists(const CPath& Path)
{
	try
	{
		return std::filesystem::exists(Path.ToStdPath());
	}
	catch (...)
	{
		return false;
	}
}

bool CFileSystem::IsFile(const CPath& Path)
{
	try
	{
		return std::filesystem::is_regular_file(Path.ToStdPath());
	}
	catch (...)
	{
		return false;
	}
}

bool CFileSystem::IsDirectory(const CPath& Path)
{
	try
	{
		return std::filesystem::is_directory(Path.ToStdPath());
	}
	catch (...)
	{
		return false;
	}
}

bool CFileSystem::IsSymbolicLink(const CPath& Path)
{
	try
	{
		return std::filesystem::is_symlink(Path.ToStdPath());
	}
	catch (...)
	{
		return false;
	}
}

bool CFileSystem::IsEmpty(const CPath& Path)
{
	try
	{
		return std::filesystem::is_empty(Path.ToStdPath());
	}
	catch (...)
	{
		return false;
	}
}

// === 文件状态查询 ===

SFileStatus CFileSystem::GetFileStatus(const CPath& Path)
{
	SFileStatus Status(Path);

	try
	{
		auto StdPath = Path.ToStdPath();

		if (!std::filesystem::exists(StdPath))
		{
			return Status;
		}

		Status.bExists = true;

		auto FileStatus = std::filesystem::status(StdPath);
		Status.Type = ConvertFileType(FileStatus);
		Status.Permissions = ConvertPermissions(FileStatus.permissions());

		if (std::filesystem::is_regular_file(StdPath))
		{
			Status.Size = std::filesystem::file_size(StdPath);
		}

		auto LastWriteTime = std::filesystem::last_write_time(StdPath);
		// 简化实现，将文件时间转换为CDateTime
		auto SystemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
		    LastWriteTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
		auto TimeT = std::chrono::system_clock::to_time_t(SystemTime);
		Status.LastWriteTime = CDateTime::FromTimeT(TimeT);

		// 在支持的平台上获取创建时间和访问时间
#if defined(_WIN32) || defined(_WIN64)
		// Windows平台可以获取更详细的时间信息
		// 这里使用简化实现
		Status.CreationTime = Status.LastWriteTime;
		Status.LastAccessTime = Status.LastWriteTime;
#else
		// Unix平台
		Status.CreationTime = Status.LastWriteTime;
		Status.LastAccessTime = Status.LastWriteTime;
#endif

		// 检查只读属性
		Status.bIsReadOnly = (Status.Permissions & EFilePermissions::OwnerWrite) == EFilePermissions::None;

		// 检查隐藏属性（简化实现）
		TString FileName = Path.GetFileName();
		Status.bIsHidden = FileName.StartsWith(".");
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to get file status for '{}': {}", Path.GetData(), e.what());
	}

	return Status;
}

uint64_t CFileSystem::GetFileSize(const CPath& Path)
{
	try
	{
		return std::filesystem::file_size(Path.ToStdPath());
	}
	catch (...)
	{
		return 0;
	}
}

CDateTime CFileSystem::GetLastWriteTime(const CPath& Path)
{
	try
	{
		auto LastWriteTime = std::filesystem::last_write_time(Path.ToStdPath());
		auto SystemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
		    LastWriteTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
		auto TimeT = std::chrono::system_clock::to_time_t(SystemTime);
		return CDateTime::FromTimeT(TimeT);
	}
	catch (...)
	{
		return CDateTime();
	}
}

CDateTime CFileSystem::GetCreationTime(const CPath& Path)
{
	// 简化实现，大部分平台不直接支持创建时间
	return GetLastWriteTime(Path);
}

EFilePermissions CFileSystem::GetPermissions(const CPath& Path)
{
	try
	{
		auto Perms = std::filesystem::status(Path.ToStdPath()).permissions();
		return ConvertPermissions(Perms);
	}
	catch (...)
	{
		return EFilePermissions::None;
	}
}

// === 文件和目录创建 ===

SFileSystemResult CFileSystem::CreateDirectory(const CPath& Path, bool bCreateParents)
{
	try
	{
		auto StdPath = Path.ToStdPath();

		if (std::filesystem::exists(StdPath))
		{
			if (std::filesystem::is_directory(StdPath))
			{
				return SFileSystemResult(true);
			}
			else
			{
				return SFileSystemResult(false, "Path exists but is not a directory");
			}
		}

		bool bSuccess;
		if (bCreateParents)
		{
			bSuccess = std::filesystem::create_directories(StdPath);
		}
		else
		{
			bSuccess = std::filesystem::create_directory(StdPath);
		}

		if (bSuccess)
		{
			NLOG_IO(Debug, "Created directory: {}", Path.GetData());
			return SFileSystemResult(true);
		}
		else
		{
			return SFileSystemResult(false, "Failed to create directory");
		}
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to create directory '{}': {}", Path.GetData(), e.what());
		return CreateErrorResult(e);
	}
}

SFileSystemResult CFileSystem::CreateFile(const CPath& Path, bool bOverwrite)
{
	try
	{
		auto StdPath = Path.ToStdPath();

		if (std::filesystem::exists(StdPath) && !bOverwrite)
		{
			return SFileSystemResult(false, "File already exists");
		}

		// 确保父目录存在
		auto ParentPath = StdPath.parent_path();
		if (!ParentPath.empty() && !std::filesystem::exists(ParentPath))
		{
			std::filesystem::create_directories(ParentPath);
		}

		// 创建空文件
		std::ofstream File(StdPath);
		if (!File)
		{
			return SFileSystemResult(false, "Failed to create file");
		}

		File.close();

		NLOG_IO(Debug, "Created file: {}", Path.GetData());
		return SFileSystemResult(true);
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to create file '{}': {}", Path.GetData(), e.what());
		return CreateErrorResult(e);
	}
}

SFileSystemResult CFileSystem::CreateSymbolicLink(const CPath& LinkPath, const CPath& TargetPath)
{
	try
	{
		std::filesystem::create_symlink(TargetPath.ToStdPath(), LinkPath.ToStdPath());
		NLOG_IO(Debug, "Created symbolic link: {} -> {}", LinkPath.GetData(), TargetPath.GetData());
		return SFileSystemResult(true);
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error,
		        "Failed to create symbolic link '{}' -> '{}': {}",
		        LinkPath.GetData(),
		        TargetPath.GetData(),
		        e.what());
		return CreateErrorResult(e);
	}
}

SFileSystemResult CFileSystem::CreateHardLink(const CPath& LinkPath, const CPath& TargetPath)
{
	try
	{
		std::filesystem::create_hard_link(TargetPath.ToStdPath(), LinkPath.ToStdPath());
		NLOG_IO(Debug, "Created hard link: {} -> {}", LinkPath.GetData(), TargetPath.GetData());
		return SFileSystemResult(true);
	}
	catch (const std::exception& e)
	{
		NLOG_IO(
		    Error, "Failed to create hard link '{}' -> '{}': {}", LinkPath.GetData(), TargetPath.GetData(), e.what());
		return CreateErrorResult(e);
	}
}

// === 文件和目录删除 ===

SFileSystemResult CFileSystem::DeleteFile(const CPath& Path)
{
	try
	{
		auto StdPath = Path.ToStdPath();

		if (!std::filesystem::exists(StdPath))
		{
			return SFileSystemResult(true); // 文件不存在，认为删除成功
		}

		if (!std::filesystem::is_regular_file(StdPath))
		{
			return SFileSystemResult(false, "Path is not a regular file");
		}

		bool bSuccess = std::filesystem::remove(StdPath);
		if (bSuccess)
		{
			NLOG_IO(Debug, "Deleted file: {}", Path.GetData());
			return SFileSystemResult(true);
		}
		else
		{
			return SFileSystemResult(false, "Failed to delete file");
		}
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to delete file '{}': {}", Path.GetData(), e.what());
		return CreateErrorResult(e);
	}
}

SFileSystemResult CFileSystem::DeleteDirectory(const CPath& Path, bool bRecursive)
{
	try
	{
		auto StdPath = Path.ToStdPath();

		if (!std::filesystem::exists(StdPath))
		{
			return SFileSystemResult(true); // 目录不存在，认为删除成功
		}

		if (!std::filesystem::is_directory(StdPath))
		{
			return SFileSystemResult(false, "Path is not a directory");
		}

		std::uintmax_t DeletedCount;
		if (bRecursive)
		{
			DeletedCount = std::filesystem::remove_all(StdPath);
		}
		else
		{
			if (!std::filesystem::is_empty(StdPath))
			{
				return SFileSystemResult(false, "Directory is not empty");
			}
			DeletedCount = std::filesystem::remove(StdPath) ? 1 : 0;
		}

		if (DeletedCount > 0)
		{
			NLOG_IO(Debug, "Deleted directory: {} ({} items)", Path.GetData(), DeletedCount);
			return SFileSystemResult(true);
		}
		else
		{
			return SFileSystemResult(false, "Failed to delete directory");
		}
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to delete directory '{}': {}", Path.GetData(), e.what());
		return CreateErrorResult(e);
	}
}

SFileSystemResult CFileSystem::Delete(const CPath& Path, bool bRecursive)
{
	if (IsDirectory(Path))
	{
		return DeleteDirectory(Path, bRecursive);
	}
	else
	{
		return DeleteFile(Path);
	}
}

// === 文件和目录移动/重命名 ===

SFileSystemResult CFileSystem::Move(const CPath& SourcePath, const CPath& DestinationPath)
{
	try
	{
		auto SourceStdPath = SourcePath.ToStdPath();
		auto DestStdPath = DestinationPath.ToStdPath();

		if (!std::filesystem::exists(SourceStdPath))
		{
			return SFileSystemResult(false, "Source path does not exist");
		}

		// 确保目标目录存在
		auto DestParent = DestStdPath.parent_path();
		if (!DestParent.empty() && !std::filesystem::exists(DestParent))
		{
			std::filesystem::create_directories(DestParent);
		}

		std::filesystem::rename(SourceStdPath, DestStdPath);

		NLOG_IO(Debug, "Moved: {} -> {}", SourcePath.GetData(), DestinationPath.GetData());
		return SFileSystemResult(true);
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to move '{}' to '{}': {}", SourcePath.GetData(), DestinationPath.GetData(), e.what());
		return CreateErrorResult(e);
	}
}

SFileSystemResult CFileSystem::Rename(const CPath& Path, const TString& NewName)
{
	CPath NewPath = Path.GetDirectoryName() / NewName;
	return Move(Path, NewPath);
}

// === 文件和目录复制 ===

SFileSystemResult CFileSystem::CopyFile(const CPath& SourcePath, const CPath& DestinationPath, EFileCopyOptions Options)
{
	try
	{
		auto SourceStdPath = SourcePath.ToStdPath();
		auto DestStdPath = DestinationPath.ToStdPath();

		if (!std::filesystem::exists(SourceStdPath))
		{
			return SFileSystemResult(false, "Source file does not exist");
		}

		if (!std::filesystem::is_regular_file(SourceStdPath))
		{
			return SFileSystemResult(false, "Source is not a regular file");
		}

		// 确保目标目录存在
		auto DestParent = DestStdPath.parent_path();
		if (!DestParent.empty() && !std::filesystem::exists(DestParent))
		{
			std::filesystem::create_directories(DestParent);
		}

		auto CopyOpts = ConvertCopyOptions(Options);
		std::filesystem::copy_file(SourceStdPath, DestStdPath, CopyOpts);

		NLOG_IO(Debug, "Copied file: {} -> {}", SourcePath.GetData(), DestinationPath.GetData());
		return SFileSystemResult(true);
	}
	catch (const std::exception& e)
	{
		NLOG_IO(
		    Error, "Failed to copy file '{}' to '{}': {}", SourcePath.GetData(), DestinationPath.GetData(), e.what());
		return CreateErrorResult(e);
	}
}

SFileSystemResult CFileSystem::CopyDirectory(const CPath& SourcePath,
                                             const CPath& DestinationPath,
                                             EFileCopyOptions Options)
{
	try
	{
		auto SourceStdPath = SourcePath.ToStdPath();
		auto DestStdPath = DestinationPath.ToStdPath();

		if (!std::filesystem::exists(SourceStdPath))
		{
			return SFileSystemResult(false, "Source directory does not exist");
		}

		if (!std::filesystem::is_directory(SourceStdPath))
		{
			return SFileSystemResult(false, "Source is not a directory");
		}

		auto CopyOpts = ConvertCopyOptions(Options);
		std::filesystem::copy(SourceStdPath, DestStdPath, CopyOpts);

		NLOG_IO(Debug, "Copied directory: {} -> {}", SourcePath.GetData(), DestinationPath.GetData());
		return SFileSystemResult(true);
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error,
		        "Failed to copy directory '{}' to '{}': {}",
		        SourcePath.GetData(),
		        DestinationPath.GetData(),
		        e.what());
		return CreateErrorResult(e);
	}
}

// === 目录遍历 ===

TArray<CPath, CMemoryManager> CFileSystem::ListDirectory(const CPath& DirectoryPath,
                                                         const SDirectoryIterationOptions& Options)
{
	TArray<CPath, CMemoryManager> Results;

	try
	{
		auto StdPath = DirectoryPath.ToStdPath();

		if (!std::filesystem::exists(StdPath) || !std::filesystem::is_directory(StdPath))
		{
			return Results;
		}

		if (Options.bRecursive)
		{
			ListDirectoryRecursive(DirectoryPath, Options, Results, 0);
		}
		else
		{
			for (const auto& Entry : std::filesystem::directory_iterator(StdPath))
			{
				CPath EntryPath(Entry.path().string().c_str());

				// 检查文件类型过滤
				bool bShouldInclude = false;
				if (Entry.is_directory() && Options.bIncludeDirectories)
				{
					bShouldInclude = true;
				}
				else if (Entry.is_regular_file() && Options.bIncludeFiles)
				{
					bShouldInclude = true;
				}

				// 检查隐藏文件过滤
				if (bShouldInclude && !Options.bIncludeHidden)
				{
					TString FileName = EntryPath.GetFileName();
					if (FileName.StartsWith("."))
					{
						bShouldInclude = false;
					}
				}

				// 检查模式匹配
				if (bShouldInclude && !Options.Pattern.IsEmpty())
				{
					TString FileName = EntryPath.GetFileName();
					if (!MatchPattern(FileName, Options.Pattern))
					{
						bShouldInclude = false;
					}
				}

				if (bShouldInclude)
				{
					Results.Add(EntryPath);
				}
			}
		}
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to list directory '{}': {}", DirectoryPath.GetData(), e.what());
	}

	return Results;
}

TArray<CPath, CMemoryManager> CFileSystem::FindFiles(const CPath& DirectoryPath,
                                                     const TString& Pattern,
                                                     bool bRecursive)
{
	SDirectoryIterationOptions Options;
	Options.bRecursive = bRecursive;
	Options.bIncludeDirectories = false;
	Options.bIncludeFiles = true;
	Options.Pattern = Pattern;

	return ListDirectory(DirectoryPath, Options);
}

TArray<CPath, CMemoryManager> CFileSystem::FindDirectories(const CPath& DirectoryPath,
                                                           const TString& Pattern,
                                                           bool bRecursive)
{
	SDirectoryIterationOptions Options;
	Options.bRecursive = bRecursive;
	Options.bIncludeDirectories = true;
	Options.bIncludeFiles = false;
	Options.Pattern = Pattern;

	return ListDirectory(DirectoryPath, Options);
}

// === 文件权限和属性 ===

SFileSystemResult CFileSystem::SetPermissions(const CPath& Path, EFilePermissions Permissions)
{
	try
	{
		auto StdPath = Path.ToStdPath();
		auto StdPerms = ConvertToStdPermissions(Permissions);

		std::filesystem::permissions(StdPath, StdPerms);

		NLOG_IO(Debug, "Set permissions for: {}", Path.GetData());
		return SFileSystemResult(true);
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to set permissions for '{}': {}", Path.GetData(), e.what());
		return CreateErrorResult(e);
	}
}

SFileSystemResult CFileSystem::SetReadOnly(const CPath& Path, bool bReadOnly)
{
	auto CurrentPermissions = GetPermissions(Path);

	if (bReadOnly)
	{
		// 移除写权限
		CurrentPermissions = static_cast<EFilePermissions>(static_cast<uint32_t>(CurrentPermissions) &
		                                                   ~(static_cast<uint32_t>(EFilePermissions::OwnerWrite) |
		                                                     static_cast<uint32_t>(EFilePermissions::GroupWrite) |
		                                                     static_cast<uint32_t>(EFilePermissions::OthersWrite)));
	}
	else
	{
		// 添加所有者写权限
		CurrentPermissions = static_cast<EFilePermissions>(static_cast<uint32_t>(CurrentPermissions) |
		                                                   static_cast<uint32_t>(EFilePermissions::OwnerWrite));
	}

	return SetPermissions(Path, CurrentPermissions);
}

SFileSystemResult CFileSystem::SetHidden(const CPath& Path, bool bHidden)
{
	// 在Unix系统上，隐藏文件通过文件名前缀.来实现
	// 这里提供基本实现
	if (bHidden)
	{
		TString FileName = Path.GetFileName();
		if (!FileName.StartsWith("."))
		{
			CPath NewPath = Path.GetDirectoryName() / ("." + FileName);
			return Move(Path, NewPath);
		}
	}
	else
	{
		TString FileName = Path.GetFileName();
		if (FileName.StartsWith("."))
		{
			CPath NewPath = Path.GetDirectoryName() / FileName.Substring(1);
			return Move(Path, NewPath);
		}
	}

	return SFileSystemResult(true);
}

SFileSystemResult CFileSystem::SetFileTime(const CPath& Path,
                                           const CDateTime& LastWriteTime,
                                           const CDateTime& LastAccessTime)
{
	try
	{
		auto StdPath = Path.ToStdPath();
		auto TimeT = LastWriteTime.ToTimeT();
		auto SystemTime = std::chrono::system_clock::from_time_t(TimeT);
		auto FileTime = std::chrono::time_point_cast<std::filesystem::file_time_type::duration>(
		    SystemTime - std::chrono::system_clock::now() + std::filesystem::file_time_type::clock::now());

		std::filesystem::last_write_time(StdPath, FileTime);

		NLOG_IO(Debug, "Set file time for: {}", Path.GetData());
		return SFileSystemResult(true);
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to set file time for '{}': {}", Path.GetData(), e.what());
		return CreateErrorResult(e);
	}
}

// === 符号链接操作 ===

CPath CFileSystem::ReadSymbolicLink(const CPath& LinkPath)
{
	try
	{
		auto Target = std::filesystem::read_symlink(LinkPath.ToStdPath());
		return CPath(Target.string().c_str());
	}
	catch (...)
	{
		return CPath();
	}
}

CPath CFileSystem::ResolveSymbolicLink(const CPath& LinkPath)
{
	try
	{
		auto Canonical = std::filesystem::canonical(LinkPath.ToStdPath());
		return CPath(Canonical.string().c_str());
	}
	catch (...)
	{
		return CPath();
	}
}

// === 路径操作 ===

CPath CFileSystem::GetAbsolutePath(const CPath& Path)
{
	try
	{
		auto Absolute = std::filesystem::absolute(Path.ToStdPath());
		return CPath(Absolute.string().c_str());
	}
	catch (...)
	{
		return Path.GetAbsolute();
	}
}

CPath CFileSystem::GetRelativePath(const CPath& Path, const CPath& BasePath)
{
	return Path.GetRelative(BasePath);
}

CPath CFileSystem::CanonicalizePath(const CPath& Path)
{
	try
	{
		auto Canonical = std::filesystem::canonical(Path.ToStdPath());
		return CPath(Canonical.string().c_str());
	}
	catch (...)
	{
		return Path.GetNormalized();
	}
}

// === 磁盘空间查询 ===

CFileSystem::SDiskSpaceInfo CFileSystem::GetDiskSpaceInfo(const CPath& Path)
{
	SDiskSpaceInfo Info;

	try
	{
		auto SpaceInfo = std::filesystem::space(Path.ToStdPath());
		Info.Capacity = SpaceInfo.capacity;
		Info.Free = SpaceInfo.free;
		Info.Available = SpaceInfo.available;
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to get disk space info for '{}': {}", Path.GetData(), e.what());
	}

	return Info;
}

// === 临时文件操作 ===

CPath CFileSystem::CreateTempFile(const TString& Prefix, const TString& Extension)
{
	try
	{
		auto TempDir = GetTempDirectory();

		// 生成随机文件名
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(1000, 9999);

		TString FileName = Prefix + TString::FromInt(dis(gen)) + Extension;
		CPath TempFile = TempDir / FileName;

		// 确保文件名唯一
		int32_t Counter = 0;
		while (Exists(TempFile) && Counter < 1000)
		{
			FileName = Prefix + TString::FromInt(dis(gen)) + TString("_") + TString::FromInt(Counter) + Extension;
			TempFile = TempDir / FileName;
			Counter++;
		}

		// 创建空文件
		if (CreateFile(TempFile, false).bSuccess)
		{
			return TempFile;
		}
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to create temp file: {}", e.what());
	}

	return CPath();
}

CPath CFileSystem::CreateTempDirectory(const TString& Prefix)
{
	try
	{
		auto TempDir = GetTempDirectory();

		// 生成随机目录名
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dis(1000, 9999);

		TString DirName = Prefix + TString::FromInt(dis(gen));
		CPath TempSubDir = TempDir / DirName;

		// 确保目录名唯一
		int32_t Counter = 0;
		while (Exists(TempSubDir) && Counter < 1000)
		{
			DirName = Prefix + TString::FromInt(dis(gen)) + TString("_") + TString::FromInt(Counter);
			TempSubDir = TempDir / DirName;
			Counter++;
		}

		// 创建目录
		if (CreateDirectory(TempSubDir, true).bSuccess)
		{
			return TempSubDir;
		}
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to create temp directory: {}", e.what());
	}

	return CPath();
}

// === 文件内容快速操作 ===

TString CFileSystem::ReadAllText(const CPath& Path)
{
	try
	{
		std::ifstream File(Path.ToStdPath(), std::ios::in);
		if (!File.is_open())
		{
			NLOG_IO(Error, "Failed to open file for reading: {}", Path.GetData());
			return TString();
		}

		std::string Content((std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());
		return TString(Content.c_str());
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to read file '{}': {}", Path.GetData(), e.what());
		return TString();
	}
}

TArray<uint8_t, CMemoryManager> CFileSystem::ReadAllBytes(const CPath& Path)
{
	TArray<uint8_t, CMemoryManager> Data;

	try
	{
		std::ifstream File(Path.ToStdPath(), std::ios::binary);
		if (!File.is_open())
		{
			NLOG_IO(Error, "Failed to open file for reading: {}", Path.GetData());
			return Data;
		}

		File.seekg(0, std::ios::end);
		std::streamsize Size = File.tellg();
		File.seekg(0, std::ios::beg);

		if (Size > 0)
		{
			Data.Resize(static_cast<int32_t>(Size));
			File.read(reinterpret_cast<char*>(Data.GetData()), Size);
		}
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to read file '{}': {}", Path.GetData(), e.what());
		Data.Clear();
	}

	return Data;
}

SFileSystemResult CFileSystem::WriteAllText(const CPath& Path, const TString& Content, bool bOverwrite)
{
	try
	{
		if (Exists(Path) && !bOverwrite)
		{
			return SFileSystemResult(false, "File already exists");
		}

		// 确保父目录存在
		auto ParentPath = Path.GetDirectoryName();
		if (!ParentPath.IsEmpty() && !Exists(ParentPath))
		{
			CreateDirectory(ParentPath, true);
		}

		std::ofstream File(Path.ToStdPath(), std::ios::out | std::ios::trunc);
		if (!File.is_open())
		{
			return SFileSystemResult(false, "Failed to open file for writing");
		}

		File << Content.GetData();
		File.close();

		if (File.fail())
		{
			return SFileSystemResult(false, "Failed to write file");
		}

		NLOG_IO(Debug, "Wrote text file: {}", Path.GetData());
		return SFileSystemResult(true);
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to write file '{}': {}", Path.GetData(), e.what());
		return CreateErrorResult(e);
	}
}

SFileSystemResult CFileSystem::WriteAllBytes(const CPath& Path,
                                             const TArray<uint8_t, CMemoryManager>& Data,
                                             bool bOverwrite)
{
	try
	{
		if (Exists(Path) && !bOverwrite)
		{
			return SFileSystemResult(false, "File already exists");
		}

		// 确保父目录存在
		auto ParentPath = Path.GetDirectoryName();
		if (!ParentPath.IsEmpty() && !Exists(ParentPath))
		{
			CreateDirectory(ParentPath, true);
		}

		std::ofstream File(Path.ToStdPath(), std::ios::binary | std::ios::trunc);
		if (!File.is_open())
		{
			return SFileSystemResult(false, "Failed to open file for writing");
		}

		if (!Data.IsEmpty())
		{
			File.write(reinterpret_cast<const char*>(Data.GetData()), Data.Size());
		}

		File.close();

		if (File.fail())
		{
			return SFileSystemResult(false, "Failed to write file");
		}

		NLOG_IO(Debug, "Wrote binary file: {} ({} bytes)", Path.GetData(), Data.Size());
		return SFileSystemResult(true);
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to write file '{}': {}", Path.GetData(), e.what());
		return CreateErrorResult(e);
	}
}

SFileSystemResult CFileSystem::AppendAllText(const CPath& Path, const TString& Content)
{
	try
	{
		// 确保父目录存在
		auto ParentPath = Path.GetDirectoryName();
		if (!ParentPath.IsEmpty() && !Exists(ParentPath))
		{
			CreateDirectory(ParentPath, true);
		}

		std::ofstream File(Path.ToStdPath(), std::ios::app);
		if (!File.is_open())
		{
			return SFileSystemResult(false, "Failed to open file for appending");
		}

		File << Content.GetData();
		File.close();

		if (File.fail())
		{
			return SFileSystemResult(false, "Failed to append to file");
		}

		NLOG_IO(Debug, "Appended to file: {}", Path.GetData());
		return SFileSystemResult(true);
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to append to file '{}': {}", Path.GetData(), e.what());
		return CreateErrorResult(e);
	}
}

// === 文件监控 ===

bool CFileSystem::StartWatchingDirectory(const CPath& DirectoryPath, bool bRecursive)
{
	// 文件监控功能的实现比较复杂，这里提供基础框架
	// 实际实现需要使用平台特定的API（如inotify、ReadDirectoryChangesW等）

	NLOG_IO(Info, "Started watching directory: {} (recursive: {})", DirectoryPath.GetData(), bRecursive ? "yes" : "no");

	// TODO: 实现真正的文件监控
	return true;
}

void CFileSystem::StopWatchingDirectory(const CPath& DirectoryPath)
{
	NLOG_IO(Info, "Stopped watching directory: {}", DirectoryPath.GetData());

	// TODO: 实现文件监控停止
}

void CFileSystem::StopAllWatching()
{
	NLOG_IO(Info, "Stopped all file watching");

	// TODO: 停止所有文件监控
}

// === 工具函数 ===

bool CFileSystem::MatchPattern(const TString& FileName, const TString& Pattern)
{
	if (Pattern.IsEmpty())
	{
		return true;
	}

	// 简单的通配符匹配实现
	try
	{
		// 将通配符模式转换为正则表达式
		TString RegexPattern = Pattern;
		RegexPattern = RegexPattern.Replace(".", "\\.");
		RegexPattern = RegexPattern.Replace("*", ".*");
		RegexPattern = RegexPattern.Replace("?", ".");

		std::regex Regex(RegexPattern.GetData(), std::regex_constants::icase);
		return std::regex_match(FileName.GetData(), Regex);
	}
	catch (...)
	{
		// 如果正则表达式失败，使用简单的字符串匹配
		return FileName.Contains(Pattern);
	}
}

uint64_t CFileSystem::CalculateDirectorySize(const CPath& DirectoryPath, bool bRecursive)
{
	uint64_t TotalSize = 0;

	try
	{
		auto StdPath = DirectoryPath.ToStdPath();

		if (!std::filesystem::exists(StdPath) || !std::filesystem::is_directory(StdPath))
		{
			return 0;
		}

		if (bRecursive)
		{
			for (const auto& Entry : std::filesystem::recursive_directory_iterator(StdPath))
			{
				if (Entry.is_regular_file())
				{
					TotalSize += Entry.file_size();
				}
			}
		}
		else
		{
			for (const auto& Entry : std::filesystem::directory_iterator(StdPath))
			{
				if (Entry.is_regular_file())
				{
					TotalSize += Entry.file_size();
				}
			}
		}
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Failed to calculate directory size for '{}': {}", DirectoryPath.GetData(), e.what());
	}

	return TotalSize;
}

TString CFileSystem::CalculateChecksum(const CPath& Path, const TString& Algorithm)
{
	// 简化实现，实际应该使用专门的哈希库
	NLOG_IO(Warning, "Checksum calculation not implemented yet for: {}", Path.GetData());
	return TString();
}

// === 内部实现 ===

EFileType CFileSystem::ConvertFileType(const std::filesystem::file_status& Status)
{
	auto Type = Status.type();

	switch (Type)
	{
	case std::filesystem::file_type::regular:
		return EFileType::Regular;
	case std::filesystem::file_type::directory:
		return EFileType::Directory;
	case std::filesystem::file_type::symlink:
		return EFileType::SymbolicLink;
	case std::filesystem::file_type::block:
		return EFileType::BlockDevice;
	case std::filesystem::file_type::character:
		return EFileType::CharDevice;
	case std::filesystem::file_type::fifo:
		return EFileType::Fifo;
	case std::filesystem::file_type::socket:
		return EFileType::Socket;
	default:
		return EFileType::Unknown;
	}
}

EFilePermissions CFileSystem::ConvertPermissions(std::filesystem::perms Perms)
{
	uint32_t Result = 0;

	using std::filesystem::perms;

	if ((Perms & perms::owner_read) != perms::none)
		Result |= static_cast<uint32_t>(EFilePermissions::OwnerRead);
	if ((Perms & perms::owner_write) != perms::none)
		Result |= static_cast<uint32_t>(EFilePermissions::OwnerWrite);
	if ((Perms & perms::owner_exec) != perms::none)
		Result |= static_cast<uint32_t>(EFilePermissions::OwnerExec);

	if ((Perms & perms::group_read) != perms::none)
		Result |= static_cast<uint32_t>(EFilePermissions::GroupRead);
	if ((Perms & perms::group_write) != perms::none)
		Result |= static_cast<uint32_t>(EFilePermissions::GroupWrite);
	if ((Perms & perms::group_exec) != perms::none)
		Result |= static_cast<uint32_t>(EFilePermissions::GroupExec);

	if ((Perms & perms::others_read) != perms::none)
		Result |= static_cast<uint32_t>(EFilePermissions::OthersRead);
	if ((Perms & perms::others_write) != perms::none)
		Result |= static_cast<uint32_t>(EFilePermissions::OthersWrite);
	if ((Perms & perms::others_exec) != perms::none)
		Result |= static_cast<uint32_t>(EFilePermissions::OthersExec);

	return static_cast<EFilePermissions>(Result);
}

std::filesystem::perms CFileSystem::ConvertToStdPermissions(EFilePermissions Permissions)
{
	std::filesystem::perms Result = std::filesystem::perms::none;

	uint32_t PermValue = static_cast<uint32_t>(Permissions);

	if (PermValue & static_cast<uint32_t>(EFilePermissions::OwnerRead))
		Result |= std::filesystem::perms::owner_read;
	if (PermValue & static_cast<uint32_t>(EFilePermissions::OwnerWrite))
		Result |= std::filesystem::perms::owner_write;
	if (PermValue & static_cast<uint32_t>(EFilePermissions::OwnerExec))
		Result |= std::filesystem::perms::owner_exec;

	if (PermValue & static_cast<uint32_t>(EFilePermissions::GroupRead))
		Result |= std::filesystem::perms::group_read;
	if (PermValue & static_cast<uint32_t>(EFilePermissions::GroupWrite))
		Result |= std::filesystem::perms::group_write;
	if (PermValue & static_cast<uint32_t>(EFilePermissions::GroupExec))
		Result |= std::filesystem::perms::group_exec;

	if (PermValue & static_cast<uint32_t>(EFilePermissions::OthersRead))
		Result |= std::filesystem::perms::others_read;
	if (PermValue & static_cast<uint32_t>(EFilePermissions::OthersWrite))
		Result |= std::filesystem::perms::others_write;
	if (PermValue & static_cast<uint32_t>(EFilePermissions::OthersExec))
		Result |= std::filesystem::perms::others_exec;

	return Result;
}

std::filesystem::copy_options CFileSystem::ConvertCopyOptions(EFileCopyOptions Options)
{
	std::filesystem::copy_options Result = std::filesystem::copy_options::none;

	uint32_t OptValue = static_cast<uint32_t>(Options);

	if (OptValue & static_cast<uint32_t>(EFileCopyOptions::SkipExisting))
		Result |= std::filesystem::copy_options::skip_existing;
	if (OptValue & static_cast<uint32_t>(EFileCopyOptions::OverwriteExisting))
		Result |= std::filesystem::copy_options::overwrite_existing;
	if (OptValue & static_cast<uint32_t>(EFileCopyOptions::UpdateExisting))
		Result |= std::filesystem::copy_options::update_existing;
	if (OptValue & static_cast<uint32_t>(EFileCopyOptions::Recursive))
		Result |= std::filesystem::copy_options::recursive;
	if (OptValue & static_cast<uint32_t>(EFileCopyOptions::CopySymlinks))
		Result |= std::filesystem::copy_options::copy_symlinks;

	return Result;
}

SFileSystemResult CFileSystem::CreateErrorResult(const std::exception& Exception)
{
	return SFileSystemResult(false, TString(Exception.what()));
}

void CFileSystem::ListDirectoryRecursive(const CPath& DirectoryPath,
                                         const SDirectoryIterationOptions& Options,
                                         TArray<CPath, CMemoryManager>& OutPaths,
                                         int32_t CurrentDepth)
{
	if (Options.MaxDepth >= 0 && CurrentDepth >= Options.MaxDepth)
	{
		return;
	}

	try
	{
		auto StdPath = DirectoryPath.ToStdPath();

		for (const auto& Entry : std::filesystem::directory_iterator(StdPath))
		{
			CPath EntryPath(Entry.path().string().c_str());

			// 检查文件类型过滤
			bool bShouldInclude = false;
			bool bIsDirectory = Entry.is_directory();

			if (bIsDirectory && Options.bIncludeDirectories)
			{
				bShouldInclude = true;
			}
			else if (Entry.is_regular_file() && Options.bIncludeFiles)
			{
				bShouldInclude = true;
			}

			// 检查隐藏文件过滤
			if (bShouldInclude && !Options.bIncludeHidden)
			{
				TString FileName = EntryPath.GetFileName();
				if (FileName.StartsWith("."))
				{
					bShouldInclude = false;
				}
			}

			// 检查模式匹配
			if (bShouldInclude && !Options.Pattern.IsEmpty())
			{
				TString FileName = EntryPath.GetFileName();
				if (!MatchPattern(FileName, Options.Pattern))
				{
					bShouldInclude = false;
				}
			}

			if (bShouldInclude)
			{
				OutPaths.Add(EntryPath);
			}

			// 递归处理子目录
			if (bIsDirectory)
			{
				ListDirectoryRecursive(EntryPath, Options, OutPaths, CurrentDepth + 1);
			}
		}
	}
	catch (const std::exception& e)
	{
		NLOG_IO(Error, "Error during recursive directory listing in '{}': {}", DirectoryPath.GetData(), e.what());
	}
}

} // namespace NLib