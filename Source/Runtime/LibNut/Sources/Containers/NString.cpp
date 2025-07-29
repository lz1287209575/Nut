#include "NString.h"
#include "Logging/NLogger.h"
#include <algorithm>
#include <cstdarg>
#include <cstdlib>
#include <cctype>
#include <cmath>

// 静态常量定义
const NString NString::EMPTY;

// 构造函数
NString::NString()
    : data_(sso_buffer_), size_(0), capacity_(SSO_BUFFER_SIZE)
{
    sso_buffer_[0] = '\0';
}

NString::NString(const char* str)
    : data_(sso_buffer_), size_(0), capacity_(SSO_BUFFER_SIZE)
{
    if (str)
    {
        InitializeFromCString(str, strlen(str));
    }
    else
    {
        sso_buffer_[0] = '\0';
    }
}

NString::NString(const char* str, size_t length)
    : data_(sso_buffer_), size_(0), capacity_(SSO_BUFFER_SIZE)
{
    if (str && length > 0)
    {
        InitializeFromCString(str, length);
    }
    else
    {
        sso_buffer_[0] = '\0';
    }
}

NString::NString(const NString& other)
    : data_(sso_buffer_), size_(0), capacity_(SSO_BUFFER_SIZE)
{
    CopyFrom(other);
}

NString::NString(NString&& other) noexcept
    : data_(sso_buffer_), size_(0), capacity_(SSO_BUFFER_SIZE)
{
    MoveFrom(std::move(other));
}

NString::NString(std::string_view sv)
    : data_(sso_buffer_), size_(0), capacity_(SSO_BUFFER_SIZE)
{
    if (!sv.empty())
    {
        InitializeFromCString(sv.data(), sv.size());
    }
    else
    {
        sso_buffer_[0] = '\0';
    }
}

NString::NString(size_t count, char ch)
    : data_(sso_buffer_), size_(0), capacity_(SSO_BUFFER_SIZE)
{
    if (count > 0)
    {
        EnsureCapacity(count);
        memset(data_, ch, count);
        SetSize(count);
    }
    else
    {
        sso_buffer_[0] = '\0';
    }
}

// 析构函数
NString::~NString()
{
    Deallocate();
}

// 赋值操作符
NString& NString::operator=(const NString& other)
{
    if (this != &other)
    {
        CopyFrom(other);
    }
    return *this;
}

NString& NString::operator=(NString&& other) noexcept
{
    if (this != &other)
    {
        Deallocate();
        MoveFrom(std::move(other));
    }
    return *this;
}

NString& NString::operator=(const char* str)
{
    if (str)
    {
        size_t length = strlen(str);
        EnsureCapacity(length);
        memcpy(data_, str, length);
        SetSize(length);
    }
    else
    {
        Clear();
    }
    return *this;
}

NString& NString::operator=(std::string_view sv)
{
    if (!sv.empty())
    {
        EnsureCapacity(sv.size());
        memcpy(data_, sv.data(), sv.size());
        SetSize(sv.size());
    }
    else
    {
        Clear();
    }
    return *this;
}

// 基础访问
char& NString::operator[](size_t index)
{
    return data_[index];
}

const char& NString::operator[](size_t index) const
{
    return data_[index];
}

char& NString::At(size_t index)
{
    if (index >= size_)
    {
        NLogger::GetLogger().Error("NString::At: Index {} out of bounds (size: {})", index, size_);
        static char dummy = '\0';
        return dummy;
    }
    return data_[index];
}

const char& NString::At(size_t index) const
{
    if (index >= size_)
    {
        NLogger::GetLogger().Error("NString::At: Index {} out of bounds (size: {})", index, size_);
        static const char dummy = '\0';
        return dummy;
    }
    return data_[index];
}

// 容器接口实现
void NString::Clear()
{
    size_ = 0;
    if (data_)
    {
        data_[0] = '\0';
    }
}

