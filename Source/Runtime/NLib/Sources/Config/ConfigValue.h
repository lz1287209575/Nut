#pragma once

#include "Containers/TArray.h"
#include "Containers/THashMap.h"
#include "Containers/TString.h"
#include "Core/Object.h"
#include "Core/SmartPointers.h"
#include "Logging/LogCategory.h"
#include "Math/MathTypes.h"

#include <type_traits>
#include <variant>

namespace NLib
{
/**
 * @brief 配置值类型枚举
 */
enum class EConfigValueType : uint8_t
{
	Null,   // 空值
	Bool,   // 布尔值
	Int32,  // 32位整数
	Int64,  // 64位整数
	Float,  // 单精度浮点数
	Double, // 双精度浮点数
	String, // 字符串
	Array,  // 数组
	Object  // 对象
};

// 前向声明
class CConfigValue;

/**
 * @brief 配置数组类型
 */
using CConfigArray = TArray<CConfigValue, CMemoryManager>;

/**
 * @brief 配置对象类型
 */
using CConfigObject = THashMap<TString, CConfigValue, CMemoryManager>;

/**
 * @brief 配置值变体类型
 */
using CConfigVariant = std::variant<std::monostate, // Null
                                    bool,           // Bool
                                    int32_t,        // Int32
                                    int64_t,        // Int64
                                    float,          // Float
                                    double,         // Double
                                    TString,        // String
                                    CConfigArray,   // Array
                                    CConfigObject   // Object
                                    >;

/**
 * @brief 配置值类
 *
 * 提供类型安全的配置值存储和访问：
 * - 支持多种基础数据类型
 * - 嵌套对象和数组支持
 * - 类型转换和验证
 * - 路径访问支持
 */
class CConfigValue
{
public:
	// === 构造函数 ===

	/**
	 * @brief 默认构造函数（空值）
	 */
	CConfigValue()
	    : Value(std::monostate{})
	{}

	/**
	 * @brief 布尔值构造函数
	 */
	explicit CConfigValue(bool InValue)
	    : Value(InValue)
	{}

	/**
	 * @brief 整数构造函数
	 */
	explicit CConfigValue(int32_t InValue)
	    : Value(InValue)
	{}
	explicit CConfigValue(int64_t InValue)
	    : Value(InValue)
	{}

	/**
	 * @brief 浮点数构造函数
	 */
	explicit CConfigValue(float InValue)
	    : Value(InValue)
	{}
	explicit CConfigValue(double InValue)
	    : Value(InValue)
	{}

	/**
	 * @brief 字符串构造函数
	 */
	explicit CConfigValue(const TString& InValue)
	    : Value(InValue)
	{}
	explicit CConfigValue(TString&& InValue)
	    : Value(std::move(InValue))
	{}
	explicit CConfigValue(const char* InValue)
	    : Value(TString(InValue))
	{}

	/**
	 * @brief 数组构造函数
	 */
	explicit CConfigValue(const CConfigArray& InValue)
	    : Value(InValue)
	{}
	explicit CConfigValue(CConfigArray&& InValue)
	    : Value(std::move(InValue))
	{}

	/**
	 * @brief 对象构造函数
	 */
	explicit CConfigValue(const CConfigObject& InValue)
	    : Value(InValue)
	{}
	explicit CConfigValue(CConfigObject&& InValue)
	    : Value(std::move(InValue))
	{}

	/**
	 * @brief 拷贝构造函数
	 */
	CConfigValue(const CConfigValue& Other) = default;

	/**
	 * @brief 移动构造函数
	 */
	CConfigValue(CConfigValue&& Other) noexcept = default;

	/**
	 * @brief 析构函数
	 */
	~CConfigValue() = default;

public:
	// === 赋值操作符 ===

	CConfigValue& operator=(const CConfigValue& Other) = default;
	CConfigValue& operator=(CConfigValue&& Other) noexcept = default;

