#include "Path.h"

#include "Logging/LogCategory.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>

namespace NLib
{
// === 构造函数 ===

NPath::NPath()
{}

NPath::NPath(const TString& InPath)
    : PathString(InPath)
{
	Normalize();
}

NPath::NPath(const char* InPath)
    : PathString(InPath ? InPath : "")
{
	Normalize();
}

NPath::NPath(const NPath& Other)
    : PathString(Other.PathString)
{}

NPath::NPath(NPath&& Other) noexcept
    : PathString(std::move(Other.PathString))
{}

// === 赋值操作符 ===

NPath& NPath::operator=(const NPath& Other)
{
	if (this != &Other)
	{
		PathString = Other.PathString;
	}
	return *this;
}

NPath& NPath::operator=(NPath&& Other) noexcept
{
	if (this != &Other)
	{
		PathString = std::move(Other.PathString);
	}
	return *this;
}

NPath& NPath::operator=(const TString& InPath)
{
	PathString = InPath;
	Normalize();
	return *this;
}

NPath& NPath::operator=(const char* InPath)
{
	PathString = InPath ? InPath : "";
	Normalize();
	return *this;
}

// === 比较操作符 ===

bool NPath::operator==(const NPath& Other) const
{
	return PathString == Other.PathString;
}

bool NPath::operator!=(const NPath& Other) const
{
	return PathString != Other.PathString;
}

bool NPath::operator<(const NPath& Other) const
{
	return PathString < Other.PathString;
}

// === 路径拼接操作符 ===

NPath NPath::operator/(const NPath& Other) const
{
	NPath Result(*this);
	Result /= Other;
	return Result;
}

NPath NPath::operator/(const TString& Other) const
{
	NPath Result(*this);
	Result /= Other;
	return Result;
}

NPath NPath::operator/(const char* Other) const
{
	NPath Result(*this);
	Result /= Other;
	return Result;
}

NPath& NPath::operator/=(const NPath& Other)
{
	return *this /= Other.PathString;
}

NPath& NPath::operator/=(const TString& Other)
{
	if (Other.IsEmpty())
	{
		return *this;
	}

	if (PathString.IsEmpty())
	{
		PathString = Other;
	}
	else
	{
		// 确保路径分隔符
		if (!PathString.EndsWith(FPathConstants::DirectorySeparatorString) &&
		    !PathString.EndsWith(FPathConstants::AltDirectorySeparatorString) &&
		    !Other.StartsWith(FPathConstants::DirectorySeparatorString) &&
		    !Other.StartsWith(FPathConstants::AltDirectorySeparatorString))
		{
			PathString += FPathConstants::DirectorySeparatorString;
		}
		PathString += Other;
	}

	Normalize();
	return *this;
}

NPath& NPath::operator/=(const char* Other)
{
	return *this /= TString(Other ? Other : "");
}

// === 路径解析 ===

TString NPath::GetFileName() const
{
	if (PathString.IsEmpty())
	{
		return TString();
	}

	try
	{
		std::filesystem::path StdPath = ToStdPath();
		return TString(StdPath.filename().string().c_str());
	}
	catch (...)
	{
		// 手动解析
		int32_t LastSeparator = PathString.LastIndexOfAny(FPathConstants::DirectorySeparatorString,
		                                                  FPathConstants::AltDirectorySeparatorString);

		if (LastSeparator >= 0)
		{
			return PathString.Substring(LastSeparator + 1);
		}

		return PathString;
	}
}

TString NPath::GetFileNameWithoutExtension() const
{
	TString FileName = GetFileName();
	int32_t DotIndex = FileName.LastIndexOf('.');

	if (DotIndex > 0) // 不包括以.开头的文件
	{
		return FileName.Substring(0, DotIndex);
	}

	return FileName;
}

TString NPath::GetExtension() const
{
	TString FileName = GetFileName();
	int32_t DotIndex = FileName.LastIndexOf('.');

	if (DotIndex > 0 && DotIndex < FileName.Length() - 1)
	{
		return FileName.Substring(DotIndex);
	}

	return TString();
}

NPath NPath::GetDirectoryName() const
{
	if (PathString.IsEmpty())
	{
		return NPath();
	}

	try
	{
		std::filesystem::path StdPath = ToStdPath();
		return NPath(StdPath.parent_path().string().c_str());
	}
	catch (...)
	{
		// 手动解析
		int32_t LastSeparator = PathString.LastIndexOfAny(FPathConstants::DirectorySeparatorString,
		                                                  FPathConstants::AltDirectorySeparatorString);

		if (LastSeparator > 0)
		{
			return NPath(PathString.Substring(0, LastSeparator));
		}

		return NPath();
	}
}

NPath NPath::GetRoot() const
{
	if (PathString.IsEmpty())
	{
		return NPath();
	}

	try
	{
		std::filesystem::path StdPath = ToStdPath();
		return NPath(StdPath.root_path().string().c_str());
	}
	catch (...)
	{
#if defined(_WIN32) || defined(_WIN64)
		// Windows: 查找盘符
		if (PathString.Length() >= 2 && PathString[1] == FPathConstants::VolumeSeparator)
		{
			return NPath(PathString.Substring(0, 2) + FPathConstants::DirectorySeparatorString);
		}
#endif
		// Unix: 根目录是 /
		if (PathString.StartsWith(FPathConstants::DirectorySeparatorString))
		{
			return NPath(FPathConstants::DirectorySeparatorString);
		}

		return NPath();
	}
}

TArray<TString, CMemoryManager> NPath::GetComponents() const
{
	return SplitPath();
}

// === 路径操作 ===

NPath& NPath::Normalize()
{
	if (PathString.IsEmpty())
	{
		return *this;
	}

	// 替换路径分隔符
	PathString = NormalizeSeparators(PathString);

	// 使用std::filesystem进行标准化
	try
	{
		std::filesystem::path StdPath(PathString.GetData());
		StdPath = StdPath.lexically_normal();
		PathString = TString(StdPath.string().c_str());
	}
	catch (...)
	{
		// 手动标准化
		auto Components = SplitPath();
		TArray<TString, CMemoryManager> NormalizedComponents;

		for (const auto& Component : Components)
		{
			if (Component == FPathConstants::CurrentDirectory)
			{
				// 忽略 "."
				continue;
			}
			else if (Component == FPathConstants::ParentDirectory)
			{
				// 处理 ".."
				if (!NormalizedComponents.IsEmpty() && NormalizedComponents.Last() != FPathConstants::ParentDirectory)
				{
					NormalizedComponents.RemoveLast();
				}
				else
				{
					NormalizedComponents.Add(Component);
				}
			}
			else
			{
				NormalizedComponents.Add(Component);
			}
		}

		// 重建路径
		PathString.Clear();
		for (int32_t i = 0; i < NormalizedComponents.Size(); ++i)
		{
			if (i > 0)
			{
				PathString += FPathConstants::DirectorySeparatorString;
			}
			PathString += NormalizedComponents[i];
		}
	}

	return *this;
}

NPath NPath::GetNormalized() const
{
	NPath Result(*this);
	return Result.Normalize();
}

NPath& NPath::MakeAbsolute()
{
	if (!IsAbsolute())
	{
		*this = GetCurrentDirectory() / *this;
		Normalize();
	}
	return *this;
}

NPath NPath::GetAbsolute() const
{
	NPath Result(*this);
	return Result.MakeAbsolute();
}

NPath NPath::GetRelative(const NPath& BasePath) const
{
	try
	{
		std::filesystem::path ThisPath = ToStdPath();
		std::filesystem::path BaseStdPath = BasePath.ToStdPath();

		auto RelativePath = std::filesystem::relative(ThisPath, BaseStdPath);
		return NPath(RelativePath.string().c_str());
	}
	catch (...)
	{
		// 手动计算相对路径
		NPath AbsThis = GetAbsolute();
		NPath AbsBase = BasePath.GetAbsolute();

		auto ThisComponents = AbsThis.GetComponents();
		auto BaseComponents = AbsBase.GetComponents();

		// 找到公共前缀
		int32_t CommonPrefix = 0;
		int32_t MinSize = std::min(ThisComponents.Size(), BaseComponents.Size());

		for (int32_t i = 0; i < MinSize; ++i)
		{
			if (ThisComponents[i] == BaseComponents[i])
			{
				CommonPrefix++;
			}
			else
			{
				break;
			}
		}

		// 构建相对路径
		NPath Result;

		// 添加回退路径
		for (int32_t i = CommonPrefix; i < BaseComponents.Size(); ++i)
		{
			Result /= FPathConstants::ParentDirectory;
		}

		// 添加前进路径
		for (int32_t i = CommonPrefix; i < ThisComponents.Size(); ++i)
		{
			Result /= ThisComponents[i];
		}

		return Result;
	}
}

NPath& NPath::ChangeExtension(const TString& NewExtension)
{
	TString FileName = GetFileNameWithoutExtension();
	NPath Directory = GetDirectoryName();

	PathString = (Directory / (FileName + NewExtension)).ToString();
	return *this;
}

NPath NPath::WithExtension(const TString& NewExtension) const
{
	NPath Result(*this);
	return Result.ChangeExtension(NewExtension);
}

// === 路径检查 ===

bool NPath::IsAbsolute() const
{
	if (PathString.IsEmpty())
	{
		return false;
	}

	try
	{
		std::filesystem::path StdPath = ToStdPath();
		return StdPath.is_absolute();
	}
	catch (...)
	{
#if defined(_WIN32) || defined(_WIN64)
		// Windows: 检查盘符或UNC路径
		return (PathString.Length() >= 2 && PathString[1] == FPathConstants::VolumeSeparator) ||
		       PathString.StartsWith("\\\\");
#else
		// Unix: 检查是否以/开头
		return PathString.StartsWith(FPathConstants::DirectorySeparatorString);
#endif
	}
}

bool NPath::IsRelative() const
{
	return !IsAbsolute();
}

bool NPath::IsFileName() const
{
	return !PathString.Contains(FPathConstants::DirectorySeparatorString) &&
	       !PathString.Contains(FPathConstants::AltDirectorySeparatorString);
}

bool NPath::IsValid() const
{
	return !PathString.IsEmpty() && !HasInvalidCharacters();
}

bool NPath::HasInvalidCharacters() const
{
	for (int32_t i = 0; i < PathString.Length(); ++i)
	{
		if (IsInvalidPathChar(PathString[i]))
		{
			return true;
		}
	}
	return false;
}

// === 静态工具方法 ===

NPath NPath::GetCurrentDirectory()
{
	try
	{
		return NPath(std::filesystem::current_path().string().c_str());
	}
	catch (...)
	{
		NLOG_IO(Error, "Failed to get current directory");
		return NPath(".");
	}
}

NPath NPath::GetTempDirectory()
{
	try
	{
		return NPath(std::filesystem::temp_directory_path().string().c_str());
	}
	catch (...)
	{
#if defined(_WIN32) || defined(_WIN64)
		const char* TempDir = std::getenv("TEMP");
		if (!TempDir)
			TempDir = std::getenv("TMP");
		if (!TempDir)
			TempDir = "C:\\Temp";
#else
		const char* TempDir = std::getenv("TMPDIR");
		if (!TempDir)
			TempDir = "/tmp";
#endif
		return NPath(TempDir);
	}
}

NPath NPath::GetUserDirectory()
{
#if defined(_WIN32) || defined(_WIN64)
	const char* UserDir = std::getenv("USERPROFILE");
	if (!UserDir)
		UserDir = std::getenv("HOMEDRIVE");
#else
	const char* UserDir = std::getenv("HOME");
#endif
	return NPath(UserDir ? UserDir : "");
}

NPath NPath::GetApplicationDirectory()
{
	try
	{
		// 获取当前可执行文件的目录
#if defined(__APPLE__)
		// macOS实现
		return GetCurrentDirectory();
#elif defined(__linux__)
		// Linux实现
		std::filesystem::path ExePath = std::filesystem::canonical("/proc/self/exe");
		return NPath(ExePath.parent_path().string().c_str());
#else
		// 其他平台，返回当前目录
		return GetCurrentDirectory();
#endif
	}
	catch (...)
	{
		return GetCurrentDirectory();
	}
}

bool NPath::IsSeparator(char Ch)
{
	return Ch == FPathConstants::DirectorySeparator || Ch == FPathConstants::AltDirectorySeparator;
}

bool NPath::IsInvalidPathChar(char Ch)
{
#if defined(_WIN32) || defined(_WIN64)
	// Windows无效字符
	return Ch < 32 || Ch == '<' || Ch == '>' || Ch == '|' || Ch == '"' || Ch == '*' || Ch == '?';
#else
	// Unix只有空字符和斜杠在某些上下文中无效
	return Ch == '\0';
#endif
}

TString NPath::NormalizeSeparators(const TString& Path)
{
	TString Result = Path;

	// 替换所有分隔符为当前平台的标准分隔符
	for (int32_t i = 0; i < Result.Length(); ++i)
	{
		if (IsSeparator(Result[i]))
		{
			Result[i] = FPathConstants::DirectorySeparator;
		}
	}

	return Result;
}

NPath NPath::GetCommonPrefix(const NPath& Path1, const NPath& Path2)
{
	auto Components1 = Path1.GetComponents();
	auto Components2 = Path2.GetComponents();

	NPath CommonPath;
	int32_t MinSize = std::min(Components1.Size(), Components2.Size());

	for (int32_t i = 0; i < MinSize; ++i)
	{
		if (Components1[i] == Components2[i])
		{
			CommonPath /= Components1[i];
		}
		else
		{
			break;
		}
	}

	return CommonPath;
}

// === 内部实现 ===

TArray<TString, CMemoryManager> NPath::SplitPath() const
{
	TArray<TString, CMemoryManager> Components;

	if (PathString.IsEmpty())
	{
		return Components;
	}

	const char* Data = PathString.GetData();
	const char* Current = Data;
	const char* End = Data + PathString.Length();

	while (Current < End)
	{
		// 跳过分隔符
		while (Current < End && IsSeparator(*Current))
		{
			Current++;
		}

		if (Current >= End)
		{
			break;
		}

		// 查找下一个分隔符
		const char* ComponentStart = Current;
		while (Current < End && !IsSeparator(*Current))
		{
			Current++;
		}

		// 添加组件
		if (Current > ComponentStart)
		{
			TString Component(ComponentStart, static_cast<int32_t>(Current - ComponentStart));
			Components.Add(Component);
		}
	}

	return Components;
}

void NPath::FromStdPath(const std::filesystem::path& StdPath)
{
	PathString = TString(StdPath.string().c_str());
}

std::filesystem::path NPath::ToStdPath() const
{
	return std::filesystem::path(PathString.GetData());
}

} // namespace NLib
