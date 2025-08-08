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

// Include nlohmann/json
#include <nlohmann/json.hpp>

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
// 前向声明
class CConfigValue;

/**
 * @brief 配置数组类型
 */
using CConfigArray = TArray<CConfigValue, CMemoryManager>;

/**
 * @brief 配置对象类型
 */
using CConfigObject = THashMap<CString, CConfigValue, std::hash<CString>, std::equal_to<CString>, CMemoryManager>;

/**
 * @brief 配置值变体类型
 */
// 前向声明
class CConfigValue;

/**
 * @brief 简化的配置值变体类型（只包含基本类型）
 */
using CConfigVariant = std::variant<std::monostate,                      // Null
                                    bool,                                // Bool
                                    int32_t,                            // Int32
                                    int64_t,                            // Int64
                                    float,                              // Float
                                    double,                             // Double
                                    CString       // String
                                    >;

/**
 * @brief 配置值类
 *
 * 使用nlohmann::json作为底层存储，提供类型安全的配置值访问：
 * - 支持多种基础数据类型
 * - 嵌套对象和数组支持  
 * - 类型转换和验证
 * - 路径访问支持
 * - JSON序列化/反序列化
 */
class CConfigValue
{
private:
	nlohmann::json InternalJson;  // 使用nlohmann::json作为底层存储

private:
	// 辅助函数：将CString转换为std::string
	static std::string CStringToStdString(const CString& str)
	{
		return std::string(str.GetData(), str.Size());
	}
	
	// 辅助函数：将std::string转换为CString  
	static CString StdStringToCString(const std::string& str)
	{
		return CString(str.c_str());
	}

public:
	// === 构造函数 ===

	/**
	 * @brief 默认构造函数（空值）
	 */
	CConfigValue() : InternalJson(nullptr) {}

	/**
	 * @brief 从nlohmann::json构造
	 */
	explicit CConfigValue(const nlohmann::json& json) : InternalJson(json) {}
	explicit CConfigValue(nlohmann::json&& json) : InternalJson(std::move(json)) {}

	/**
	 * @brief 基础类型构造函数
	 */
	explicit CConfigValue(bool value) : InternalJson(value) {}
	explicit CConfigValue(int32_t value) : InternalJson(value) {}
	explicit CConfigValue(int64_t value) : InternalJson(value) {}
	explicit CConfigValue(float value) : InternalJson(value) {}
	explicit CConfigValue(double value) : InternalJson(value) {}
	explicit CConfigValue(const char* value) : InternalJson(std::string(value)) {}
	explicit CConfigValue(const CString& value) : InternalJson(CStringToStdString(value)) {}
	
	/**
	 * @brief 从CConfigArray构造
	 */
	explicit CConfigValue(const CConfigArray& array) 
	{
		InternalJson = nlohmann::json::array();
		for (const auto& item : array)
		{
			InternalJson.push_back(item.InternalJson);
		}
	}
	
	/**
	 * @brief 从CConfigObject构造
	 */
	explicit CConfigValue(const CConfigObject& object)
	{
		InternalJson = nlohmann::json::object();
		for (const auto& pair : object)
		{
			InternalJson[CStringToStdString(pair.first)] = pair.second.InternalJson;
		}
	}

	/**
	 * @brief 拷贝和移动构造函数
	 */
	CConfigValue(const CConfigValue& other) : InternalJson(other.InternalJson) {}
	CConfigValue(CConfigValue&& other) noexcept : InternalJson(std::move(other.InternalJson)) {}

	/**
	 * @brief 析构函数
	 */
	~CConfigValue() = default;

public:
	// === 赋值操作符 ===

	CConfigValue& operator=(const CConfigValue& Other)
	{
		if (this != &Other)
		{
			InternalJson = Other.InternalJson;
		}
		return *this;
	}
	
	CConfigValue& operator=(CConfigValue&& Other) noexcept
	{
		if (this != &Other)
		{
			InternalJson = std::move(Other.InternalJson);
		}
		return *this;
	}

