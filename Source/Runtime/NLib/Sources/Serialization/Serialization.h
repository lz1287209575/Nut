#pragma once

/**
 * @file Serialization.h
 * @brief NLib序列化模块主头文件
 *
 * 提供完整的序列化功能，包括：
 * - 二进制序列化（高效、紧凑）
 * - JSON序列化（可读、互操作）
 * - 反射序列化（自动化、类型安全）
 * - 版本控制和兼容性支持
 */

#include "BinarySerializer.h"
#include "JsonSerializer.h"
#include "Serializer.h"

namespace NLib
{
/**
 * @brief 序列化模块命名空间
 */
namespace Serialization
{
// === 核心类型别名 ===
using Archive = CSerializationArchive;
using BinaryArchive = CBinarySerializationArchive;
using JsonArchive = CJsonSerializationArchive;
using Factory = CSerializationFactory;

using Context = SSerializationContext;
using Result = SSerializationResult;
using Mode = ESerializationMode;
using Format = ESerializationFormat;
using Flags = ESerializationFlags;

// === 帮助器类别名 ===
using BinaryHelper = CBinarySerializationHelper;
using JsonHelper = CJsonSerializationHelper;

// === 便利函数 ===

/**
 * @brief 序列化对象到二进制数据
 */
template <typename T>
inline TArray<uint8_t, CMemoryManager> ToBinary(const T& Object, Flags SerializationFlags = Flags::None)
{
	return BinaryHelper::SerializeToBytes(Object, SerializationFlags);
}

/**
 * @brief 从二进制数据反序列化对象
 */
template <typename T>
inline bool FromBinary(T& Object, const TArray<uint8_t, CMemoryManager>& Data, Flags SerializationFlags = Flags::None)
{
	return BinaryHelper::DeserializeFromBytes(Object, Data, SerializationFlags);
}

/**
 * @brief 序列化对象到JSON字符串
 */
template <typename T>
inline CString ToJson(const T& Object, bool bPrettyPrint = true)
{
	return JsonHelper::SerializeToString(Object, bPrettyPrint);
}

/**
 * @brief 从JSON字符串反序列化对象
 */
template <typename T>
inline bool FromJson(T& Object, const CString& JsonString)
{
	return JsonHelper::DeserializeFromString(Object, JsonString);
}

/**
 * @brief 序列化对象到流
 */
template <typename T>
inline Result ToStream(const T& Object,
                       TSharedPtr<NStream> Stream,
                       Format SerializationFormat = Format::Binary,
                       Flags SerializationFlags = Flags::None)
{
	Context Ctx(Mode::Serialize, SerializationFormat);
	Ctx.Flags = SerializationFlags;

	auto Archive = Factory::CreateArchive(Stream, Ctx);
	if (!Archive)
	{
		return Result(false, "Failed to create serialization archive");
	}

	auto InitResult = Archive->Initialize();
	if (!InitResult.bSuccess)
	{
		return InitResult;
	}

	T& MutableObject = const_cast<T&>(Object);
	auto SerializeResult = Archive->SerializeObject(MutableObject);
	if (!SerializeResult.bSuccess)
	{
		return SerializeResult;
	}

	return Archive->Finalize();
}

/**
 * @brief 从流反序列化对象
 */
template <typename T>
inline Result FromStream(T& Object,
                         TSharedPtr<NStream> Stream,
                         Format SerializationFormat = Format::Binary,
                         Flags SerializationFlags = Flags::None)
{
	Context Ctx(Mode::Deserialize, SerializationFormat);
	Ctx.Flags = SerializationFlags;

	auto Archive = Factory::CreateArchive(Stream, Ctx);
	if (!Archive)
	{
		return Result(false, "Failed to create serialization archive");
	}

	auto InitResult = Archive->Initialize();
	if (!InitResult.bSuccess)
	{
		return InitResult;
	}

	auto SerializeResult = Archive->SerializeObject(Object);
	if (!SerializeResult.bSuccess)
	{
		return SerializeResult;
	}

	return Archive->Finalize();
}

/**
 * @brief 创建二进制序列化档案
 */
inline TSharedPtr<BinaryArchive> CreateBinaryArchive(TSharedPtr<NStream> Stream,
                                                     Mode SerializationMode,
                                                     Flags SerializationFlags = Flags::None)
{
	Context Ctx(SerializationMode, Format::Binary);
	Ctx.Flags = SerializationFlags;
	return std::static_pointer_cast<BinaryArchive>(Factory::CreateArchive(Stream, Ctx));
}

/**
 * @brief 创建JSON序列化档案
 */
inline TSharedPtr<JsonArchive> CreateJsonArchive(TSharedPtr<NStream> Stream,
                                                 Mode SerializationMode,
                                                 bool bPrettyPrint = true)
{
	Context Ctx(SerializationMode, Format::JSON);
	if (bPrettyPrint)
	{
		Ctx.Flags = static_cast<Flags>(static_cast<uint32_t>(Ctx.Flags) | static_cast<uint32_t>(Flags::PrettyPrint));
	}
	return std::static_pointer_cast<JsonArchive>(Factory::CreateArchive(Stream, Ctx));
}

/**
 * @brief 检查对象是否可序列化
 */
template <typename T>
constexpr bool IsSerializable()
{
	// 简化实现，实际应该检查是否有序列化函数
	return std::is_class_v<T> || std::is_fundamental_v<T>;
}

/**
 * @brief 获取对象序列化大小估计（二进制格式）
 */
template <typename T>
inline int32_t GetSerializationSizeEstimate(const T& Object)
{
	// 简化实现，返回基础估计
	if constexpr (std::is_fundamental_v<T>)
	{
		return sizeof(T);
	}
	else
	{
		// 对于复杂对象，返回保守估计
		return 1024; // 1KB估计
	}
}
} // namespace Serialization

// === 全局便利函数 ===

/**
 * @brief 全局序列化到二进制
 */
template <typename T>
TArray<uint8_t, CMemoryManager> SerializeToBinary(const T& Object,
                                                  ESerializationFlags Flags = ESerializationFlags::None)
{
	return Serialization::ToBinary(Object, Flags);
}

/**
 * @brief 全局从二进制反序列化
 */
template <typename T>
bool DeserializeFromBinary(T& Object,
                           const TArray<uint8_t, CMemoryManager>& Data,
                           ESerializationFlags Flags = ESerializationFlags::None)
{
	return Serialization::FromBinary(Object, Data, Flags);
}

/**
 * @brief 全局序列化到JSON
 */
template <typename T>
CString SerializeToJson(const T& Object, bool bPrettyPrint = true)
{
	return Serialization::ToJson(Object, bPrettyPrint);
}

/**
 * @brief 全局从JSON反序列化
 */
template <typename T>
bool DeserializeFromJson(T& Object, const CString& JsonString)
{
	return Serialization::FromJson(Object, JsonString);
}

} // namespace NLib

