#pragma once

#include "Containers/THashMap.h"
#include "Serializer.h"

namespace NLib
{
/**
 * @brief 二进制序列化魔数
 */
struct SBinarySerializationHeader
{
	static constexpr uint32_t MAGIC_NUMBER = 0x4E4C4942; // "NLIB" in ASCII

	uint32_t Magic = MAGIC_NUMBER;
	uint32_t Version = 1;
	uint32_t Flags = 0;
	uint32_t Reserved = 0;

	bool IsValid() const
	{
		return Magic == MAGIC_NUMBER;
	}
};

/**
 * @brief 二进制序列化档案
 *
 * 提供高效的二进制序列化功能
 */
class CBinarySerializationArchive : public CSerializationArchive
{
	GENERATED_BODY()

public:
	// === 构造函数 ===

	explicit CBinarySerializationArchive(TSharedPtr<CStream> InStream, const SSerializationContext& InContext);
	~CBinarySerializationArchive() override = default;

public:
	// === 初始化和完成 ===

	/**
	 * @brief 初始化档案（写入或读取头部）
	 */
	SSerializationResult Initialize();

	/**
	 * @brief 完成档案（写入结束标记等）
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
	SSerializationResult Serialize(TString& Value) override;

public:
	// === 二进制特有的序列化方法 ===

	/**
	 * @brief 序列化原始字节数组
	 */
	SSerializationResult SerializeBytes(uint8_t* Data, int32_t Size);

	/**
	 * @brief 序列化带长度的字节数组
	 */
	SSerializationResult SerializeByteArray(TArray<uint8_t, CMemoryManager>& Array);

	/**
	 * @brief 序列化字符串（带长度前缀）
	 */
	SSerializationResult SerializeStringWithLength(TString& Str);

	/**
	 * @brief 序列化固定长度字符串
	 */
	SSerializationResult SerializeFixedString(char* Buffer, int32_t MaxLength);

public:
	// === 压缩和加密支持 ===

	/**
	 * @brief 序列化压缩数据块
	 */
	SSerializationResult SerializeCompressedBlock(TArray<uint8_t, CMemoryManager>& Data);

	/**
	 * @brief 序列化加密数据块
	 */
	SSerializationResult SerializeEncryptedBlock(TArray<uint8_t, CMemoryManager>& Data, const TString& Key);

public:
	// === 对象引用支持 ===

	/**
	 * @brief 序列化对象引用
	 */
	template <typename T>
	SSerializationResult SerializeReference(TSharedPtr<T>& Object)
	{
		if (IsSerializing())
		{
			return SerializeObjectReference(Object);
		}
		else
		{
			return DeserializeObjectReference(Object);
		}
	}

public:
	// === 类型信息支持 ===

	/**
	 * @brief 序列化类型信息
	 */
	SSerializationResult SerializeTypeInfo(const TString& TypeName, uint32_t TypeHash);

	/**
	 * @brief 验证类型信息
	 */
	SSerializationResult ValidateTypeInfo(const TString& ExpectedTypeName, uint32_t ExpectedTypeHash);

protected:
	// === CSerializationArchive 虚函数实现 ===

	SSerializationResult BeginObject(const TString& TypeName) override;
	SSerializationResult EndObject(const TString& TypeName) override;
	SSerializationResult BeginField(const TString& FieldName) override;
	SSerializationResult EndField(const TString& FieldName) override;

private:
	// === 内部实现 ===

	/**
	 * @brief 写入头部信息
	 */
	SSerializationResult WriteHeader();

	/**
	 * @brief 读取头部信息
	 */
	SSerializationResult ReadHeader();

	/**
	 * @brief 序列化对象引用的内部实现
	 */
	template <typename T>
	SSerializationResult SerializeObjectReference(TSharedPtr<T>& Object)
	{
		if (!Object)
		{
			uint32_t NullId = 0;
			return Serialize(NullId);
		}

		// 检查对象是否已经序列化过
		void* RawPtr = Object.Get();
		auto ExistingId = ObjectToIdMap.Find(RawPtr);
		if (ExistingId)
		{
			return Serialize(*ExistingId);
		}

		// 分配新的ID并序列化
		uint32_t NewId = ++NextObjectId;
		ObjectToIdMap.Add(RawPtr, NewId);

		auto Result = Serialize(NewId);
		if (!Result.bSuccess)
		{
			return Result;
		}

		// 序列化对象内容
		return SerializeObject(*Object);
	}