// 字符串特有功能
void NString::Reserve(size_t new_capacity)
{
    if (new_capacity > capacity_)
    {
        EnsureCapacity(new_capacity);
    }
}

void NString::Resize(size_t new_size, char fill_char)
{
    if (new_size > size_)
    {
        EnsureCapacity(new_size);
        memset(data_ + size_, fill_char, new_size - size_);
    }
    SetSize(new_size);
}

void NString::ShrinkToFit()
{
    if (!IsUsingSSO() && capacity_ > size_ + 1)
    {
        if (size_ <= SSO_BUFFER_SIZE)
        {
            // 切换回SSO
            memcpy(sso_buffer_, data_, size_);
            DeallocateElements(data_, capacity_);
            data_ = sso_buffer_;
            capacity_ = SSO_BUFFER_SIZE;
        }
        else
        {
            // 重新分配精确大小
            char* new_data = AllocateElements<char>(size_ + 1);
            memcpy(new_data, data_, size_);
            DeallocateElements(data_, capacity_);
            data_ = new_data;
            capacity_ = size_;
        }
        data_[size_] = '\0';
    }
}

// 字符串操作
NString& NString::Append(const char* str)
{
    if (str)
    {
        return Append(str, strlen(str));
    }
    return *this;
}

NString& NString::Append(const char* str, size_t length)
{
    if (str && length > 0)
    {
        EnsureCapacity(size_ + length);
        memcpy(data_ + size_, str, length);
        SetSize(size_ + length);
    }
    return *this;
}

NString& NString::Append(const NString& other)
{
    return Append(other.data_, other.size_);
}

NString& NString::Append(std::string_view sv)
{
    return Append(sv.data(), sv.size());
}

NString& NString::Append(size_t count, char ch)
{
    if (count > 0)
    {
        EnsureCapacity(size_ + count);
        memset(data_ + size_, ch, count);
        SetSize(size_ + count);
    }
    return *this;
}

NString& NString::Insert(size_t pos, const char* str)
{
    if (str)
    {
        return Insert(pos, str, strlen(str));
    }
    return *this;
}

NString& NString::Insert(size_t pos, const char* str, size_t length)
{
    if (!str || length == 0 || pos > size_)
        return *this;
    
    EnsureCapacity(size_ + length);
    
    // 移动现有数据
    if (pos < size_)
    {
        memmove(data_ + pos + length, data_ + pos, size_ - pos);
    }
    
    // 插入新数据
    memcpy(data_ + pos, str, length);
    SetSize(size_ + length);
    
    return *this;
}

NString& NString::Insert(size_t pos, const NString& other)
{
    return Insert(pos, other.data_, other.size_);
}

NString& NString::Insert(size_t pos, size_t count, char ch)
{
    if (count == 0 || pos > size_)
        return *this;
    
    EnsureCapacity(size_ + count);
    
    // 移动现有数据
    if (pos < size_)
    {
        memmove(data_ + pos + count, data_ + pos, size_ - pos);
    }
    
    // 插入新数据
    memset(data_ + pos, ch, count);
    SetSize(size_ + count);
    
    return *this;
}

NString& NString::Erase(size_t pos, size_t length)
{
    if (pos >= size_)
        return *this;
    
    if (length == NPOS || pos + length >= size_)
    {
        // 删除到末尾
        SetSize(pos);
    }
    else
    {
        // 删除中间部分
        memmove(data_ + pos, data_ + pos + length, size_ - pos - length);
        SetSize(size_ - length);
    }
    
    return *this;
}

void NString::PushBack(char ch)
{
    EnsureCapacity(size_ + 1);
    data_[size_] = ch;
    SetSize(size_ + 1);
}

void NString::PopBack()
{
    if (size_ > 0)
    {
        SetSize(size_ - 1);
    }
}

// 查找功能
size_t NString::Find(const char* str, size_t pos) const
{
    if (!str || pos >= size_)
        return NPOS;
    
    const char* result = strstr(data_ + pos, str);
    return result ? static_cast<size_t>(result - data_) : NPOS;
}

