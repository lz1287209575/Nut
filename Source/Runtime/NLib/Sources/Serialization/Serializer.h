#pragma once

#include "Containers/TArray.h"
#include "Containers/TString.h"
#include "Core/Object.h"
#include "Core/SmartPointers.h"
#include "IO/Stream.h"
#include "Logging/LogCategory.h"
#include "Serializer.generate.h"

#include <type_traits>

namespace NLib
{
/**
 * @brief 序列化模式
 */
enum class ESerializationMode : uint8_t
{
	Serialize,  // 序列化（写入）
	Deserialize // 反序列化（读取）
};

/**
 * @brief 序列化格式
 */
enum class ESerializationFormat : uint8_t
{
	Binary, // 二进制格式
	JSON,   // JSON格式
	XML,    // XML格式（未来支持）
	Custom  // 自定义格式
};

/**
 * @brief 序列化版本信息
 */
struct SSerializationVersion
{
	uint32_t Major = 1;
	uint32_t Minor = 0;
	uint32_t Patch = 0;

	SSerializationVersion() = default;
	SSerializationVersion(uint32_t InMajor, uint32_t InMinor = 0, uint32_t InPatch = 0)
	    : Major(InMajor),
	      Minor(InMinor),
	      Patch(InPatch)
	{}

	bool operator==(const SSerializationVersion& Other) const
	{
		return Major == Other.Major && Minor == Other.Minor && Patch == Other.Patch;
	}

	bool operator!=(const SSerializationVersion& Other) const
	{
		return !(*this == Other);
	}

	bool operator<(const SSerializationVersion& Other) const
	{
		if (Major != Other.Major)
			return Major < Other.Major;
		if (Minor != Other.Minor)
			return Minor < Other.Minor;
		return Patch < Other.Patch;
	}

	bool IsCompatible(const SSerializationVersion& Other) const
	{
		// 主版本号相同且当前版本不低于目标版本
		return Major == Other.Major && !(*this < Other);
	}

	TString ToString() const
	{
		return TString::FromInt(Major) + TString(".") + TString::FromInt(Minor) + TString(".") +
		       TString::FromInt(Patch);
	}
};

/**
 * @brief 序列化结果
 */
struct SSerializationResult
{
	bool bSuccess = false;
	TString ErrorMessage;
	int32_t BytesProcessed = 0;

	SSerializationResult() = default;
	SSerializationResult(bool bInSuccess)
	    : bSuccess(bInSuccess)
	{}
	SSerializationResult(bool bInSuccess, const TString& InErrorMessage)
	    : bSuccess(bInSuccess),
	      ErrorMessage(InErrorMessage)
	{}
	SSerializationResult(bool bInSuccess, int32_t InBytesProcessed)
	    : bSuccess(bInSuccess),
	      BytesProcessed(InBytesProcessed)
	{}

	operator bool() const
	{
		return bSuccess;
	}

	TString ToString() const
	{
		if (bSuccess)
		{
			return TString("Success (") + TString::FromInt(BytesProcessed) + TString(" bytes)");
		}
		else
		{
			return TString("Error: ") + ErrorMessage;
		}
	}
};

/**
 * @brief 序列化标记
 */
enum class ESerializationFlags : uint32_t
{
	None = 0,
	SkipTransient = 1,       // 跳过临时属性
	IncludeTypeInfo = 2,     // 包含类型信息
	CompressData = 4,        // 压缩数据
	EncryptData = 8,         // 加密数据
	PrettyPrint = 16,        // 格式化输出（文本格式）
	IncludeMetadata = 32,    // 包含元数据
	ValidateOnRead = 64,     // 读取时验证
	AllowPartialRead = 128,  // 允许部分读取
	PreserveReferences = 256 // 保持引用关系
};

ENUM_CLASS_FLAGS(ESerializationFlags)

/**
 * @brief 序列化上下文
 */
struct SSerializationContext
{
	ESerializationMode Mode = ESerializationMode::Serialize;
	ESerializationFormat Format = ESerializationFormat::Binary;
	ESerializationFlags Flags = ESerializationFlags::None;
	SSerializationVersion Version;
	TString RootTypeName;
	void* UserData = nullptr;

	SSerializationContext() = default;
	SSerializationContext(ESerializationMode InMode, ESerializationFormat InFormat = ESerializationFormat::Binary)
	    : Mode(InMode),
	      Format(InFormat)
	{}