	CConfigValue& operator=(bool InValue)
	{
		InternalJson = InValue;
		return *this;
	}
	CConfigValue& operator=(int32_t InValue)
	{
		InternalJson = InValue;
		return *this;
	}
	CConfigValue& operator=(int64_t InValue)
	{
		InternalJson = InValue;
		return *this;
	}
	CConfigValue& operator=(float InValue)
	{
		InternalJson = InValue;
		return *this;
	}
	CConfigValue& operator=(double InValue)
	{
		InternalJson = InValue;
		return *this;
	}
	CConfigValue& operator=(const CString& InValue)
	{
		InternalJson = CStringToStdString(InValue);
		return *this;
	}
	CConfigValue& operator=(CString&& InValue)
	{
		InternalJson = CStringToStdString(InValue);
		return *this;
	}
	CConfigValue& operator=(const char* InValue)
	{
		InternalJson = std::string(InValue);
		return *this;
	}
	CConfigValue& operator=(const CConfigArray& InValue)
	{
		nlohmann::json JsonArray = nlohmann::json::array();
		for (const auto& Item : InValue)
		{
			JsonArray.push_back(Item.InternalJson);
		}
		InternalJson = JsonArray;
		return *this;
	}
	CConfigValue& operator=(CConfigArray&& InValue)
	{
		nlohmann::json JsonArray = nlohmann::json::array();
		for (auto& Item : InValue)
		{
			JsonArray.push_back(std::move(Item.InternalJson));
		}
		InternalJson = JsonArray;
		return *this;
	}
	CConfigValue& operator=(const CConfigObject& InValue)
	{
		nlohmann::json JsonObject = nlohmann::json::object();
		for (const auto& Pair : InValue)
		{
			JsonObject[CStringToStdString(Pair.first)] = Pair.second.InternalJson;
		}
		InternalJson = JsonObject;
		return *this;
	}
	CConfigValue& operator=(CConfigObject&& InValue)
	{
		nlohmann::json JsonObject = nlohmann::json::object();
		for (auto& Pair : InValue)
		{
			JsonObject[CStringToStdString(Pair.first)] = std::move(Pair.second.InternalJson);
		}
		InternalJson = JsonObject;
		return *this;
	}

public:
	// === 类型查询 ===

	/**
	 * @brief 获取值类型
	 */
	EConfigValueType GetType() const
	{
		if (InternalJson.is_null())
		{
			return EConfigValueType::Null;
		}
		else if (InternalJson.is_boolean())
		{
			return EConfigValueType::Bool;
		}
		else if (InternalJson.is_number_integer())
		{
			// 检查是否适合int32_t
			int64_t Value = InternalJson.get<int64_t>();
			if (Value >= INT32_MIN && Value <= INT32_MAX)
			{
				return EConfigValueType::Int32;
			}
			else
			{
				return EConfigValueType::Int64;
			}
		}
		else if (InternalJson.is_number_float())
		{
			return EConfigValueType::Double;
		}
		else if (InternalJson.is_string())
		{
			return EConfigValueType::String;
		}
		else if (InternalJson.is_array())
		{
			return EConfigValueType::Array;
		}
		else if (InternalJson.is_object())
		{
			return EConfigValueType::Object;
		}
		else
		{
			return EConfigValueType::Null;
		}
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
		if (InternalJson.is_boolean())
		{
			return InternalJson.get<bool>();
		}
		else if (InternalJson.is_number())
		{
			return InternalJson.get<double>() != 0.0;
		}
		else if (InternalJson.is_string())
		{
			std::string Str = InternalJson.get<std::string>();
			return Str == "true" || Str == "1";
		}

		return DefaultValue;
	}