	/**
	 * @brief 反序列化对象引用的内部实现
	 */
	template <typename T>
	SSerializationResult DeserializeObjectReference(TSharedPtr<T>& Object)
	{
		uint32_t ObjectId = 0;
		auto Result = Serialize(ObjectId);
		if (!Result.bSuccess)
		{
			return Result;
		}

		if (ObjectId == 0)
		{
			Object = nullptr;
			return SSerializationResult(true);
		}

		// 检查对象是否已经反序列化过
		auto ExistingObject = IdToObjectMap.Find(ObjectId);
		if (ExistingObject)
		{
			Object = std::static_pointer_cast<T>(*ExistingObject);
			return SSerializationResult(true);
		}

		// 创建新对象并反序列化
		Object = MakeShared<T>();
		IdToObjectMap.Add(ObjectId, Object);

		return SerializeObject(*Object);
	}

	/**
	 * @brief 字节序转换
	 */
	template <typename T>
	void ConvertEndianness(T& Value)
	{
		if constexpr (sizeof(T) > 1)
		{
			// 简化实现，实际应该检查系统字节序
			uint8_t* Bytes = reinterpret_cast<uint8_t*>(&Value);
			for (size_t i = 0; i < sizeof(T) / 2; ++i)
			{
				std::swap(Bytes[i], Bytes[sizeof(T) - 1 - i]);
			}
		}
	}

private:
	SBinarySerializationHeader Header;
	bool bHeaderInitialized = false;

	// 对象引用管理
	uint32_t NextObjectId = 0;
	THashMap<void*, uint32_t, CMemoryManager> ObjectToIdMap;            // 序列化：对象指针 -> ID
	THashMap<uint32_t, TSharedPtr<void>, CMemoryManager> IdToObjectMap; // 反序列化：ID -> 对象

	// 嵌套计数
	int32_t ObjectNestingLevel = 0;
	TArray<TString, CMemoryManager> ObjectTypeStack;
};

/**
 * @brief 二进制序列化帮助器
 */
class CBinarySerializationHelper
{
public:
	/**
	 * @brief 序列化对象到流
	 */
	template <typename T>
	static SSerializationResult SerializeToStream(const T& Object,
	                                              TSharedPtr<CStream> Stream,
	                                              ESerializationFlags Flags = ESerializationFlags::None)
	{
		SSerializationContext Context(ESerializationMode::Serialize, ESerializationFormat::Binary);
		Context.Flags = Flags;

		auto Archive = MakeShared<CBinarySerializationArchive>(Stream, Context);
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
	 * @brief 从流反序列化对象
	 */
	template <typename T>
	static SSerializationResult DeserializeFromStream(T& Object,
	                                                  TSharedPtr<CStream> Stream,
	                                                  ESerializationFlags Flags = ESerializationFlags::None)
	{
		SSerializationContext Context(ESerializationMode::Deserialize, ESerializationFormat::Binary);
		Context.Flags = Flags;

		auto Archive = MakeShared<CBinarySerializationArchive>(Stream, Context);
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
	 * @brief 序列化对象到字节数组
	 */
	template <typename T>
	static TArray<uint8_t, CMemoryManager> SerializeToBytes(const T& Object,
	                                                        ESerializationFlags Flags = ESerializationFlags::None)
	{
		auto MemoryStream = MakeShared<CMemoryStream>();
		auto Result = SerializeToStream(Object, MemoryStream, Flags);

		if (Result.bSuccess)
		{
			return MemoryStream->GetBuffer();
		}
		else
		{
			NLOG_SERIALIZATION(Error, "Failed to serialize object to bytes: {}", Result.ErrorMessage.GetData());
			return TArray<uint8_t, CMemoryManager>();
		}
	}

	/**
	 * @brief 从字节数组反序列化对象
	 */
	template <typename T>
	static bool DeserializeFromBytes(T& Object,
	                                 const TArray<uint8_t, CMemoryManager>& Data,
	                                 ESerializationFlags Flags = ESerializationFlags::None)
	{
		auto MemoryStream = MakeShared<CMemoryStream>(Data);
		auto Result = DeserializeFromStream(Object, MemoryStream, Flags);

		if (!Result.bSuccess)
		{
			NLOG_SERIALIZATION(Error, "Failed to deserialize object from bytes: {}", Result.ErrorMessage.GetData());
		}

		return Result.bSuccess;
	}
};

// === 类型别名 ===
using FBinarySerializationArchive = CBinarySerializationArchive;
using FBinarySerializationHelper = CBinarySerializationHelper;

} // namespace NLib