	bool IsSerializing() const
	{
		return Mode == ESerializationMode::Serialize;
	}
	bool IsDeserializing() const
	{
		return Mode == ESerializationMode::Deserialize;
	}
	bool HasFlag(ESerializationFlags Flag) const
	{
		return (static_cast<uint32_t>(Flags) & static_cast<uint32_t>(Flag)) != 0;
	}
};

/**
 * @brief 序列化档案基类
 *
 * 提供统一的序列化接口，支持不同的格式和流
 */
NCLASS()
class NSerializationArchive : public NObject
{
	GENERATED_BODY()

public:
	// === 构造函数 ===

	explicit NSerializationArchive(TSharedPtr<CStream> InStream, const SSerializationContext& InContext);
	virtual ~NSerializationArchive() = default;

public:
	// === 基本属性 ===

	const SSerializationContext& GetContext() const
	{
		return Context;
	}
	TSharedPtr<CStream> GetStream() const
	{
		return Stream;
	}
	bool IsValid() const
	{
		return Stream && Stream->CanRead() && Stream->CanWrite();
	}
	bool IsSerializing() const
	{
		return Context.IsSerializing();
	}
	bool IsDeserializing() const
	{
		return Context.IsDeserializing();
	}

public:
	// === 基础类型序列化 ===

	virtual SSerializationResult Serialize(bool& Value) = 0;
	virtual SSerializationResult Serialize(int8_t& Value) = 0;
	virtual SSerializationResult Serialize(uint8_t& Value) = 0;
	virtual SSerializationResult Serialize(int16_t& Value) = 0;
	virtual SSerializationResult Serialize(uint16_t& Value) = 0;
	virtual SSerializationResult Serialize(int32_t& Value) = 0;
	virtual SSerializationResult Serialize(uint32_t& Value) = 0;
	virtual SSerializationResult Serialize(int64_t& Value) = 0;
	virtual SSerializationResult Serialize(uint64_t& Value) = 0;
	virtual SSerializationResult Serialize(float& Value) = 0;
	virtual SSerializationResult Serialize(double& Value) = 0;
	virtual SSerializationResult Serialize(TString& Value) = 0;

public:
	// === 容器序列化 ===

	template <typename T, typename Allocator>
	SSerializationResult Serialize(TArray<T, Allocator>& Array)
	{
		if (IsSerializing())
		{
			int32_t Size = Array.Size();
			auto Result = Serialize(Size);
			if (!Result.bSuccess)
			{
				return Result;
			}

			for (int32_t i = 0; i < Size; ++i)
			{
				Result = Serialize(Array[i]);
				if (!Result.bSuccess)
				{
					return Result;
				}
			}
		}
		else
		{
			int32_t Size = 0;
			auto Result = Serialize(Size);
			if (!Result.bSuccess)
			{
				return Result;
			}

			Array.Clear();
			Array.Reserve(Size);

			for (int32_t i = 0; i < Size; ++i)
			{
				T Element{};
				Result = Serialize(Element);
				if (!Result.bSuccess)
				{
					return Result;
				}
				Array.Add(std::move(Element));
			}
		}

		return SSerializationResult(true);
	}

public:
	// === 智能指针序列化 ===

	template <typename T>
	SSerializationResult Serialize(TSharedPtr<T>& Ptr)
	{
		if (IsSerializing())
		{
			bool bIsValid = (Ptr != nullptr);
			auto Result = Serialize(bIsValid);
			if (!Result.bSuccess)
			{
				return Result;
			}

			if (bIsValid)
			{
				return Serialize(*Ptr);
			}
		}
		else
		{
			bool bIsValid = false;
			auto Result = Serialize(bIsValid);
			if (!Result.bSuccess)
			{
				return Result;
			}

			if (bIsValid)
			{
				Ptr = MakeShared<T>();
				return Serialize(*Ptr);
			}
			else
			{
				Ptr = nullptr;
			}
		}

		return SSerializationResult(true);
	}

public:
	// === 枚举序列化 ===

	template <typename T>
	SSerializationResult SerializeEnum(T& Value)
	{
		static_assert(std::is_enum_v<T>, "T must be an enum type");

		using UnderlyingType = std::underlying_type_t<T>;
		UnderlyingType UnderlyingValue = static_cast<UnderlyingType>(Value);

		auto Result = SerializeRaw(UnderlyingValue);
		if (Result.bSuccess && IsDeserializing())
		{
			Value = static_cast<T>(UnderlyingValue);
		}

		return Result;
	}

public:
	// === 原始数据序列化 ===