	CConfigValue& operator=(bool InValue)
	{
		Value = InValue;
		return *this;
	}
	CConfigValue& operator=(int32_t InValue)
	{
		Value = InValue;
		return *this;
	}
	CConfigValue& operator=(int64_t InValue)
	{
		Value = InValue;
		return *this;
	}
	CConfigValue& operator=(float InValue)
	{
		Value = InValue;
		return *this;
	}
	CConfigValue& operator=(double InValue)
	{
		Value = InValue;
		return *this;
	}
	CConfigValue& operator=(const TString& InValue)
	{
		Value = InValue;
		return *this;
	}
	CConfigValue& operator=(TString&& InValue)
	{
		Value = std::move(InValue);
		return *this;
	}
	CConfigValue& operator=(const char* InValue)
	{
		Value = TString(InValue);
		return *this;
	}
	CConfigValue& operator=(const CConfigArray& InValue)
	{
		Value = InValue;
		return *this;
	}
	CConfigValue& operator=(CConfigArray&& InValue)
	{
		Value = std::move(InValue);
		return *this;
	}
	CConfigValue& operator=(const CConfigObject& InValue)
	{
		Value = InValue;
		return *this;
	}
	CConfigValue& operator=(CConfigObject&& InValue)
	{
		Value = std::move(InValue);
		return *this;
	}

public:
	// === 类型查询 ===

	/**
	 * @brief 获取值类型
	 */
	EConfigValueType GetType() const
	{
		return static_cast<EConfigValueType>(Value.index());
	}

	/**
	 * @brief 检查是否为空值
	 */
	bool IsNull() const
	{
		return GetType() == EConfigValueType::Null;
	}

	/**
	 * @brief 检查是否为布尔值
	 */
	bool IsBool() const
	{
		return GetType() == EConfigValueType::Bool;
	}

	/**
	 * @brief 检查是否为整数
	 */
	bool IsInt() const
	{
		auto Type = GetType();
		return Type == EConfigValueType::Int32 || Type == EConfigValueType::Int64;
	}

	/**
	 * @brief 检查是否为浮点数
	 */
	bool IsFloat() const
	{
		auto Type = GetType();
		return Type == EConfigValueType::Float || Type == EConfigValueType::Double;
	}

	/**
	 * @brief 检查是否为数字
	 */
	bool IsNumber() const
	{
		return IsInt() || IsFloat();
	}

	/**
	 * @brief 检查是否为字符串
	 */
	bool IsString() const
	{
		return GetType() == EConfigValueType::String;
	}

	/**
	 * @brief 检查是否为数组
	 */
	bool IsArray() const
	{
		return GetType() == EConfigValueType::Array;
	}

	/**
	 * @brief 检查是否为对象
	 */
	bool IsObject() const
	{
		return GetType() == EConfigValueType::Object;
	}

public:
	// === 类型转换（带默认值） ===

	/**
	 * @brief 转换为布尔值
	 */
	bool AsBool(bool DefaultValue = false) const
	{
		if (IsBool())
		{
			return std::get<bool>(Value);
		}
		else if (IsInt())
		{
			return AsInt64() != 0;
		}
		else if (IsFloat())
		{
			return AsDouble() != 0.0;
		}
		else if (IsString())
		{
			const auto& Str = std::get<TString>(Value);
			return Str.Equals("true", false) || Str.Equals("1", false);
		}

		return DefaultValue;
	}

	/**
	 * @brief 转换为32位整数
	 */
	int32_t AsInt32(int32_t DefaultValue = 0) const
	{
		if (GetType() == EConfigValueType::Int32)
		{
			return std::get<int32_t>(Value);
		}
		else if (GetType() == EConfigValueType::Int64)
		{
			int64_t Val = std::get<int64_t>(Value);
			return static_cast<int32_t>(
			    NMath::Clamp(Val, static_cast<int64_t>(INT32_MIN), static_cast<int64_t>(INT32_MAX)));
		}
		else if (IsFloat())
		{
			return static_cast<int32_t>(AsDouble());
		}
		else if (IsBool())
		{
			return std::get<bool>(Value) ? 1 : 0;
		}
		else if (IsString())
		{
			try
			{
				return std::stoi(std::get<TString>(Value).GetData());
			}
			catch (...)
			{
				return DefaultValue;
			}
		}

		return DefaultValue;
	}

