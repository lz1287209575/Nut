#pragma once

#include "CContainer.h"
#include "NLibMacros.h"
#include "CString.generate.h"

#include <cstring>
#include <string_view>

namespace NLib
{
NCLASS()
class LIBNLIB_API CString : public CContainer
{
	GENERATED_BODY()

public:
	using Iterator = CIteratorBase<char, CString>;
	using ConstIterator = CIteratorBase<const char, CString>;

	static constexpr size_t SSO_BUFFER_SIZE = 23; // Small String Optimization
	static constexpr size_t NPOS = static_cast<size_t>(-1);

	// 构造函数
	CString();
	CString(const char* str);
	CString(const char* str, size_t length);
	CString(const CString& other);
	CString(CString&& other) noexcept;
	CString(std::string_view sv);
	explicit CString(size_t count, char ch = '\0');

	// 析构函数
	virtual ~CString();

	// 赋值操作符
	CString& operator=(const CString& other);
	CString& operator=(CString&& other) noexcept;
	CString& operator=(const char* str);
	CString& operator=(std::string_view sv);

	// 基础访问
	char& operator[](size_t index);
	const char& operator[](size_t index) const;
	char& At(size_t index);
	const char& At(size_t index) const;

	char& Front()
	{
		return Data[0];
	}
	const char& Front() const
	{
		return Data[0];
	}
	char& Back()
	{
		return Data[Size - 1];
	}
	const char& Back() const
	{
		return Data[Size - 1];
	}

	const char* GetData() const
	{
		return Data;
	}
	char* GetData()
	{
		return Data;
	}
	const char* CStr() const
	{
		return Data;
	}

	// 容器接口实现
	size_t GetSize() const override
	{
		return Size;
	}
	size_t GetCapacity() const override
	{
		return Capacity;
	}
	bool IsEmpty() const override
	{
		return Size == 0;
	}
	void Clear() override;

	// 字符串特有功能
	size_t Length() const
	{
		return Size;
	}
	void Reserve(size_t NewCapacity);
	void Resize(size_t NewSize, char FillChar = '\0');
	void ShrinkToFit();

	// 字符串操作
	CString& Append(const char* str);
	CString& Append(const char* str, size_t length);
	CString& Append(const CString& other);
	CString& Append(std::string_view sv);
	CString& Append(size_t count, char ch);

	CString& Insert(size_t pos, const char* str);
	CString& Insert(size_t pos, const char* str, size_t length);
	CString& Insert(size_t pos, const CString& other);
	CString& Insert(size_t pos, size_t count, char ch);

	CString& Erase(size_t pos, size_t length = NPOS);

	void PushBack(char ch);
	void PopBack();

	// 查找功能
	size_t Find(const char* str, size_t pos = 0) const;
	size_t Find(const CString& other, size_t pos = 0) const;
	size_t Find(char ch, size_t pos = 0) const;

	size_t RFind(const char* str, size_t pos = NPOS) const;
	size_t RFind(const CString& other, size_t pos = NPOS) const;
	size_t RFind(char ch, size_t pos = NPOS) const;

	size_t FindFirstOf(const char* chars, size_t pos = 0) const;
	size_t FindFirstNotOf(const char* chars, size_t pos = 0) const;
	size_t FindLastOf(const char* chars, size_t pos = NPOS) const;
	size_t FindLastNotOf(const char* chars, size_t pos = NPOS) const;

	// 子字符串
	CString Substring(size_t pos, size_t length = NPOS) const;

	// 比较
	int Compare(const char* str) const;
	int Compare(const CString& other) const;
	int Compare(std::string_view sv) const;

	bool StartsWith(const char* prefix) const;
	bool StartsWith(const CString& prefix) const;
	bool EndsWith(const char* suffix) const;
	bool EndsWith(const CString& suffix) const;

	// 修改操作
	CString& ToLower();
	CString& ToUpper();
	CString& Trim();
	CString& TrimLeft();
	CString& TrimRight();

	CString ToLowerCopy() const;
	CString ToUpperCopy() const;
	CString TrimCopy() const;

	// 分割和连接 (前向声明，实现在NString.cpp中)
	// CArray<CString> Split(char delimiter) const;
	// CArray<CString> Split(const char* delimiter) const;
	// static CString Join(const CArray<CString>& strings, const char* separator);

	// 格式化
	static CString Format(const char* format, ...);
	static CString VFormat(const char* format, va_list args);

	// 类型转换
	int32_t ToInt32(int base = 10) const;
	int64_t ToInt64(int base = 10) const;
	float ToFloat() const;
	double ToDouble() const;
	bool ToBool() const;

	static CString FromInt32(int32_t value);
	static CString FromInt64(int64_t value);
	static CString FromFloat(float value, int precision = 6);
	static CString FromDouble(double value, int precision = 6);
	static CString FromBool(bool value);

	// UTF-8 支持
	size_t GetCharacterCount() const; // 字符数（非字节数）
	CString GetCharacterAt(size_t char_index) const;
	bool IsValidUtf8() const;

	// 迭代器
	Iterator Begin()
	{
		return Iterator(Data);
	}
	Iterator End()
	{
		return Iterator(Data + Size);
	}
	ConstIterator Begin() const
	{
		return ConstIterator(Data);
	}
	ConstIterator End() const
	{
		return ConstIterator(Data + Size);
	}
	ConstIterator CBegin() const
	{
		return ConstIterator(Data);
	}
	ConstIterator CEnd() const
	{
		return ConstIterator(Data + Size);
	}

	// C++ range-based for loop support (lowercase methods)
	Iterator begin()
	{
		return Begin();
	}
	Iterator end()
	{
		return End();
	}
	ConstIterator begin() const
	{
		return Begin();
	}
	ConstIterator end() const
	{
		return End();
	}

	// 运算符重载
	CString operator+(const char* str) const;
	CString operator+(const CString& other) const;
	CString operator+(char ch) const;

	CString& operator+=(const char* str);
	CString& operator+=(const CString& other);
	CString& operator+=(char ch);

	bool operator==(const char* str) const;
	bool operator==(const CString& other) const;
	bool operator!=(const char* str) const;
	bool operator!=(const CString& other) const;
	bool operator<(const CString& other) const;
	bool operator<=(const CString& other) const;
	bool operator>(const CString& other) const;
	bool operator>=(const CString& other) const;

	// CObject 接口重写
	virtual bool Equals(const CObject* other) const override;
	virtual size_t GetHashCode() const override;
	virtual CString ToString() const override;

	// 静态常量
	static const CString EMPTY;

private:
	char* Data;
	size_t Size;
	size_t Capacity;

	// Small String Optimization 缓冲区
	alignas(char*) char SsoBuffer[SSO_BUFFER_SIZE + 1];

	// 内部工具函数
	bool IsUsingSSO() const
	{
		return Data == SsoBuffer;
	}
	void EnsureCapacity(size_t RequiredCapacity);
	void SetSize(size_t NewSize);
	void InitializeFromCString(const char* str, size_t length);
	void MoveFrom(CString&& other) noexcept;
	void CopyFrom(const CString& other);
	void Deallocate();

	// UTF-8 工具函数
	static bool IsUtf8StartByte(char Byte);
	static size_t GetUtf8CharLength(char FirstByte);
	static bool IsValidUtf8Sequence(const char* Str, size_t Length);
};
} // namespace NLib
