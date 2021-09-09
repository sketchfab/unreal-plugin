// Copyright Epic Games, Inc. All Rights Reserved.

#include "Builders/SKGLTFContainerBuilder.h"

FGLTFContainerBuilder::FGLTFContainerBuilder(const FString& FilePath, const USKGLTFExportOptions* ExportOptions, bool bSelectedActorsOnly)
	: FGLTFConvertBuilder(FilePath, ExportOptions, bSelectedActorsOnly)
{
}

void FGLTFContainerBuilder::WriteGlb(FArchive& Archive)
{
	FBufferArchive JsonData;
	WriteJson(JsonData);

	const TArray<uint8>& BufferData = GetBufferData();

	WriteGlb(Archive, JsonData, BufferData);
}

void FGLTFContainerBuilder::WriteGlb(FArchive& Archive, const TArray<uint8>& JsonData, const TArray<uint8>& BinaryData)
{
	const uint32 JsonChunkType = 0x4E4F534A; // "JSON" in ASCII
	const uint32 BinaryChunkType = 0x004E4942; // "BIN" in ASCII
	const uint32 FileSize =
        3 * sizeof(uint32) +
        2 * sizeof(uint32) + GetPaddedChunkSize(JsonData.Num()) +
        2 * sizeof(uint32) + GetPaddedChunkSize(BinaryData.Num());

	WriteHeader(Archive, FileSize);
	WriteChunk(Archive, JsonChunkType, JsonData, 0x20);
	WriteChunk(Archive, BinaryChunkType, BinaryData, 0x0);
}

void FGLTFContainerBuilder::WriteHeader(FArchive& Archive, uint32 FileSize)
{
	const uint32 FileSignature = 0x46546C67; // "glTF" in ASCII
	const uint32 FileVersion = 2;

	WriteInt(Archive, FileSignature);
	WriteInt(Archive, FileVersion);
	WriteInt(Archive, FileSize);
}

void FGLTFContainerBuilder::WriteChunk(FArchive& Archive, uint32 ChunkType, const TArray<uint8>& ChunkData, uint8 ChunkTrailingByte)
{
	const uint32 ChunkLength = GetPaddedChunkSize(ChunkData.Num());
	const uint32 ChunkTrailing = GetTrailingChunkSize(ChunkData.Num());

	WriteInt(Archive, ChunkLength);
	WriteInt(Archive, ChunkType);
	WriteData(Archive, ChunkData);
	WriteFill(Archive, ChunkTrailing, ChunkTrailingByte);
}

void FGLTFContainerBuilder::WriteInt(FArchive& Archive, uint32 Value)
{
	Archive.SerializeInt(Value, MAX_uint32);
}

void FGLTFContainerBuilder::WriteData(FArchive& Archive, const TArray<uint8>& Data)
{
	Archive.Serialize(const_cast<uint8*>(Data.GetData()), Data.Num());
}

void FGLTFContainerBuilder::WriteFill(FArchive& Archive, int32 Size, uint8 Value)
{
	while (--Size >= 0)
	{
		Archive.Serialize(&Value, sizeof(Value));
	}
}

int32 FGLTFContainerBuilder::GetPaddedChunkSize(int32 Size)
{
	return (Size + 3) & ~3;
}

int32 FGLTFContainerBuilder::GetTrailingChunkSize(int32 Size)
{
	return (4 - (Size & 3)) & 3;
}
