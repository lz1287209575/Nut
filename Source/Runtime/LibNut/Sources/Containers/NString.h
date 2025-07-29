#pragma once

#include "NContainer.h"
#include <string_view>
#include <cstring>

NCLASS()
class LIBNLIB_API NString : public NContainer
{
    GENERATED_BODY()

public:
    using Iterator = NIteratorBase<char, NString>;
    using ConstIterator = NIteratorBase<const char, NString>;
    
    static constexpr size_t SSO_BUFFER_SIZE = 23; // Small String Optimization
    static constexpr size_t NPOS = static_cast<size_t>(-1);

    // 构造函数
    NString();
    NString(const char* str);
    NString(const char* str, size_t length);
    NString(const NString& other);
    NString(NString&& other) noexcept;
    NString(std::string_view sv);
    explicit NString(size_t count, char ch = '\0');
    
    // 析构函数
    virtual ~NString();
    
    // 赋值操作符
    NString& operator=(const NString& other);
    NString& operator=(NString&& other) noexcept;
    NString& operator=(const char* str);
    NString& operator=(std::string_view sv);
    
    // 基础访问
    char& operator[](size_t index);
    const char& operator[](size_t index) const;
    char& At(size_t index);
    const char& At(size_t index) const;
    
    char& Front() { return data_[0]; }
    const char& Front() const { return data_[0]; }
    char& Back() { return data_[size_ - 1]; }
    const char& Back() const { return data_[size_ - 1]; }
    
    const char* Data() const { return data_; }
    char* Data() { return data_; }
    const char* CStr() const { return data_; }
    
    // 容器接口实现
    size_t GetSize() const override { return size_; }
    size_t GetCapacity() const override { return capacity_; }
    bool IsEmpty() const override { return size_ == 0; }
    void Clear() override;
    
    // 字符串特有功能
    size_t Length() const { return size_; }
    void Reserve(size_t new_capacity);
    void Resize(size_t new_size, char fill_char = '\0');
    void ShrinkToFit();
    
    // 字符串操作
    NString& Append(const char* str);
    NString& Append(const char* str, size_t length);
    NString& Append(const NString& other);
    NString& Append(std::string_view sv);
    NString& Append(size_t count, char ch);
    
    NString& Insert(size_t pos, const char* str);
    NString& Insert(size_t pos, const char* str, size_t length);
    NString& Insert(size_t pos, const NString& other);
    NString& Insert(size_t pos, size_t count, char ch);
    
    NString& Erase(size_t pos, size_t length = NPOS);
    
    void PushBack(char ch);
    void PopBack();
    
    // 查找功能
    size_t Find(const char* str, size_t pos = 0) const;
    size_t Find(const NString& other, size_t pos = 0) const;
    size_t Find(char ch, size_t pos = 0) const;
    
    size_t RFind(const char* str, size_t pos = NPOS) const;
    size_t RFind(const NString& other, size_t pos = NPOS) const;
    size_t RFind(char ch, size_t pos = NPOS) const;
    
    size_t FindFirstOf(const char* chars, size_t pos = 0) const;
    size_t FindFirstNotOf(const char* chars, size_t pos = 0) const;
    size_t FindLastOf(const char* chars, size_t pos = NPOS) const;
    size_t FindLastNotOf(const char* chars, size_t pos = NPOS) const;
    
    // 子字符串
    NString Substring(size_t pos, size_t length = NPOS) const;
    
    // 比较
    int Compare(const char* str) const;
    int Compare(const NString& other) const;
    int Compare(std::string_view sv) const;
    
    bool StartsWith(const char* prefix) const;
    bool StartsWith(const NString& prefix) const;
    bool EndsWith(const char* suffix) const;
    bool EndsWith(const NString& suffix) const;
    
    // 修改操作
    NString& ToLower();
    NString& ToUpper();
    NString& Trim();
    NString& TrimLeft();
    NString& TrimRight();
    
    NString ToLowerCopy() const;
    NString ToUpperCopy() const;
    NString TrimCopy() const;
    
    // 分割和连接 (前向声明，实现在NString.cpp中)
    // NArray<NString> Split(char delimiter) const;
    // NArray<NString> Split(const char* delimiter) const;
    // static NString Join(const NArray<NString>& strings, const char* separator);
    
    // 格式化
    static NString Format(const char* format, ...);
    static NString VFormat(const char* format, va_list args);
    
    // 类型转换
    int32_t ToInt32(int base = 10) const;
    int64_t ToInt64(int base = 10) const;
    float ToFloat() const;
    double ToDouble() const;
    bool ToBool() const;
    
    static NString FromInt32(int32_t value);
    static NString FromInt64(int64_t value);
    static NString FromFloat(float value, int precision = 6);
    static NString FromDouble(double value, int precision = 6);
    static NString FromBool(bool value);
    
    // UTF-8 支持
    size_t GetCharacterCount() const; // 字符数（非字节数）
    NString GetCharacterAt(size_t char_index) const;
    bool IsValidUtf8() const;
    
    // 迭代器
    Iterator Begin() { return Iterator(data_); }
    Iterator End() { return Iterator(data_ + size_); }
    ConstIterator Begin() const { return ConstIterator(data_); }
    ConstIterator End() const { return ConstIterator(data_ + size_); }
    ConstIterator CBegin() const { return ConstIterator(data_); }
    ConstIterator CEnd() const { return ConstIterator(data_ + size_); }
    
    // 运算符重载
    NString operator+(const char* str) const;
    NString operator+(const NString& other) const;
    NString operator+(char ch) const;
    
    NString& operator+=(const char* str);
    NString& operator+=(const NString& other);
    NString& operator+=(char ch);
    
    bool operator==(const char* str) const;
    bool operator==(const NString& other) const;
    bool operator!=(const char* str) const;
    bool operator!=(const NString& other) const;
    bool operator<(const NString& other) const;
    bool operator<=(const NString& other) const;
    bool operator>(const NString& other) const;
    bool operator>=(const NString& other) const;
    
    // NObject 接口重写
    virtual bool Equals(const NObject* other) const override;
    virtual size_t GetHashCode() const override;
    virtual NString ToString() const override;
    
    // 静态常量
    static const NString EMPTY;

private:
    char* data_;
    size_t size_;
    size_t capacity_;
    
    // Small String Optimization 缓冲区
    alignas(char*) char sso_buffer_[SSO_BUFFER_SIZE + 1];
    
    // 内部工具函数
    bool IsUsingSSO() const { return data_ == sso_buffer_; }
    void EnsureCapacity(size_t required_capacity);
    void SetSize(size_t new_size);
    void InitializeFromCString(const char* str, size_t length);
    void MoveFrom(NString&& other) noexcept;
    void CopyFrom(const NString& other);
    void Deallocate();
    
    // UTF-8 工具函数
    static bool IsUtf8StartByte(char byte);
    static size_t GetUtf8CharLength(char first_byte);
    static bool IsValidUtf8Sequence(const char* str, size_t length);
};