size_t NString::Find(const NString& other, size_t pos) const
{
    return Find(other.data_, pos);
}

size_t NString::Find(char ch, size_t pos) const
{
    if (pos >= size_)
        return NPOS;
    
    const char* result = strchr(data_ + pos, ch);
    return result ? static_cast<size_t>(result - data_) : NPOS;
}

size_t NString::RFind(char ch, size_t pos) const
{
    if (size_ == 0)
        return NPOS;
    
    size_t search_pos = (pos == NPOS || pos >= size_) ? size_ - 1 : pos;
    
    for (size_t i = search_pos + 1; i > 0; --i)
    {
        if (data_[i - 1] == ch)
            return i - 1;
    }
    
    return NPOS;
}

// 子字符串
NString NString::Substring(size_t pos, size_t length) const
{
    if (pos >= size_)
        return NString();
    
    size_t actual_length = (length == NPOS || pos + length > size_) ? 
        size_ - pos : length;
    
    return NString(data_ + pos, actual_length);
}

// 比较
int NString::Compare(const char* str) const
{
    if (!str)
        return size_ > 0 ? 1 : 0;
    
    return strcmp(data_, str);
}

int NString::Compare(const NString& other) const
{
    return Compare(other.data_);
}

int NString::Compare(std::string_view sv) const
{
    size_t min_size = std::min(size_, sv.size());
    int result = memcmp(data_, sv.data(), min_size);
    
    if (result == 0)
    {
        if (size_ < sv.size()) return -1;
        if (size_ > sv.size()) return 1;
    }
    
    return result;
}

bool NString::StartsWith(const char* prefix) const
{
    if (!prefix) return true;
    
    size_t prefix_len = strlen(prefix);
    if (prefix_len > size_) return false;
    
    return memcmp(data_, prefix, prefix_len) == 0;
}

bool NString::StartsWith(const NString& prefix) const
{
    if (prefix.size_ > size_) return false;
    
    return memcmp(data_, prefix.data_, prefix.size_) == 0;
}

bool NString::EndsWith(const char* suffix) const
{
    if (!suffix) return true;
    
    size_t suffix_len = strlen(suffix);
    if (suffix_len > size_) return false;
    
    return memcmp(data_ + size_ - suffix_len, suffix, suffix_len) == 0;
}

bool NString::EndsWith(const NString& suffix) const
{
    if (suffix.size_ > size_) return false;
    
    return memcmp(data_ + size_ - suffix.size_, suffix.data_, suffix.size_) == 0;
}

// 修改操作
NString& NString::ToLower()
{
    for (size_t i = 0; i < size_; ++i)
    {
        data_[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(data_[i])));
    }
    return *this;
}

NString& NString::ToUpper()
{
    for (size_t i = 0; i < size_; ++i)
    {
        data_[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(data_[i])));
    }
    return *this;
}

NString& NString::Trim()
{
    return TrimLeft().TrimRight();
}

NString& NString::TrimLeft()
{
    size_t start = 0;
    while (start < size_ && std::isspace(static_cast<unsigned char>(data_[start])))
    {
        ++start;
    }
    
    if (start > 0)
    {
        memmove(data_, data_ + start, size_ - start);
        SetSize(size_ - start);
    }
    
    return *this;
}

NString& NString::TrimRight()
{
    while (size_ > 0 && std::isspace(static_cast<unsigned char>(data_[size_ - 1])))
    {
        --size_;
        data_[size_] = '\0';
    }
    
    return *this;
}

NString NString::ToLowerCopy() const
{
    NString result(*this);
    return result.ToLower();
}

NString NString::ToUpperCopy() const
{
    NString result(*this);
    return result.ToUpper();
}

NString NString::TrimCopy() const
{
    NString result(*this);
    return result.Trim();
}

