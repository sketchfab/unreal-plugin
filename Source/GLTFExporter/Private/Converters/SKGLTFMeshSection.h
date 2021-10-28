// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Converters/SKGLTFIndexArray.h"

struct FGLTFMeshSection
{
	FGLTFMeshSection(const FStaticMeshLODResources* MeshLOD, const FGLTFIndexArray& SectionIndices);
	FGLTFMeshSection(const FSkeletalMeshLODRenderData* MeshLOD, const FGLTFIndexArray& SectionIndices);

	TArray<uint32> IndexMap;
	TArray<uint32> IndexBuffer;

	TArray<TArray<FBoneIndexType>> BoneMaps;
	TArray<uint32> BoneMapLookup;
	FBoneIndexType MaxBoneIndex;
};
