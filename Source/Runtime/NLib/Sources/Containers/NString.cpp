#include "CString.h"
#include "CString.generate.h"

#include "NLib.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>

namespace NLib
{

// 静态常量定义
const CString CString::EMPTY = CString();

// 构造函数实现
CString::CString()
    : Data(SsoBuffer),
      Size(0),
      Capacity(SSO_BUFFER_SIZE)
{
	SsoBuffer[0] = '\0';
}

CString::CString(const char* str)
    : Data(SsoBuffer),
      Size(0),
      Capacity(SSO_BUFFER_SIZE)
{
	if (str)
	{
		size_t length = strlen(str);
		InitializeFromCString(str, length);
	}
	else
	{
		SsoBuffer[0] = '\0';
	}
}

CString::CString(const char* str, size_t length)
    : Data(SsoBuffer),
      Size(0),
      Capacity(SSO_BUFFER_SIZE)
{
	if (str && length > 0)
	{
		InitializeFromCString(str, length);
	}
	else
	{
		SsoBuffer[0] = '\0';
	}
}

CString::CString(const CString& other)
    : Data(SsoBuffer),
      Size(0),
      Capacity(SSO_BUFFER_SIZE)
{
	CopyFrom(other);
}

CString::CString(CString&& other) noexcept
    : Data(SsoBuffer),
      Size(0),
      Capacity(SSO_BUFFER_SIZE)
{
	MoveFrom(std::move(other));
}

CString::CString(std::string_view sv)
    : Data(SsoBuffer),
      Size(0),
      Capacity(SSO_BUFFER_SIZE)
{
	if (!sv.empty())
	{
		InitializeFromCString(sv.data(), sv.size());
	}
	else
	{
		SsoBuffer[0] = '\0';
	}
}

CString::CString(size_t count, char ch)
    : Data(SsoBuffer),
      Size(0),
      Capacity(SSO_BUFFER_SIZE)
{
	if (count > 0)
	{
		EnsureCapacity(count);
		std::fill_n(Data, count, ch);
		SetSize(count);
	}
	else
	{
		SsoBuffer[0] = '\0';
	}
}

// 析构函数
CString::~CString()
{
	Deallocate();
}

// 赋值操作符
CString& CString::operator=(const CString& other)
{
	if (this != &other)
	{
		CopyFrom(other);
	}
	return *this;
}

CString& CString::operator=(CString&& other) noexcept
{
	if (this != &other)
	{
		MoveFrom(std::move(other));
	}
	return *this;
}

CString& CString::operator=(const char* str)
{
	if (str)
	{
		size_t length = strlen(str);
		if (length <= Capacity)
		{
			memcpy(Data, str, length);
			SetSize(length);
		}
		else
		{
			Deallocate();
			Data = SsoBuffer;
			Size = 0;
			Capacity = SSO_BUFFER_SIZE;
			InitializeFromCString(str, length);
		}
	}
	else
	{
		Clear();
	}
	return *this;
}

CString& CString::operator=(std::string_view sv)
{
	if (!sv.empty())
	{
		if (sv.size() <= Capacity)
		{
			memcpy(Data, sv.data(), sv.size());
			SetSize(sv.size());
		}
		else
		{
			Deallocate();
			Data = SsoBuffer;
			Size = 0;
			Capacity = SSO_BUFFER_SIZE;
			InitializeFromCString(sv.data(), sv.size());
		}
	}
	else
	{
		Clear();
	}
	return *this;
}

// 基础访问
char& CString::operator[](size_t index)
{
	LIBNUT_ASSERT(index < Size, "CString index out of bounds");
	return Data[index];
}

const char& CString::operator[](size_t index) const
{
	LIBNUT_ASSERT(index < Size, "CString index out of bounds");
	return Data[index];
}

char& CString::At(size_t index)
{
	if (index >= Size)
	{
		NLIB_LOG_ERROR("CString::At() index {} out of bounds (size: {})", index, Size);
		throw std::out_of_range("CString index out of bounds");
	}
	return Data[index];
}

const char& CString::At(size_t index) const
{
	if (index >= Size)
	{
		NLIB_LOG_ERROR("CString::At() index {} out of bounds (size: {})", index, Size);
		throw std::out_of_range("CString index out of bounds");
	}
	return Data[index];
}

// 容器接口实现
void CString::Clear()
{
	SetSize(0);
}

void CString::Reserve(size_t NewCapacity)
{
	if (NewCapacity > Capacity)
	{
		EnsureCapacity(NewCapacity);
	}
}

void CString::Resize(size_t NewSize, char FillChar)
{
	if (NewSize > Size)
	{
		EnsureCapacity(NewSize);
		std::fill(Data + Size, Data + NewSize, FillChar);
	}
	SetSize(NewSize);
}

void CString::ShrinkToFit()
{
	if (!IsUsingSSO() && Size <= SSO_BUFFER_SIZE)
	{
		// 如果字符串足够小，移回SSO缓冲区
		char* OldData = Data;
		size_t OldSize = Size;

		memcpy(SsoBuffer, Data, Size);
		SsoBuffer[Size] = '\0';

		Data = SsoBuffer;
		Capacity = SSO_BUFFER_SIZE;

		CContainer::DeallocateElements<char>(OldData, OldSize + 1);
	}
}

// 字符串操作
CString& CString::Append(const char* str)
{
	if (str)
	{
		return Append(str, strlen(str));
	}
	return *this;
}

CString& CString::Append(const char* str, size_t length)
{
	if (str && length > 0)
	{
		EnsureCapacity(Size + length);
		memcpy(Data + Size, str, length);
		SetSize(Size + length);
	}
	return *this;
}

CString& CString::Append(const CString& other)
{
	return Append(other.Data, other.Size);
}

CString& CString::Append(std::string_view sv)
{
	return Append(sv.data(), sv.size());
}

CString& CString::Append(size_t count, char ch)
{
	if (count > 0)
	{
		EnsureCapacity(Size + count);
		std::fill_n(Data + Size, count, ch);
		SetSize(Size + count);
	}
	return *this;
}

void CString::PushBack(char ch)
{
	EnsureCapacity(Size + 1);
	Data[Size] = ch;
	SetSize(Size + 1);
}

void CString::PopBack()
{
	if (Size > 0)
	{
		SetSize(Size - 1);
	}
}

// Insert 操作实现
CString& CString::Insert(size_t pos, const char* str)
{
	if (str)
	{
		return Insert(pos, str, strlen(str));
	}
	return *this;
}

CString& CString::Insert(size_t pos, const char* str, size_t length)
{
	if (!str || length == 0 || pos > Size)
		return *this;

	EnsureCapacity(Size + length);

	// 移动现有数据
	if (pos < Size)
	{
		memmove(Data + pos + length, Data + pos, Size - pos);
	}

	// 插入新数据
	memcpy(Data + pos, str, length);
	SetSize(Size + length);

	return *this;
}

CString& CString::Insert(size_t pos, const CString& other)
{
	return Insert(pos, other.Data, other.Size);
}

CString& CString::Insert(size_t pos, size_t count, char ch)
{
	if (count == 0 || pos > Size)
		return *this;

	EnsureCapacity(Size + count);

	// 移动现有数据
	if (pos < Size)
	{
		memmove(Data + pos + count, Data + pos, Size - pos);
	}

	// 插入字符
	std::fill_n(Data + pos, count, ch);
	SetSize(Size + count);

	return *this;
}

// Erase 操作实现
CString& CString::Erase(size_t pos, size_t length)
{
	if (pos >= Size)
		return *this;

	if (length == NPOS || pos + length > Size)
	{
		length = Size - pos;
	}

	if (length > 0)
	{
		// 移动数据
		if (pos + length < Size)
		{
			memmove(Data + pos, Data + pos + length, Size - pos - length);
		}

		SetSize(Size - length);
	}

	return *this;
}

// 查找功能实现
size_t CString::Find(const char* str, size_t pos) const
{
	if (!str || pos >= Size)
		return NPOS;

	const char* found = strstr(Data + pos, str);
	return found ? (found - Data) : NPOS;
}

size_t CString::Find(const CString& other, size_t pos) const
{
	return Find(other.Data, pos);
}

size_t CString::Find(char ch, size_t pos) const
{
	if (pos >= Size)
		return NPOS;

	const char* found = strchr(Data + pos, ch);
	return found ? (found - Data) : NPOS;
}

size_t CString::RFind(const char* str, size_t pos) const
{
	if (!str || Size == 0)
		return NPOS;

	size_t str_len = strlen(str);
	if (str_len > Size)
		return NPOS;

	if (pos == NPOS || pos > Size - str_len)
		pos = Size - str_len;

	for (size_t i = pos + 1; i > 0; --i)
	{
		if (memcmp(Data + i - 1, str, str_len) == 0)
		{
			return i - 1;
		}
	}

	return NPOS;
}

size_t CString::RFind(const CString& other, size_t pos) const
{
	return RFind(other.Data, pos);
}

size_t CString::RFind(char ch, size_t pos) const
{
	if (Size == 0)
		return NPOS;

	if (pos == NPOS || pos >= Size)
		pos = Size - 1;

	for (size_t i = pos + 1; i > 0; --i)
	{
		if (Data[i - 1] == ch)
		{
			return i - 1;
		}
	}

	return NPOS;
}

// 子字符串实现
CString CString::Substring(size_t pos, size_t length) const
{
	if (pos >= Size)
		return CString();

	if (length == NPOS || pos + length > Size)
	{
		length = Size - pos;
	}

	return CString(Data + pos, length);
}

// 比较
int CString::Compare(const char* str) const
{
	if (!str)
		return Size > 0 ? 1 : 0;
	return strcmp(Data, str);
}

int CString::Compare(const CString& other) const
{
	return strcmp(Data, other.Data);
}

int CString::Compare(std::string_view sv) const
{
	size_t min_len = std::min(Size, sv.size());
	int result = memcmp(Data, sv.data(), min_len);
	if (result == 0)
	{
		if (Size < sv.size())
			return -1;
		if (Size > sv.size())
			return 1;
	}
	return result;
}

bool CString::StartsWith(const char* prefix) const
{
	if (!prefix)
		return true;
	size_t prefix_len = strlen(prefix);
	return Size >= prefix_len && memcmp(Data, prefix, prefix_len) == 0;
}

bool CString::StartsWith(const CString& prefix) const
{
	return Size >= prefix.Size && memcmp(Data, prefix.Data, prefix.Size) == 0;
}

bool CString::EndsWith(const char* suffix) const
{
	if (!suffix)
		return true;
	size_t suffix_len = strlen(suffix);
	return Size >= suffix_len && memcmp(Data + Size - suffix_len, suffix, suffix_len) == 0;
}

bool CString::EndsWith(const CString& suffix) const
{
	return Size >= suffix.Size && memcmp(Data + Size - suffix.Size, suffix.Data, suffix.Size) == 0;
}

// 修改操作
CString& CString::ToLower()
{
	for (size_t i = 0; i < Size; ++i)
	{
		Data[i] = std::tolower(Data[i]);
	}
	return *this;
}

CString& CString::ToUpper()
{
	for (size_t i = 0; i < Size; ++i)
	{
		Data[i] = std::toupper(Data[i]);
	}
	return *this;
}

CString CString::ToLowerCopy() const
{
	CString result(*this);
	return result.ToLower();
}

CString CString::ToUpperCopy() const
{
	CString result(*this);
	return result.ToUpper();
}

// 格式化
CString CString::Format(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	CString result = VFormat(format, args);
	va_end(args);
	return result;
}

CString CString::VFormat(const char* format, va_list args)
{
	// 首先获取需要的缓冲区大小
	va_list args_copy;
	va_copy(args_copy, args);
	int size = vsnprintf(nullptr, 0, format, args_copy);
	va_end(args_copy);

	if (size <= 0)
		return CString();

	CString result;
	result.EnsureCapacity(size);
	vsnprintf(result.Data, size + 1, format, args);
	result.SetSize(size);

	return result;
}

// 类型转换实现
int32_t CString::ToInt32(int base) const
{
	if (Size == 0)
		return 0;
	return static_cast<int32_t>(strtol(Data, nullptr, base));
}

int64_t CString::ToInt64(int base) const
{
	if (Size == 0)
		return 0;
	return strtoll(Data, nullptr, base);
}

float CString::ToFloat() const
{
	if (Size == 0)
		return 0.0f;
	return strtof(Data, nullptr);
}

double CString::ToDouble() const
{
	if (Size == 0)
		return 0.0;
	return strtod(Data, nullptr);
}

bool CString::ToBool() const
{
	if (Size == 0)
		return false;
	CString lower = ToLowerCopy();
	return lower == "true" || lower == "1" || lower == "yes";
}

CString CString::FromInt32(int32_t value)
{
	return Format("%d", value);
}

CString CString::FromInt64(int64_t value)
{
	return Format("%lld", value);
}

CString CString::FromFloat(float value, int precision)
{
	return Format("%.*f", precision, value);
}

CString CString::FromDouble(double value, int precision)
{
	return Format("%.*lf", precision, value);
}

CString CString::FromBool(bool value)
{
	return value ? CString("true") : CString("false");
}

// Trim 实现
CString& CString::Trim()
{
	return TrimLeft().TrimRight();
}

CString& CString::TrimLeft()
{
	if (Size == 0)
		return *this;

	size_t start = 0;
	while (start < Size && std::isspace(Data[start]))
	{
		++start;
	}

	if (start > 0)
	{
		if (start >= Size)
		{
			Clear();
		}
		else
		{
			memmove(Data, Data + start, Size - start);
			SetSize(Size - start);
		}
	}

	return *this;
}

CString& CString::TrimRight()
{
	if (Size == 0)
		return *this;

	size_t end = Size;
	while (end > 0 && std::isspace(Data[end - 1]))
	{
		--end;
	}

	if (end < Size)
	{
		SetSize(end);
	}

	return *this;
}

CString CString::TrimCopy() const
{
	CString result(*this);
	return result.Trim();
}

// 查找字符集合的方法
size_t CString::FindFirstOf(const char* chars, size_t pos) const
{
	if (!chars || pos >= Size)
		return NPOS;

	for (size_t i = pos; i < Size; ++i)
	{
		if (strchr(chars, Data[i]))
		{
			return i;
		}
	}
	return NPOS;
}

size_t CString::FindFirstNotOf(const char* chars, size_t pos) const
{
	if (!chars || pos >= Size)
		return NPOS;

	for (size_t i = pos; i < Size; ++i)
	{
		if (!strchr(chars, Data[i]))
		{
			return i;
		}
	}
	return NPOS;
}

size_t CString::FindLastOf(const char* chars, size_t pos) const
{
	if (!chars || Size == 0)
		return NPOS;

	if (pos == NPOS || pos >= Size)
		pos = Size - 1;

	for (size_t i = pos + 1; i > 0; --i)
	{
		if (strchr(chars, Data[i - 1]))
		{
			return i - 1;
		}
	}
	return NPOS;
}

size_t CString::FindLastNotOf(const char* chars, size_t pos) const
{
	if (!chars || Size == 0)
		return NPOS;

	if (pos == NPOS || pos >= Size)
		pos = Size - 1;

	for (size_t i = pos + 1; i > 0; --i)
	{
		if (!strchr(chars, Data[i - 1]))
		{
			return i - 1;
		}
	}
	return NPOS;
}

// UTF-8 支持实现
size_t CString::GetCharacterCount() const
{
	size_t count = 0;
	size_t i = 0;

	while (i < Size)
	{
		if (IsUtf8StartByte(Data[i]))
		{
			size_t charLen = GetUtf8CharLength(Data[i]);
			if (i + charLen <= Size && IsValidUtf8Sequence(Data + i, charLen))
			{
				i += charLen;
				++count;
			}
			else
			{
				// 无效UTF-8序列，跳过一个字节
				++i;
				++count;
			}
		}
		else
		{
			++i;
			++count;
		}
	}

	return count;
}

bool CString::IsValidUtf8() const
{
	size_t i = 0;

	while (i < Size)
	{
		if (IsUtf8StartByte(Data[i]))
		{
			size_t charLen = GetUtf8CharLength(Data[i]);
			if (i + charLen > Size || !IsValidUtf8Sequence(Data + i, charLen))
			{
				return false;
			}
			i += charLen;
		}
		else if ((Data[i] & 0x80) != 0)
		{
			// 非ASCII且不是UTF-8起始字节
			return false;
		}
		else
		{
			++i;
		}
	}

	return true;
}

// UTF-8 工具函数实现
bool CString::IsUtf8StartByte(char byte)
{
	return (byte & 0x80) == 0 || (byte & 0xE0) == 0xC0 || (byte & 0xF0) == 0xE0 || (byte & 0xF8) == 0xF0;
}

size_t CString::GetUtf8CharLength(char firstByte)
{
	if ((firstByte & 0x80) == 0)
		return 1;
	if ((firstByte & 0xE0) == 0xC0)
		return 2;
	if ((firstByte & 0xF0) == 0xE0)
		return 3;
	if ((firstByte & 0xF8) == 0xF0)
		return 4;
	return 1; // 无效字节，返回1
}

bool CString::IsValidUtf8Sequence(const char* str, size_t length)
{
	if (length == 0)
		return false;

	// 检查第一个字节
	char first = str[0];

	if (length == 1)
	{
		return (first & 0x80) == 0; // ASCII
	}

	// 检查后续字节都是10xxxxxx格式
	for (size_t i = 1; i < length; ++i)
	{
		if ((str[i] & 0xC0) != 0x80)
		{
			return false;
		}
	}

	// 检查编码范围是否正确
	if (length == 2)
	{
		uint32_t codepoint = ((first & 0x1F) << 6) | (str[1] & 0x3F);
		return codepoint >= 0x80;
	}
	else if (length == 3)
	{
		uint32_t codepoint = ((first & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
		return codepoint >= 0x800 && (codepoint < 0xD800 || codepoint > 0xDFFF);
	}
	else if (length == 4)
	{
		uint32_t codepoint = ((first & 0x07) << 18) | ((str[1] & 0x3F) << 12) | ((str[2] & 0x3F) << 6) |
		                     (str[3] & 0x3F);
		return codepoint >= 0x10000 && codepoint <= 0x10FFFF;
	}

	return false;
}

// 运算符重载
CString CString::operator+(const char* str) const
{
	CString result(*this);
	return result.Append(str);
}

CString CString::operator+(const CString& other) const
{
	CString result(*this);
	return result.Append(other);
}

CString CString::operator+(char ch) const
{
	CString result(*this);
	result.PushBack(ch);
	return result;
}

CString& CString::operator+=(const char* str)
{
	return Append(str);
}

CString& CString::operator+=(const CString& other)
{
	return Append(other);
}

CString& CString::operator+=(char ch)
{
	PushBack(ch);
	return *this;
}

bool CString::operator==(const char* str) const
{
	return Compare(str) == 0;
}

bool CString::operator==(const CString& other) const
{
	return Size == other.Size && memcmp(Data, other.Data, Size) == 0;
}

bool CString::operator!=(const char* str) const
{
	return !(*this == str);
}

bool CString::operator!=(const CString& other) const
{
	return !(*this == other);
}

bool CString::operator<(const CString& other) const
{
	return Compare(other) < 0;
}

// NObject接口重写
bool CString::Equals(const CObject* other) const
{
	if (auto str = dynamic_cast<const CString*>(other))
	{
		return *this == *str;
	}
	return false;
}

size_t CString::GetHashCode() const
{
	// 简单的djb2哈希算法
	size_t hash = 5381;
	for (size_t i = 0; i < Size; ++i)
	{
		hash = ((hash << 5) + hash) + static_cast<unsigned char>(Data[i]);
	}
	return hash;
}

CString CString::ToString() const
{
	return *this;
}

// 内部工具函数实现
void CString::EnsureCapacity(size_t RequiredCapacity)
{
	if (RequiredCapacity <= Capacity)
		return;

	// 计算新容量，至少增长到当前的1.5倍
	size_t NewCapacity = std::max(RequiredCapacity, Capacity + Capacity / 2);
	NewCapacity = std::max(NewCapacity, static_cast<size_t>(MIN_CAPACITY));

	char* NewData = CContainer::AllocateElements<char>(NewCapacity + 1);
	if (Size > 0)
	{
		memcpy(NewData, Data, Size);
	}
	NewData[Size] = '\0';

	if (!IsUsingSSO())
	{
		CContainer::DeallocateElements<char>(Data, Capacity + 1);
	}

	Data = NewData;
	Capacity = NewCapacity;
}

void CString::SetSize(size_t NewSize)
{
	Size = NewSize;
	Data[Size] = '\0';
}

void CString::InitializeFromCString(const char* str, size_t length)
{
	if (length <= SSO_BUFFER_SIZE)
	{
		// 使用SSO缓冲区
		memcpy(SsoBuffer, str, length);
		SsoBuffer[length] = '\0';
		Data = SsoBuffer;
		Size = length;
		Capacity = SSO_BUFFER_SIZE;
	}
	else
	{
		// 分配新内存
		EnsureCapacity(length);
		memcpy(Data, str, length);
		SetSize(length);
	}
}

void CString::MoveFrom(CString&& other) noexcept
{
	if (other.IsUsingSSO())
	{
		// 其他对象使用SSO，需要复制数据
		memcpy(SsoBuffer, other.SsoBuffer, other.Size + 1);
		Data = SsoBuffer;
		Size = other.Size;
		Capacity = SSO_BUFFER_SIZE;
	}
	else
	{
		// 其他对象使用堆内存，直接移动指针
		if (!IsUsingSSO())
		{
			CContainer::DeallocateElements<char>(Data, Capacity + 1);
		}

		Data = other.Data;
		Size = other.Size;
		Capacity = other.Capacity;

		// 重置other对象为SSO状态
		other.Data = other.SsoBuffer;
		other.Size = 0;
		other.Capacity = SSO_BUFFER_SIZE;
		other.SsoBuffer[0] = '\0';
	}
}

void CString::CopyFrom(const CString& other)
{
	if (other.Size <= Capacity)
	{
		// 当前容量足够，直接复制
		memcpy(Data, other.Data, other.Size);
		SetSize(other.Size);
	}
	else
	{
		// 需要重新分配内存
		Deallocate();
		Data = SsoBuffer;
		Size = 0;
		Capacity = SSO_BUFFER_SIZE;
		InitializeFromCString(other.Data, other.Size);
	}
}

void CString::Deallocate()
{
	if (!IsUsingSSO())
	{
		CContainer::DeallocateElements<char>(Data, Capacity + 1);
		Data = SsoBuffer;
		Size = 0;
		Capacity = SSO_BUFFER_SIZE;
		SsoBuffer[0] = '\0';
	}
}

} // namespace NLib