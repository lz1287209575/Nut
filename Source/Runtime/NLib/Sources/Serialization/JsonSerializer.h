#pragma once

#include "Config/ConfigValue.h"
#include "Config/JsonParser.h"
#include "Containers/TArray.h"
#include "Serializer.h"

namespace NLib
{
/**
 * @brief JSON序列化档案
 *
 * 提供基于JSON的序列化功能，具有良好的可读性和互操作性
 */
class CJsonSerializationArchive : public CSerializationArchive
{
	GENERATED_BODY()

public:
	// === 构造函数 ===

	explicit CJsonSerializationArchive(TSharedPtr<NStream> InStream, const SSerializationContext& InContext);
	~CJsonSerializationArchive() override = default;

public:
	// === 初始化和完成 ===

	/**
	 * @brief 初始化档案
	 */
	SSerializationResult Initialize();

	/**
	 * @brief 完成档案（写入JSON到流）
	 */
	SSerializationResult Finalize();

public:
	// === 基础类型序列化实现 ===

	SSerializationResult Serialize(bool& Value) override;
	SSerializationResult Serialize(int8_t& Value) override;
	SSerializationResult Serialize(uint8_t& Value) override;
	SSerializationResult Serialize(int16_t& Value) override;
	SSerializationResult Serialize(uint16_t& Value) override;
	SSerializationResult Serialize(int32_t& Value) override;
	SSerializationResult Serialize(uint32_t& Value) override;
	SSerializationResult Serialize(int64_t& Value) override;
	SSerializationResult Serialize(uint64_t& Value) override;
	SSerializationResult Serialize(float& Value) override;
	SSerializationResult Serialize(double& Value) override;
	SSerializationResult Serialize(CString& Value) override;

public:
	// === JSON特有的序列化方法 ===

	/**
	 * @brief 序列化为JSON null值
	 */
	SSerializationResult SerializeNull();

	/**
	 * @brief 序列化数组开始
	 */
	SSerializationResult BeginArray(int32_t Size = -1);

	/**
	 * @brief 序列化数组结束
	 */
	SSerializationResult EndArray();

	/**
	 * @brief 序列化数组元素
	 */
	template <typename T>
	SSerializationResult SerializeArrayElement(T& Element)
	{
		if (IsDeserializing() && CurrentArrayIndex >= CurrentArraySize)
		{
			return SSerializationResult(false, "Array index out of bounds");
		}

		auto Result = Serialize(Element);
		if (Result.bSuccess)
		{
			CurrentArrayIndex++;
		}

		return Result;
	}

	/**
	 * @brief 获取当前数组大小（反序列化时）
	 */
	int32_t GetCurrentArraySize() const
	{
		return CurrentArraySize;
	}

public:
	// === 配置值转换 ===

	/**
	 * @brief 从ConfigValue序列化
	 */
	SSerializationResult SerializeFromConfigValue(const CConfigValue& Value);

	/**
	 * @brief 序列化到ConfigValue
	 */
	SSerializationResult SerializeToConfigValue(CConfigValue& Value);

protected:
	// === CSerializationArchive 虚函数实现 ===

	SSerializationResult BeginObject(const CString& TypeName) override;
	SSerializationResult EndObject(const CString& TypeName) override;
	SSerializationResult BeginField(const CString& FieldName) override;
	SSerializationResult EndField(const CString& FieldName) override;

private:
	// === 内部实现 ===

	/**
	 * @brief 从流读取JSON
	 */
	SSerializationResult ReadJsonFromStream();

	/**
	 * @brief 写入JSON到流
	 */
	SSerializationResult WriteJsonToStream();

	/**
	 * @brief 获取当前JSON值
	 */
	CConfigValue* GetCurrentValue();

	/**
	 * @brief 设置当前JSON值
	 */
	SSerializationResult SetCurrentValue(const CConfigValue& Value);

	/**
	 * @brief 导航到字段
	 */
	SSerializationResult NavigateToField(const CString& FieldName);

	/**
	 * @brief 从字段导航回来
	 */
	SSerializationResult NavigateFromField(const CString& FieldName);

	/**
	 * @brief 确保当前值是对象
	 */
	SSerializationResult EnsureCurrentIsObject();

	/**
	 * @brief 确保当前值是数组
	 */
	SSerializationResult EnsureCurrentIsArray();

private:
	// JSON数据结构
	CConfigValue RootValue;

	// 导航栈
	struct SNavigationFrame
	{
		CConfigValue* Value = nullptr;
		CString Key;
		int32_t ArrayIndex = -1;
		bool bIsArray = false;

