// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Crc.h"

class FGLTFBinaryHashKey
{
public:

	FGLTFBinaryHashKey(const void* RawData, int64 ByteLength)
		: Hash(FCrc::MemCrc32(RawData, ByteLength))
		, Bytes(static_cast<const uint8*>(RawData), ByteLength)
	{
	}

	bool operator==(const FGLTFBinaryHashKey& Other) const
	{
		return CompareBytes(Other);
	}

	bool operator!=(const FGLTFBinaryHashKey& Other) const
	{
		return !CompareBytes(Other);
	}

	friend uint32 GetTypeHash(const FGLTFBinaryHashKey& Other)
	{
		return Other.Hash;
	}

private:

	bool CompareBytes(const FGLTFBinaryHashKey& Other) const
	{
		return Bytes.Num() == Other.Bytes.Num()
			&& FMemory::Memcmp(Bytes.GetData(), Other.Bytes.GetData(), Bytes.Num()) == 0;
	}

	const uint32 Hash;
	const TArray64<uint8> Bytes;
};