// 格式化
NString NString::Format(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    NString result = VFormat(format, args);
    va_end(args);
    return result;
}

NString NString::VFormat(const char* format, va_list args)
{
    if (!format) return NString();
    
    // 首先尝试使用小缓冲区
    char buffer[256];
    va_list args_copy;
    va_copy(args_copy, args);
    
    int required = vsnprintf(buffer, sizeof(buffer), format, args_copy);
    va_end(args_copy);
    
    if (required < 0) return NString();
    
    if (static_cast<size_t>(required) < sizeof(buffer))
    {
        return NString(buffer, required);
    }
    
    // 需要更大的缓冲区
    NString result;
    result.EnsureCapacity(required);
    vsnprintf(result.data_, required + 1, format, args);
    result.SetSize(required);
    
    return result;
}

// 类型转换
int32_t NString::ToInt32(int base) const
{
    return static_cast<int32_t>(strtol(data_, nullptr, base));
}

int64_t NString::ToInt64(int base) const
{
    return strtoll(data_, nullptr, base);
}

float NString::ToFloat() const
{
    return strtof(data_, nullptr);
}

double NString::ToDouble() const
{
    return strtod(data_, nullptr);
}

bool NString::ToBool() const
{
    if (size_ == 0) return false;
    if (Compare("true") == 0 || Compare("1") == 0) return true;
    if (Compare("false") == 0 || Compare("0") == 0) return false;
    return ToInt32() != 0;
}

NString NString::FromInt32(int32_t value)
{
    return Format("%d", value);
}

NString NString::FromInt64(int64_t value)
{
    return Format("%lld", value);
}

NString NString::FromFloat(float value, int precision)
{
    return Format("%.*f", precision, value);
}

NString NString::FromDouble(double value, int precision)
{
    return Format("%.*f", precision, value);
}

NString NString::FromBool(bool value)
{
    return value ? NString("true") : NString("false");
}

// UTF-8支持 (简化实现)
size_t NString::GetCharacterCount() const
{
    size_t count = 0;
    for (size_t i = 0; i < size_;)
    {
        if (IsUtf8StartByte(data_[i]))
        {
            size_t char_len = GetUtf8CharLength(data_[i]);
            if (i + char_len <= size_)
            {
                ++count;
                i += char_len;
            }
            else
            {
                break; // 无效的UTF-8序列
            }
        }
        else
        {
            ++i; // 跳过无效字节
        }
    }
    return count;
}

// 运算符重载
NString NString::operator+(const char* str) const
{
    NString result(*this);
    result.Append(str);
    return result;
}

NString NString::operator+(const NString& other) const
{
    NString result(*this);
    result.Append(other);
    return result;
}

NString NString::operator+(char ch) const
{
    NString result(*this);
    result.PushBack(ch);
    return result;
}

NString& NString::operator+=(const char* str)
{
    return Append(str);
}

NString& NString::operator+=(const NString& other)
{
    return Append(other);
}

NString& NString::operator+=(char ch)
{
    PushBack(ch);
    return *this;
}

bool NString::operator==(const char* str) const
{
    return Compare(str) == 0;
}

bool NString::operator==(const NString& other) const
{
    return Compare(other) == 0;
}

bool NString::operator!=(const char* str) const
{
    return Compare(str) != 0;
}

bool NString::operator!=(const NString& other) const
{
    return Compare(other) != 0;
}

bool NString::operator<(const NString& other) const
{
    return Compare(other) < 0;
}

// NObject 接口重写
bool NString::Equals(const NObject* other) const
{
    const NString* other_string = dynamic_cast<const NString*>(other);
    return other_string && (*this == *other_string);
}

size_t NString::GetHashCode() const
{
    return NHash<const char*>{}(data_);
}

NString NString::ToString() const
{
    return *this;
}