		SNavigationFrame() = default;
		SNavigationFrame(CConfigValue* InValue, const CString& InKey = CString())
		    : Value(InValue),
		      Key(InKey)
		{}
		SNavigationFrame(CConfigValue* InValue, int32_t InArrayIndex)
		    : Value(InValue),
		      ArrayIndex(InArrayIndex),
		      bIsArray(true)
		{}

		bool operator==(const SNavigationFrame& Other) const
		{
			return Value == Other.Value && Key == Other.Key && ArrayIndex == Other.ArrayIndex && bIsArray == Other.bIsArray;
		}
	};

	TArray<SNavigationFrame, CMemoryManager> NavigationStack;

	// 数组状态
	int32_t CurrentArrayIndex = 0;
	int32_t CurrentArraySize = 0;

	// 状态标志
	bool bInitialized = false;
	bool bJsonLoaded = false;
};

/**
 * @brief JSON序列化帮助器
 */
class CJsonSerializationHelper
{
public:
	/**
	 * @brief 序列化对象到JSON流
	 */
	template <typename T>
	static SSerializationResult SerializeToStream(const T& Object, TSharedPtr<NStream> Stream, bool bPrettyPrint = true)
	{
		SSerializationContext Context(ESerializationMode::Serialize, ESerializationFormat::JSON);
		if (bPrettyPrint)
		{
			Context.Flags = static_cast<ESerializationFlags>(static_cast<uint32_t>(Context.Flags) |
			                                                 static_cast<uint32_t>(ESerializationFlags::PrettyPrint));
		}

		auto Archive = MakeShared<CJsonSerializationArchive>(Stream, Context);
		auto Result = Archive->Initialize();
		if (!Result.bSuccess)
		{
			return Result;
		}

		T& MutableObject = const_cast<T&>(Object);
		Result = Archive->SerializeObject(MutableObject);
		if (!Result.bSuccess)
		{
			return Result;
		}

		return Archive->Finalize();
	}

	/**
	 * @brief 从JSON流反序列化对象
	 */
	template <typename T>
	static SSerializationResult DeserializeFromStream(T& Object, TSharedPtr<NStream> Stream)
	{
		SSerializationContext Context(ESerializationMode::Deserialize, ESerializationFormat::JSON);

		auto Archive = MakeShared<CJsonSerializationArchive>(Stream, Context);
		auto Result = Archive->Initialize();
		if (!Result.bSuccess)
		{
			return Result;
		}

		Result = Archive->SerializeObject(Object);
		if (!Result.bSuccess)
		{
			return Result;
		}

		return Archive->Finalize();
	}

	/**
	 * @brief 序列化对象到JSON字符串
	 */
	template <typename T>
	static CString SerializeToString(const T& Object, bool bPrettyPrint = true)
	{
		auto MemoryStream = MakeShared<NMemoryStream>();
		auto Result = SerializeToStream(Object, MemoryStream, bPrettyPrint);

		if (Result.bSuccess)
		{
			const auto& Buffer = MemoryStream->GetBuffer();
			return CString(reinterpret_cast<const char*>(Buffer.GetData()), Buffer.Size());
		}
		else
		{
			NLOG_SERIALIZATION(Error, "Failed to serialize object to JSON: {}", Result.ErrorMessage.GetData());
			return CString();
		}
	}

	/**
	 * @brief 从JSON字符串反序列化对象
	 */
	template <typename T>
	static bool DeserializeFromString(T& Object, const CString& JsonString)
	{
		auto MemoryStream = MakeShared<NMemoryStream>(reinterpret_cast<const uint8_t*>(JsonString.GetData()),
		                                              JsonString.Length());
		auto Result = DeserializeFromStream(Object, MemoryStream);

		if (!Result.bSuccess)
		{
			NLOG_SERIALIZATION(Error, "Failed to deserialize object from JSON: {}", Result.ErrorMessage.GetData());
		}

		return Result.bSuccess;
	}

	/**
	 * @brief 序列化ConfigValue到JSON字符串
	 */
	static CString ConfigValueToJson(const CConfigValue& Value, bool bPrettyPrint = true);

	/**
	 * @brief 从JSON字符串解析为ConfigValue
	 */
	static CConfigValue JsonToConfigValue(const CString& JsonString);
};

// === 类型别名 ===
using FJsonSerializationArchive = CJsonSerializationArchive;
using FJsonSerializationHelper = CJsonSerializationHelper;

} // namespace NLib