// === 序列化宏 ===

/**
 * @brief 为类声明序列化支持
 */
#define NLIB_SERIALIZABLE(ClassName)                                                                                   \
	friend class NLib::CSerializationArchive;                                                                          \
	SSerializationResult Serialize(NLib::CSerializationArchive& Archive);

/**
 * @brief 实现基础类型的序列化函数
 */
#define NLIB_IMPLEMENT_SERIALIZATION(ClassName)                                                                        \
	SSerializationResult ClassName::Serialize(NLib::CSerializationArchive& Archive)

/**
 * @brief 序列化字段的便利宏
 */
#define NLIB_SERIALIZE_FIELD(Archive, FieldName)                                                                       \
	do                                                                                                                 \
	{                                                                                                                  \
		auto Result = Archive.BeginField(#FieldName);                                                                  \
		if (!Result.bSuccess)                                                                                          \
			return Result;                                                                                             \
		Result = Archive.Serialize(FieldName);                                                                         \
		if (!Result.bSuccess)                                                                                          \
			return Result;                                                                                             \
		Result = Archive.EndField(#FieldName);                                                                         \
		if (!Result.bSuccess)                                                                                          \
			return Result;                                                                                             \
	} while (0)

/**
 * @brief 序列化可选字段的便利宏
 */
#define NLIB_SERIALIZE_OPTIONAL_FIELD(Archive, FieldName, DefaultValue)                                                \
	do                                                                                                                 \
	{                                                                                                                  \
		if (Archive.IsSerializing() || Archive.HasField(#FieldName))                                                   \
		{                                                                                                              \
			auto Result = Archive.BeginField(#FieldName);                                                              \
			if (!Result.bSuccess && Archive.IsDeserializing())                                                         \
			{                                                                                                          \
				FieldName = DefaultValue;                                                                              \
				break;                                                                                                 \
			}                                                                                                          \
			if (!Result.bSuccess)                                                                                      \
				return Result;                                                                                         \
			Result = Archive.Serialize(FieldName);                                                                     \
			if (!Result.bSuccess)                                                                                      \
				return Result;                                                                                         \
			Result = Archive.EndField(#FieldName);                                                                     \
			if (!Result.bSuccess)                                                                                      \
				return Result;                                                                                         \
		}                                                                                                              \
		else if (Archive.IsDeserializing())                                                                            \
		{                                                                                                              \
			FieldName = DefaultValue;                                                                                  \
		}                                                                                                              \
	} while (0)