	/**
	 * @brief 转换为64位整数
	 */
	int64_t AsInt64(int64_t DefaultValue = 0) const
	{
		if (GetType() == EConfigValueType::Int64)
		{
			return std::get<int64_t>(Value);
		}
		else if (GetType() == EConfigValueType::Int32)
		{
			return static_cast<int64_t>(std::get<int32_t>(Value));
		}
		else if (IsFloat())
		{
			return static_cast<int64_t>(AsDouble());
		}
		else if (IsBool())
		{
			return std::get<bool>(Value) ? 1 : 0;
		}
		else if (IsString())
		{
			try
			{
				return std::stoll(std::get<TString>(Value).GetData());
			}
			catch (...)
			{
				return DefaultValue;
			}
		}

		return DefaultValue;
	}

	/**
	 * @brief 转换为单精度浮点数
	 */
	float AsFloat(float DefaultValue = 0.0f) const
	{
		return static_cast<float>(AsDouble(static_cast<double>(DefaultValue)));
	}

	/**
	 * @brief 转换为双精度浮点数
	 */
	double AsDouble(double DefaultValue = 0.0) const
	{
		if (GetType() == EConfigValueType::Double)
		{
			return std::get<double>(Value);
		}
		else if (GetType() == EConfigValueType::Float)
		{
			return static_cast<double>(std::get<float>(Value));
		}
		else if (IsInt())
		{
			return static_cast<double>(AsInt64());
		}
		else if (IsBool())
		{
			return std::get<bool>(Value) ? 1.0 : 0.0;
		}
		else if (IsString())
		{
			try
			{
				return std::stod(std::get<TString>(Value).GetData());
			}
			catch (...)
			{
				return DefaultValue;
			}
		}

		return DefaultValue;
	}

	/**
	 * @brief 转换为字符串
	 */
	TString AsString(const TString& DefaultValue = TString()) const
	{
		if (IsString())
		{
			return std::get<TString>(Value);
		}
		else if (IsBool())
		{
			return std::get<bool>(Value) ? TString("true") : TString("false");
		}
		else if (GetType() == EConfigValueType::Int32)
		{
			return TString::FromInt(std::get<int32_t>(Value));
		}
		else if (GetType() == EConfigValueType::Int64)
		{
			return TString::FromInt64(std::get<int64_t>(Value));
		}
		else if (GetType() == EConfigValueType::Float)
		{
			return TString::FromFloat(std::get<float>(Value));
		}
		else if (GetType() == EConfigValueType::Double)
		{
			return TString::FromDouble(std::get<double>(Value));
		}
		else if (IsNull())
		{
			return TString("null");
		}

		return DefaultValue;
	}

	/**
	 * @brief 获取数组引用
	 */
	const CConfigArray& AsArray() const
	{
		static CConfigArray EmptyArray;
		if (IsArray())
		{
			return std::get<CConfigArray>(Value);
		}
		return EmptyArray;
	}

	/**
	 * @brief 获取数组引用（可修改）
	 */
	CConfigArray& AsArray()
	{
		if (!IsArray())
		{
			Value = CConfigArray();
		}
		return std::get<CConfigArray>(Value);
	}

	/**
	 * @brief 获取对象引用
	 */
	const CConfigObject& AsObject() const
	{
		static CConfigObject EmptyObject;
		if (IsObject())
		{
			return std::get<CConfigObject>(Value);
		}
		return EmptyObject;
	}

	/**
	 * @brief 获取对象引用（可修改）
	 */
	CConfigObject& AsObject()
	{
		if (!IsObject())
		{
			Value = CConfigObject();
		}
		return std::get<CConfigObject>(Value);
	}

public:
	// === 数组操作 ===

	/**
	 * @brief 获取数组大小
	 */
	size_t Size() const
	{
		if (IsArray())
		{
			return AsArray().Size();
		}
		else if (IsObject())
		{
			return AsObject().Size();
		}
		return 0;
	}

	/**
	 * @brief 检查数组是否为空
	 */
	bool IsEmpty() const
	{
		return Size() == 0;
	}

	/**
	 * @brief 数组访问操作符
	 */
	const CConfigValue& operator[](size_t Index) const
	{
		static CConfigValue NullValue;
		if (IsArray() && Index < AsArray().Size())
		{
			return AsArray()[Index];
		}
		return NullValue;
	}