	/**
	 * @brief 转换为32位整数
	 */
	int32_t AsInt32(int32_t DefaultValue = 0) const
	{
		if (InternalJson.is_number_integer())
		{
			int64_t Val = InternalJson.get<int64_t>();
			if (Val >= INT32_MIN && Val <= INT32_MAX)
			{
				return static_cast<int32_t>(Val);
			}
			else
			{
				return Val > 0 ? INT32_MAX : INT32_MIN;
			}
		}
		else if (InternalJson.is_number_float())
		{
			return static_cast<int32_t>(InternalJson.get<double>());
		}
		else if (InternalJson.is_boolean())
		{
			return InternalJson.get<bool>() ? 1 : 0;
		}
		else if (InternalJson.is_string())
		{
			try
			{
				return std::stoi(InternalJson.get<std::string>());
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
		if (InternalJson.is_number_integer())
		{
			return InternalJson.get<int64_t>();
		}
		else if (InternalJson.is_number_float())
		{
			return static_cast<int64_t>(InternalJson.get<double>());
		}
		else if (InternalJson.is_boolean())
		{
			return InternalJson.get<bool>() ? 1 : 0;
		}
		else if (InternalJson.is_string())
		{
			try
			{
				return std::stoll(InternalJson.get<std::string>());
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
		if (InternalJson.is_number())
		{
			return InternalJson.get<double>();
		}
		else if (InternalJson.is_boolean())
		{
			return InternalJson.get<bool>() ? 1.0 : 0.0;
		}
		else if (InternalJson.is_string())
		{
			try
			{
				return std::stod(InternalJson.get<std::string>());
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
	CString AsString(const CString& DefaultValue = CString()) const
	{
		if (InternalJson.is_string())
		{
			return StdStringToCString(InternalJson.get<std::string>());
		}
		else if (InternalJson.is_boolean())
		{
			return InternalJson.get<bool>() ? CString("true") : CString("false");
		}
		else if (InternalJson.is_number_integer())
		{
			int64_t Val = InternalJson.get<int64_t>();
			if (Val >= INT32_MIN && Val <= INT32_MAX)
			{
				return CString::FromInt(static_cast<int32_t>(Val));
			}
			else
			{
				return CString::FromInt64(Val);
			}
		}
		else if (InternalJson.is_number_float())
		{
			return CString::FromDouble(InternalJson.get<double>());
		}
		else if (InternalJson.is_null())
		{
			return CString("null");
		}

		return DefaultValue;
	}
	
	/**
	 * @brief 获取哈希值
	 */
	size_t GetHashCode() const
	{
		// 简化哈希实现：基于类型和部分数据
		size_t Hash = static_cast<size_t>(GetType());
		
		switch (GetType())
		{
		case EConfigValueType::Bool:
			Hash ^= static_cast<size_t>(InternalJson.get<bool>());
			break;
		case EConfigValueType::Int32:
			Hash ^= static_cast<size_t>(AsInt32());
			break;
		case EConfigValueType::Int64:
			Hash ^= static_cast<size_t>(InternalJson.get<int64_t>());
			break;
		case EConfigValueType::Double:
			Hash ^= std::hash<double>{}(InternalJson.get<double>());
			break;
		case EConfigValueType::String:
			Hash ^= std::hash<std::string>{}(InternalJson.get<std::string>());
			break;
		default:
			// 对于复杂类型，使用简单哈希
			Hash ^= reinterpret_cast<size_t>(this);
			break;
		}
		
		return Hash;
	}

	/**
	 * @brief 获取数组引用
	 */
	const CConfigArray& AsArray() const
	{
		static CConfigArray EmptyArray;
		static thread_local CConfigArray TempArray; // 临时数组，用于转换
		
		if (InternalJson.is_array())
		{
			// 将nlohmann::json数组转换为CConfigArray
			TempArray.Clear();
			TempArray.Reserve(InternalJson.size());
			for (const auto& Item : InternalJson)
			{
				TempArray.PushBack(CConfigValue(Item));
			}
			return TempArray;
		}
		return EmptyArray;
	}

	/**
	 * @brief 获取数组引用（可修改）
	 */
	CConfigArray& AsArray()
	{
		static thread_local CConfigArray TempArray; // 临时数组
		
		if (!InternalJson.is_array())
		{
			InternalJson = nlohmann::json::array();
		}
		
		// 将nlohmann::json数组转换为CConfigArray
		TempArray.Clear();
		TempArray.Reserve(InternalJson.size());
		for (const auto& Item : InternalJson)
		{
			TempArray.PushBack(CConfigValue(Item));
		}
		return TempArray;
	}

	/**
	 * @brief 获取对象引用
	 */
	const CConfigObject& AsObject() const
	{
		static CConfigObject EmptyObject;
		static thread_local CConfigObject TempObject; // 临时对象
		
		if (InternalJson.is_object())
		{
			// 将nlohmann::json对象转换为CConfigObject
			TempObject.Clear();
			for (const auto& Item : InternalJson.items())
			{
				TempObject.Insert(StdStringToCString(Item.key()), CConfigValue(Item.value()));
			}
			return TempObject;
		}
		return EmptyObject;
	}

	/**
	 * @brief 获取对象引用（可修改）
	 */
	CConfigObject& AsObject()
	{
		static thread_local CConfigObject TempObject; // 临时对象
		
		if (!InternalJson.is_object())
		{
			InternalJson = nlohmann::json::object();
		}
		
		// 将nlohmann::json对象转换为CConfigObject
		TempObject.Clear();
		for (const auto& Item : InternalJson.items())
		{
			TempObject.Insert(StdStringToCString(Item.key()), CConfigValue(Item.value()));
		}
		return TempObject;
	}

public:
	// === 数组操作 ===

	/**
	 * @brief 获取数组大小
	 */
	size_t Size() const
	{
		if (InternalJson.is_array() || InternalJson.is_object())
		{
			return InternalJson.size();
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
		static thread_local CConfigValue TempValue;
		
		if (InternalJson.is_array() && Index < InternalJson.size())
		{
			TempValue = CConfigValue(InternalJson[Index]);
			return TempValue;
		}
		return NullValue;
	}

	/**
	 * @brief 数组访问操作符（可修改）
	 */
	CConfigValue& operator[](size_t Index)
	{
		static thread_local CConfigValue TempValue;
		
		if (!InternalJson.is_array())
		{
			InternalJson = nlohmann::json::array();
		}

		// 等待数组扩容到指定索引
		while (InternalJson.size() <= Index)
		{
			InternalJson.push_back(nlohmann::json(nullptr));
		}

		TempValue = CConfigValue(InternalJson[Index]);
		return TempValue;
	}

	/**
	 * @brief 添加数组元素
	 */
	void PushBack(const CConfigValue& Val)
	{
		if (!InternalJson.is_array())
		{
			InternalJson = nlohmann::json::array();
		}
		InternalJson.push_back(Val.InternalJson);
	}

	void PushBack(CConfigValue&& Val)
	{
		if (!InternalJson.is_array())
		{
			InternalJson = nlohmann::json::array();
		}
		InternalJson.push_back(std::move(Val.InternalJson));
	}

public:
	// === 对象操作 ===

	/**
	 * @brief 对象访问操作符
	 */
	const CConfigValue& operator[](const CString& Key) const
	{
		static CConfigValue NullValue;
		static thread_local CConfigValue TempValue;
		
		if (InternalJson.is_object())
		{
			std::string StdKey = CStringToStdString(Key);
			if (InternalJson.contains(StdKey))
			{
				TempValue = CConfigValue(InternalJson[StdKey]);
				return TempValue;
			}
		}
		return NullValue;
	}

	/**
	 * @brief 对象访问操作符（可修改）
	 */
	CConfigValue& operator[](const CString& Key)
	{
		static thread_local CConfigValue TempValue;
		
		if (!InternalJson.is_object())
		{
			InternalJson = nlohmann::json::object();
		}

		std::string StdKey = CStringToStdString(Key);
		if (!InternalJson.contains(StdKey))
		{
			InternalJson[StdKey] = nlohmann::json(nullptr);
		}

		TempValue = CConfigValue(InternalJson[StdKey]);
		return TempValue;
	}

	/**
	 * @brief 检查对象是否包含键
	 */
	bool HasKey(const CString& Key) const
	{
		if (InternalJson.is_object())
		{
			return InternalJson.contains(CStringToStdString(Key));
		}
		return false;
	}

	/**
	 * @brief 获取对象的所有键
	 */
	TArray<CString> GetKeys() const
	{
		TArray<CString> Keys;
		if (InternalJson.is_object())
		{
			Keys.Reserve(InternalJson.size());
			for (const auto& Item : InternalJson.items())
			{
				Keys.PushBack(StdStringToCString(Item.key()));
			}
		}
		return Keys;
	}

	/**
	 * @brief 移除对象键
	 */
	bool RemoveKey(const CString& Key)
	{
		if (InternalJson.is_object())
		{
			std::string StdKey = CStringToStdString(Key);
			if (InternalJson.contains(StdKey))
			{
				InternalJson.erase(StdKey);
				return true;
			}
		}
		return false;
	}

public:
	// === 路径访问 ===

	/**
	 * @brief 通过路径获取值
	 * @param Path 路径，例如 "database.host" 或 "servers[0].port"
	 */
	const CConfigValue& GetByPath(const CString& Path) const;

	/**
	 * @brief 通过路径设置值
	 */
	void SetByPath(const CString& Path, const CConfigValue& Val);

	/**
	 * @brief 检查路径是否存在
	 */
	bool HasPath(const CString& Path) const;

public:
	// === 比较操作符 ===

	bool operator==(const CConfigValue& Other) const
	{
		// 直接使用nlohmann::json的比较
		return InternalJson == Other.InternalJson;
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
	CString ToJsonString(bool Pretty = false, int32_t Indent = 2) const;

	/**
	 * @brief 获取底层nlohmann::json对象的引用
	 */
	const nlohmann::json& GetInternalJson() const { return InternalJson; }
	nlohmann::json& GetInternalJson() { return InternalJson; }

	/**
	 * @brief 获取类型名称
	 */
	CString GetTypeName() const
	{
		switch (GetType())
		{
		case EConfigValueType::Null:
			return CString("null");
		case EConfigValueType::Bool:
			return CString("bool");
		case EConfigValueType::Int32:
			return CString("int32");
		case EConfigValueType::Int64:
			return CString("int64");
		case EConfigValueType::Float:
			return CString("float");
		case EConfigValueType::Double:
			return CString("double");
		case EConfigValueType::String:
			return CString("string");
		case EConfigValueType::Array:
			return CString("array");
		case EConfigValueType::Object:
			return CString("object");
		default:
			return CString("unknown");
		}
	}

	// 注意：所有其他成员变量已经被 InternalJson 替代
};

// === 类型别名 ===
using FConfigValue = CConfigValue;
using FConfigArray = CConfigArray;
using FConfigObject = CConfigObject;

} // namespace NLib

// === std::hash 特化 ===
namespace std
{
template <>
struct hash<NLib::CConfigValue>
{
	size_t operator()(const NLib::CConfigValue& Value) const
	{
		return Value.GetHashCode();
	}
};
}
