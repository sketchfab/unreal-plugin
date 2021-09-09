// Copyright Epic Games, Inc. All Rights Reserved.

#include "Builders/SKGLTFBufferBuilder.h"
#include "Serialization/BufferArchive.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

FGLTFBufferBuilder::FGLTFBufferBuilder(const FString& FilePath, const USKGLTFExportOptions* ExportOptions)
	: FGLTFJsonBuilder(FilePath, ExportOptions)
{
	FGLTFJsonBuffer JsonBuffer;

	if (bIsGlbFile)
	{
		BufferArchive = MakeUnique<FBufferArchive>();
	}
	else
	{
		const FString ExternalBinaryPath = FPaths::ChangeExtension(FilePath, TEXT(".bin"));
		JsonBuffer.URI = FPaths::GetCleanFilename(ExternalBinaryPath);

		BufferArchive.Reset(IFileManager::Get().CreateFileWriter(*ExternalBinaryPath));
		if (BufferArchive == nullptr)
		{
			AddErrorMessage(FString::Printf(TEXT("Failed to write external binary buffer to file: %s"), *ExternalBinaryPath));
		}
	}

	BufferIndex = AddBuffer(JsonBuffer);
}

FGLTFBufferBuilder::~FGLTFBufferBuilder()
{
	if (BufferArchive != nullptr)
	{
		BufferArchive->Close();
	}
}

const TArray<uint8>& FGLTFBufferBuilder::GetBufferData() const
{
	check(bIsGlbFile);
	return *static_cast<FBufferArchive*>(BufferArchive.Get());
}

FGLTFJsonBufferViewIndex FGLTFBufferBuilder::AddBufferView(const void* RawData, uint64 ByteLength, ESKGLTFJsonBufferTarget BufferTarget, uint8 DataAlignment)
{
	if (BufferArchive == nullptr)
	{
		// TODO: report error
		return FGLTFJsonBufferViewIndex(INDEX_NONE);
	}

	uint64 ByteOffset = BufferArchive->Tell();

	// Data offset must be a multiple of the size of the glTF component type (given by ByteAlignment).
	const int64 Padding = (DataAlignment - (ByteOffset % DataAlignment)) % DataAlignment;
	if (Padding > 0)
	{
		ByteOffset += Padding;
		BufferArchive->Seek(ByteOffset);
	}

	BufferArchive->Serialize(const_cast<void*>(RawData), ByteLength);

	FGLTFJsonBuffer& JsonBuffer = GetBuffer(BufferIndex);
	JsonBuffer.ByteLength = BufferArchive->Tell();

	FGLTFJsonBufferView JsonBufferView;
	JsonBufferView.Buffer = BufferIndex;
	JsonBufferView.ByteOffset = ByteOffset;
	JsonBufferView.ByteLength = ByteLength;
	JsonBufferView.Target = BufferTarget;

	return FGLTFJsonBuilder::AddBufferView(JsonBufferView);
}
