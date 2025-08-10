#include "Serializer.h"

#include "BinarySerializer.h"
#include "JsonSerializer.h"
#include "Logging/LogCategory.h"

namespace NLib
{
// === NSerializationArchive 实现 ===

NSerializationArchive::NSerializationArchive(TSharedPtr<NStream> InStream, const SSerializationContext& InContext)
    : Stream(InStream),
      Context(InContext)
{
	if (!Stream)
	{
		NLOG_SERIALIZATION(Error, "Invalid stream provided to serialization archive");
	}
}

// === CSerializationFactory 实现 ===

TSharedPtr<NSerializationArchive> CSerializationFactory::CreateArchive(TSharedPtr<NStream> Stream,
                                                                       const SSerializationContext& Context)
{
	if (!Stream)
	{
		NLOG_SERIALIZATION(Error, "Cannot create archive with null stream");
		return nullptr;
	}

	switch (Context.Format)
	{
	case ESerializationFormat::Binary:
		return MakeShared<CBinarySerializationArchive>(Stream, Context);

	case ESerializationFormat::JSON:
		return MakeShared<CJsonSerializationArchive>(Stream, Context);

	case ESerializationFormat::XML:
		NLOG_SERIALIZATION(Warning, "XML serialization not implemented yet");
		return nullptr;

	case ESerializationFormat::Custom:
		NLOG_SERIALIZATION(Warning, "Custom serialization format not supported by factory");
		return nullptr;

	default:
		NLOG_SERIALIZATION(Error, "Unknown serialization format: {}", static_cast<int>(Context.Format));
		return nullptr;
	}
}

TSharedPtr<NSerializationArchive> CSerializationFactory::CreateBinaryArchive(TSharedPtr<NStream> Stream,
                                                                             ESerializationMode Mode)
{
	SSerializationContext Context(Mode, ESerializationFormat::Binary);
	return CreateArchive(Stream, Context);
}

TSharedPtr<NSerializationArchive> CSerializationFactory::CreateJsonArchive(TSharedPtr<NStream> Stream,
                                                                           ESerializationMode Mode,
                                                                           bool bPrettyPrint)
{
	SSerializationContext Context(Mode, ESerializationFormat::JSON);
	if (bPrettyPrint)
	{
		Context.Flags = static_cast<ESerializationFlags>(static_cast<uint32_t>(Context.Flags) |
		                                                 static_cast<uint32_t>(ESerializationFlags::PrettyPrint));
	}

	return CreateArchive(Stream, Context);
}

} // namespace NLib