	template <typename T>
	SSerializationResult SerializeRaw(T& Value)
	{
		static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

		if (IsSerializing())
		{
			auto WriteResult = Stream->Write(reinterpret_cast<const uint8_t*>(&Value), sizeof(T));
			return SSerializationResult(WriteResult.bSuccess, WriteResult.BytesProcessed);
		}
		else
		{
			auto ReadResult = Stream->Read(reinterpret_cast<uint8_t*>(&Value), sizeof(T));
			return SSerializationResult(ReadResult.bSuccess && ReadResult.BytesProcessed == sizeof(T),
			                            ReadResult.BytesProcessed);
		}
	}

public:
	// === 命名字段序列化 ===

	template <typename T>
	SSerializationResult SerializeField(const TString& FieldName, T& Value)
	{
		auto Result = BeginField(FieldName);
		if (!Result.bSuccess)
		{
			return Result;
		}

		Result = Serialize(Value);
		if (!Result.bSuccess)
		{
			return Result;
		}

		return EndField(FieldName);
	}

public:
	// === 对象序列化 ===

	template <typename T>
	SSerializationResult SerializeObject(T& Object, const TString& TypeName = TString())
	{
		auto Result = BeginObject(TypeName.IsEmpty() ? GetTypeName<T>() : TypeName);
		if (!Result.bSuccess)
		{
			return Result;
		}

		// 如果对象有Serialize方法，使用它
		if constexpr (HasSerializeMethod<T>())
		{
			Result = Object.Serialize(*this);
		}
		else
		{
			// 否则尝试反射序列化
			Result = SerializeWithReflection(Object);
		}

		if (!Result.bSuccess)
		{
			return Result;
		}

		return EndObject(TypeName.IsEmpty() ? GetTypeName<T>() : TypeName);
	}

protected:
	// === 格式特定的虚函数 ===

	virtual SSerializationResult BeginObject(const TString& TypeName) = 0;
	virtual SSerializationResult EndObject(const TString& TypeName) = 0;
	virtual SSerializationResult BeginField(const TString& FieldName) = 0;
	virtual SSerializationResult EndField(const TString& FieldName) = 0;

protected:
	// === 类型特性检测 ===

	template <typename T>
	static constexpr bool HasSerializeMethod()
	{
		// 简化的检测，实际应使用SFINAE
		return false;
	}

	template <typename T>
	static TString GetTypeName()
	{
		// 简化的类型名获取，实际应使用反射系统
		return TString("Unknown");
	}

	template <typename T>
	SSerializationResult SerializeWithReflection(T& Object)
	{
		// 反射序列化的占位实现
		return SSerializationResult(false, "Reflection serialization not implemented");
	}

protected:
	TSharedPtr<CStream> Stream;
	SSerializationContext Context;
};

/**
 * @brief 序列化工厂
 */
class CSerializationFactory
{
public:
	/**
	 * @brief 创建序列化档案
	 */
	static TSharedPtr<NSerializationArchive> CreateArchive(TSharedPtr<CStream> Stream,
	                                                       const SSerializationContext& Context);

	/**
	 * @brief 创建二进制序列化档案
	 */
	static TSharedPtr<NSerializationArchive> CreateBinaryArchive(TSharedPtr<CStream> Stream, ESerializationMode Mode);

	/**
	 * @brief 创建JSON序列化档案
	 */
	static TSharedPtr<NSerializationArchive> CreateJsonArchive(TSharedPtr<CStream> Stream,
	                                                           ESerializationMode Mode,
	                                                           bool bPrettyPrint = false);
};

// === 便捷宏定义 ===

/**
 * @brief 声明序列化方法的宏
 */
#define DECLARE_SERIALIZABLE()                                                                                         \
public:                                                                                                                \
	virtual SSerializationResult Serialize(NSerializationArchive& Archive);

/**
 * @brief 实现序列化方法开始的宏
 */
#define IMPLEMENT_SERIALIZATION(ClassName) SSerializationResult ClassName::Serialize(NSerializationArchive& Archive)

/**
 * @brief 序列化字段的宏
 */
#define SERIALIZE_FIELD(Archive, FieldName)                                                                            \
	do                                                                                                                 \
	{                                                                                                                  \
		auto Result = Archive.SerializeField(#FieldName, FieldName);                                                   \
		if (!Result.bSuccess)                                                                                          \
			return Result;                                                                                             \
	} while (0)

// === 类型别名 ===
using FSerializationArchive = NSerializationArchive;
using FSerializationFactory = CSerializationFactory;
using FSerializationResult = SSerializationResult;
using FSerializationContext = SSerializationContext;
using FSerializationVersion = SSerializationVersion;

} // namespace NLib