// 内部工具函数
void NString::EnsureCapacity(size_t required_capacity)
{
    if (required_capacity <= capacity_)
        return;
    
    size_t new_capacity = CalculateGrowth(capacity_, required_capacity);
    
    if (IsUsingSSO())
    {
        // 从SSO切换到堆分配
        char* new_data = AllocateElements<char>(new_capacity + 1);
        memcpy(new_data, sso_buffer_, size_);
        data_ = new_data;
        capacity_ = new_capacity;
    }
    else
    {
        // 重新分配堆内存
        char* new_data = AllocateElements<char>(new_capacity + 1);
        memcpy(new_data, data_, size_);
        DeallocateElements(data_, capacity_);
        data_ = new_data;
        capacity_ = new_capacity;
    }
    
    data_[size_] = '\0';
}

void NString::SetSize(size_t new_size)
{
    size_ = new_size;
    data_[size_] = '\0';
}

void NString::InitializeFromCString(const char* str, size_t length)
{
    if (length <= SSO_BUFFER_SIZE)
    {
        memcpy(sso_buffer_, str, length);
        data_ = sso_buffer_;
        capacity_ = SSO_BUFFER_SIZE;
    }
    else
    {
        data_ = AllocateElements<char>(length + 1);
        memcpy(data_, str, length);
        capacity_ = length;
    }
    
    SetSize(length);
}

void NString::MoveFrom(NString&& other) noexcept
{
    if (other.IsUsingSSO())
    {
        // 其他对象使用SSO，复制数据
        memcpy(sso_buffer_, other.sso_buffer_, other.size_ + 1);
        data_ = sso_buffer_;
        capacity_ = SSO_BUFFER_SIZE;
    }
    else
    {
        // 其他对象使用堆分配，移动指针
        data_ = other.data_;
        capacity_ = other.capacity_;
    }
    
    size_ = other.size_;
    
    // 重置other为SSO状态
    other.data_ = other.sso_buffer_;
    other.size_ = 0;
    other.capacity_ = SSO_BUFFER_SIZE;
    other.sso_buffer_[0] = '\0';
}

void NString::CopyFrom(const NString& other)
{
    if (other.size_ <= capacity_)
    {
        // 可以使用现有缓冲区
        memcpy(data_, other.data_, other.size_);
        SetSize(other.size_);
    }
    else
    {
        // 需要重新分配
        Deallocate();
        
        if (other.IsUsingSSO())
        {
            memcpy(sso_buffer_, other.sso_buffer_, other.size_ + 1);
            data_ = sso_buffer_;
            capacity_ = SSO_BUFFER_SIZE;
        }
        else
        {
            data_ = AllocateElements<char>(other.capacity_ + 1);
            memcpy(data_, other.data_, other.size_);
            capacity_ = other.capacity_;
        }
        
        size_ = other.size_;
        data_[size_] = '\0';
    }
}

void NString::Deallocate()
{
    if (!IsUsingSSO() && data_)
    {
        DeallocateElements(data_, capacity_);
    }
    
    data_ = sso_buffer_;
    size_ = 0;
    capacity_ = SSO_BUFFER_SIZE;
    sso_buffer_[0] = '\0';
}

// UTF-8 工具函数
bool NString::IsUtf8StartByte(char byte)
{
    return (static_cast<unsigned char>(byte) & 0x80) == 0 || 
           (static_cast<unsigned char>(byte) & 0xE0) == 0xC0 ||
           (static_cast<unsigned char>(byte) & 0xF0) == 0xE0 ||
           (static_cast<unsigned char>(byte) & 0xF8) == 0xF0;
}

size_t NString::GetUtf8CharLength(char first_byte)
{
    unsigned char byte = static_cast<unsigned char>(first_byte);
    if ((byte & 0x80) == 0) return 1;
    if ((byte & 0xE0) == 0xC0) return 2;
    if ((byte & 0xF0) == 0xE0) return 3;
    if ((byte & 0xF8) == 0xF0) return 4;
    return 1; // 无效字节，当作单字节处理
}