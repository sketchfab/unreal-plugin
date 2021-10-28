// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;

typedef TArray<const UMaterialInterface*> FGLTFMaterialArray;

inline uint32 GetTypeHash(const FGLTFMaterialArray& MaterialArray)
{
	uint32 Hash = GetTypeHash(MaterialArray.Num());
	for (const UMaterialInterface* Material : MaterialArray)
	{
		Hash = HashCombine(Hash, GetTypeHash(Material));
	}
	return Hash;
}
