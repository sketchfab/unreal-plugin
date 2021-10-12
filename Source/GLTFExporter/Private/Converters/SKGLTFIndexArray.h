// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

typedef TArray<int32> FGLTFIndexArray;

#if !( (ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 25) || (ENGINE_MAJOR_VERSION >= 5) )
inline uint32 GetTypeHash(const FGLTFIndexArray& IndexArray)
{
	uint32 Hash = GetTypeHash(IndexArray.Num());
	for (int32 Index : IndexArray)
	{
		Hash = HashCombine(Hash, GetTypeHash(Index));
	}
	return Hash;
}
#endif
