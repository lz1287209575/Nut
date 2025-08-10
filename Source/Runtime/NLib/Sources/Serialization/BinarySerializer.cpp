#include "BinarySerializer.h"

#include "Logging/LogCategory.h"

#include <algorithm>
#include <cstring>

namespace NLib
{
// === CBinarySerializationArchive 实现 ===

CBinarySerializationArchive::CBinarySerializationArchive(TSharedPtr<NStream> InStream,
                                                         const SSerializationContext& InContext)
    : CSerializationArchive(InStream, InContext)
{}

SSerializationResult CBinarySerializationArchive::Initialize()
{
	if (bHeaderInitialized)
	{
		return SSerializationResult(true);
	}

	if (IsSerializing())
	{
		auto Result = WriteHeader();
		if (Result.bSuccess)
		{
			bHeaderInitialized = true;
		}
		return Result;
	}
	else
	{
		auto Result = ReadHeader();
		if (Result.bSuccess)
		{
			bHeaderInitialized = true;
		}
		return Result;
	}
}

SSerializationResult CBinarySerializationArchive::Finalize()
{
	if (!bHeaderInitialized)
	{
		return SSerializationResult(false, "Archive not initialized");
	}

	// 刷新流缓冲区
	if (Stream && !Stream->Flush())
	{
		return SSerializationResult(false, "Failed to flush stream");
	}

	return SSerializationResult(true);
}

// === 基础类型序列化实现 ===

SSerializationResult CBinarySerializationArchive::Serialize(bool& Value)
{
	uint8_t ByteValue = Value ? 1 : 0;
	auto Result = SerializeRaw(ByteValue);
	if (Result.bSuccess && IsDeserializing())
	{
		Value = (ByteValue != 0);
	}
	return Result;
}

SSerializationResult CBinarySerializationArchive::Serialize(int8_t& Value)
{
	return SerializeRaw(Value);
}

SSerializationResult CBinarySerializationArchive::Serialize(uint8_t& Value)
{
	return SerializeRaw(Value);
}

SSerializationResult CBinarySerializationArchive::Serialize(int16_t& Value)
{
	return SerializeRaw(Value);
}

SSerializationResult CBinarySerializationArchive::Serialize(uint16_t& Value)
{
	return SerializeRaw(Value);
}

SSerializationResult CBinarySerializationArchive::Serialize(int32_t& Value)
{
	return SerializeRaw(Value);
}

SSerializationResult CBinarySerializationArchive::Serialize(uint32_t& Value)
{
	return SerializeRaw(Value);
}

SSerializationResult CBinarySerializationArchive::Serialize(int64_t& Value)
{
	return SerializeRaw(Value);
}

SSerializationResult CBinarySerializationArchive::Serialize(uint64_t& Value)
{
	return SerializeRaw(Value);
}

SSerializationResult CBinarySerializationArchive::Serialize(float& Value)
{
	return SerializeRaw(Value);
}

SSerializationResult CBinarySerializationArchive::Serialize(double& Value)
{
	return SerializeRaw(Value);
}

SSerializationResult CBinarySerializationArchive::Serialize(CString& Value)
{
	if (IsSerializing())
	{
		uint32_t Length = static_cast<uint32_t>(Value.Length());
		auto Result = Serialize(Length);
		if (!Result.bSuccess)
		{
			return Result;
		}

		if (Length > 0)
		{
			auto WriteResult = Stream->Write(reinterpret_cast<const uint8_t*>(Value.GetData()), Length);
			return SSerializationResult(WriteResult.bSuccess, WriteResult.BytesProcessed);
		}
	}
	else
	{
		uint32_t Length = 0;
		auto Result = Serialize(Length);
		if (!Result.bSuccess)
		{
			return Result;
		}

		if (Length > 0)
		{
			// 安全检查：防止过大的字符串长度
			if (Length > 0x10000000) // 256MB limit
			{
				return SSerializationResult(false, "String length too large");
			}

			TArray<char, CMemoryManager> Buffer(Length + 1);
			auto ReadResult = Stream->Read(reinterpret_cast<uint8_t*>(Buffer.GetData()), Length);
			if (!ReadResult.bSuccess || ReadResult.BytesProcessed != static_cast<int32_t>(Length))
			{
				return SSerializationResult(false, "Failed to read string data");
			}

			Buffer[Length] = '\0';
			Value = CString(Buffer.GetData(), Length);
		}
		else
		{
			Value = CString();
		}
	}

	return SSerializationResult(true);
}

// === 二进制特有的序列化方法 ===

SSerializationResult CBinarySerializationArchive::SerializeBytes(uint8_t* Data, int32_t Size)
{
	if (!Data || Size <= 0)
	{
		return SSerializationResult(false, "Invalid data or size");
	}

	if (IsSerializing())
	{
		auto WriteResult = Stream->Write(Data, Size);
		return SSerializationResult(WriteResult.bSuccess, WriteResult.BytesProcessed);
	}
	else
	{
		auto ReadResult = Stream->Read(Data, Size);
		return SSerializationResult(ReadResult.bSuccess && ReadResult.BytesProcessed == Size,
		                            ReadResult.BytesProcessed);
	}
}

SSerializationResult CBinarySerializationArchive::SerializeByteArray(TArray<uint8_t, CMemoryManager>& Array)
{
	if (IsSerializing())
	{
		uint32_t Size = static_cast<uint32_t>(Array.Size());
		auto Result = Serialize(Size);
		if (!Result.bSuccess)
		{
			return Result;
		}

		if (Size > 0)
		{
			return SerializeBytes(Array.GetData(), Size);
		}
	}
	else
	{
		uint32_t Size = 0;
		auto Result = Serialize(Size);
		if (!Result.bSuccess)
		{
			return Result;
		}

		if (Size > 0)
		{
			// 安全检查
			if (Size > 0x40000000) // 1GB limit
			{
				return SSerializationResult(false, "Byte array size too large");
			}

			Array.Resize(Size);
			return SerializeBytes(Array.GetData(), Size);
		}
		else
		{
			Array.Clear();
		}
	}

	return SSerializationResult(true);
}

SSerializationResult CBinarySerializationArchive::SerializeStringWithLength(CString& Str)
{
	return Serialize(Str); // 默认实现已经包含长度
}

SSerializationResult CBinarySerializationArchive::SerializeFixedString(char* Buffer, int32_t MaxLength)
{
	if (!Buffer || MaxLength <= 0)
	{
		return SSerializationResult(false, "Invalid buffer or length");
	}

	if (IsSerializing())
	{
		int32_t ActualLength = static_cast<int32_t>(strlen(Buffer));
		ActualLength = std::min(ActualLength, MaxLength - 1);

		// 写入固定长度的缓冲区
		TArray<char, CMemoryManager> TempBuffer(MaxLength);
		memset(TempBuffer.GetData(), 0, MaxLength);
		memcpy(TempBuffer.GetData(), Buffer, ActualLength);

		return SerializeBytes(reinterpret_cast<uint8_t*>(TempBuffer.GetData()), MaxLength);
	}
	else
	{
		auto Result = SerializeBytes(reinterpret_cast<uint8_t*>(Buffer), MaxLength);
		if (Result.bSuccess)
		{
			Buffer[MaxLength - 1] = '\0'; // 确保字符串结束
		}
		return Result;
	}
}

SSerializationResult CBinarySerializationArchive::SerializeCompressedBlock(TArray<uint8_t, CMemoryManager>& Data)
{
	// 压缩功能的占位实现
	NLOG_SERIALIZATION(Warning, "Compression not implemented yet");
	return SerializeByteArray(Data);
}

SSerializationResult CBinarySerializationArchive::SerializeEncryptedBlock(TArray<uint8_t, CMemoryManager>& Data,
                                                                          const CString& Key)
{
	// 加密功能的占位实现
	NLOG_SERIALIZATION(Warning, "Encryption not implemented yet");
	return SerializeByteArray(Data);
}

// === 类型信息支持 ===

SSerializationResult CBinarySerializationArchive::SerializeTypeInfo(const CString& TypeName, uint32_t TypeHash)
{
	if (!Context.HasFlag(ESerializationFlags::IncludeTypeInfo))
	{
		return SSerializationResult(true); // 跳过类型信息
	}

	auto Result = Serialize(const_cast<CString&>(TypeName));
	if (!Result.bSuccess)
	{
		return Result;
	}

	return Serialize(const_cast<uint32_t&>(TypeHash));
}

SSerializationResult CBinarySerializationArchive::ValidateTypeInfo(const CString& ExpectedTypeName,
                                                                   uint32_t ExpectedTypeHash)
{
	if (!Context.HasFlag(ESerializationFlags::IncludeTypeInfo))
	{
		return SSerializationResult(true); // 跳过类型验证
	}

	CString ActualTypeName;
	auto Result = Serialize(ActualTypeName);
	if (!Result.bSuccess)
	{
		return Result;
	}

	uint32_t ActualTypeHash = 0;
	Result = Serialize(ActualTypeHash);
	if (!Result.bSuccess)
	{
		return Result;
	}

	if (ActualTypeName != ExpectedTypeName || ActualTypeHash != ExpectedTypeHash)
	{
		return SSerializationResult(
		    false, CString("Type mismatch: expected ") + ExpectedTypeName + CString(", got ") + ActualTypeName);
	}

	return SSerializationResult(true);
}

// === CSerializationArchive 虚函数实现 ===

SSerializationResult CBinarySerializationArchive::BeginObject(const CString& TypeName)
{
	ObjectNestingLevel++;
	ObjectTypeStack.PushBack(TypeName);

	// 可选地写入类型信息
	if (Context.HasFlag(ESerializationFlags::IncludeTypeInfo))
	{
		uint32_t TypeHash = 0; // 简化实现，实际应计算类型哈希
		auto Result = SerializeTypeInfo(TypeName, TypeHash);
		if (!Result.bSuccess)
		{
			return Result;
		}
	}

	return SSerializationResult(true);
}

SSerializationResult CBinarySerializationArchive::EndObject(const CString& TypeName)
{
	if (ObjectNestingLevel <= 0 || ObjectTypeStack.IsEmpty())
	{
		return SSerializationResult(false, "Mismatched EndObject call");
	}

	CString ExpectedType = ObjectTypeStack.Last();
	ObjectTypeStack.PopBack();
	ObjectNestingLevel--;

	if (ExpectedType != TypeName)
	{
		return SSerializationResult(
		    false, CString("Type mismatch in EndObject: expected ") + ExpectedType + CString(", got ") + TypeName);
	}

	return SSerializationResult(true);
}

SSerializationResult CBinarySerializationArchive::BeginField(const CString& FieldName)
{
	// 二进制格式通常不需要字段名，但可以可选地包含用于调试
	if (Context.HasFlag(ESerializationFlags::IncludeMetadata))
	{
		return Serialize(const_cast<CString&>(FieldName));
	}

	return SSerializationResult(true);
}

SSerializationResult CBinarySerializationArchive::EndField(const CString& FieldName)
{
	// 二进制格式通常不需要字段结束标记
	return SSerializationResult(true);
}

// === 内部实现 ===

SSerializationResult CBinarySerializationArchive::WriteHeader()
{
	Header.Magic = SBinarySerializationHeader::MAGIC_NUMBER;
	Header.Version = 1;
	Header.Flags = static_cast<uint32_t>(Context.Flags);
	Header.Reserved = 0;

	auto WriteResult = Stream->Write(reinterpret_cast<const uint8_t*>(&Header), sizeof(Header));
	if (!WriteResult.bSuccess || WriteResult.BytesProcessed != sizeof(Header))
	{
		return SSerializationResult(false, "Failed to write binary header");
	}

	NLOG_SERIALIZATION(Info, "Binary serialization header written");
	return SSerializationResult(true, WriteResult.BytesProcessed);
}

SSerializationResult CBinarySerializationArchive::ReadHeader()
{
	auto ReadResult = Stream->Read(reinterpret_cast<uint8_t*>(&Header), sizeof(Header));
	if (!ReadResult.bSuccess || ReadResult.BytesProcessed != sizeof(Header))
	{
		return SSerializationResult(false, "Failed to read binary header");
	}

	if (!Header.IsValid())
	{
		return SSerializationResult(false, CString("Invalid binary header magic: ") + CString::FromInt(static_cast<int32_t>(Header.Magic)));
	}

	if (Header.Version > 1)
	{
		return SSerializationResult(false, CString("Unsupported binary version: ") + CString::FromInt(Header.Version));
	}

	// 更新上下文标志
	Context.Flags = static_cast<ESerializationFlags>(Header.Flags);

	NLOG_SERIALIZATION(Info, "Binary serialization header read: version {}, flags {}", Header.Version, Header.Flags);
	return SSerializationResult(true, ReadResult.BytesProcessed);
}

} // namespace NLib