	/**
	 * @brief 数组访问操作符（可修改）
	 */
	CConfigValue& operator[](size_t Index)
	{
		if (!IsArray())
		{
			Value = CConfigArray();
		}

		auto& Array = AsArray();
		while (Array.Size() <= Index)
		{
			Array.Add(CConfigValue());
		}

		return Array[Index];
	}

	/**
	 * @brief 添加数组元素
	 */
	void PushBack(const CConfigValue& Val)
	{
		AsArray().Add(Val);
	}

	void PushBack(CConfigValue&& Val)
	{
		AsArray().Add(std::move(Val));
	}

public:
	// === 对象操作 ===

	/**
	 * @brief 对象访问操作符
	 */
	const CConfigValue& operator[](const TString& Key) const
	{
		static CConfigValue NullValue;
		if (IsObject())
		{
			auto* Found = AsObject().Find(Key);
			if (Found)
			{
				return *Found;
			}
		}
		return NullValue;
	}

	/**
	 * @brief 对象访问操作符（可修改）
	 */
	CConfigValue& operator[](const TString& Key)
	{
		if (!IsObject())
		{
			Value = CConfigObject();
		}

		auto& Object = AsObject();
		auto* Found = Object.Find(Key);
		if (Found)
		{
			return *Found;
		}

		Object.Add(Key, CConfigValue());
		return Object[Key];
	}

	/**
	 * @brief 检查对象是否包含键
	 */
	bool HasKey(const TString& Key) const
	{
		if (IsObject())
		{
			return AsObject().Contains(Key);
		}
		return false;
	}

	/**
	 * @brief 获取对象的所有键
	 */
	TArray<TString, CMemoryManager> GetKeys() const
	{
		TArray<TString, CMemoryManager> Keys;
		if (IsObject())
		{
			const auto& Object = AsObject();
			Keys.Reserve(Object.Size());
			for (const auto& Pair : Object)
			{
				Keys.Add(Pair.Key);
			}
		}
		return Keys;
	}

	/**
	 * @brief 移除对象键
	 */
	bool RemoveKey(const TString& Key)
	{
		if (IsObject())
		{
			return AsObject().Remove(Key);
		}
		return false;
	}

public:
	// === 路径访问 ===

	/**
	 * @brief 通过路径获取值
	 * @param Path 路径，例如 "database.host" 或 "servers[0].port"
	 */
	const CConfigValue& GetByPath(const TString& Path) const;

	/**
	 * @brief 通过路径设置值
	 */
	void SetByPath(const TString& Path, const CConfigValue& Val);

	/**
	 * @brief 检查路径是否存在
	 */
	bool HasPath(const TString& Path) const;

public:
	// === 比较操作符 ===

	bool operator==(const CConfigValue& Other) const
	{
		return Value == Other.Value;
	}

	bool operator!=(const CConfigValue& Other) const
	{
		return !(*this == Other);
	}

public:
	// === 调试和序列化 ===

	/**
	 * @brief 转换为JSON字符串
	 */
	TString ToJsonString(bool Pretty = false, int32_t Indent = 0) const;

	/**
	 * @brief 获取类型名称
	 */
	TString GetTypeName() const
	{
		switch (GetType())
		{
		case EConfigValueType::Null:
			return TString("null");
		case EConfigValueType::Bool:
			return TString("bool");
		case EConfigValueType::Int32:
			return TString("int32");
		case EConfigValueType::Int64:
			return TString("int64");
		case EConfigValueType::Float:
			return TString("float");
		case EConfigValueType::Double:
			return TString("double");
		case EConfigValueType::String:
			return TString("string");
		case EConfigValueType::Array:
			return TString("array");
		case EConfigValueType::Object:
			return TString("object");
		default:
			return TString("unknown");
		}
	}

private:
	// === 成员变量 ===

	CConfigVariant Value; // 配置值
};

// === 类型别名 ===
using FConfigValue = CConfigValue;
using FConfigArray = CConfigArray;
using FConfigObject = CConfigObject;

} // namespace NLib