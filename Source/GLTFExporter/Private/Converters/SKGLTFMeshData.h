// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MeshDescription.h"

struct FGLTFMeshData
{
	FGLTFMeshData(const UStaticMesh* StaticMesh, const UStaticMeshComponent* StaticMeshComponent, int32 LODIndex);
	FGLTFMeshData(const USkeletalMesh* SkeletalMesh, const USkeletalMeshComponent* SkeletalMeshComponent, int32 LODIndex);

	const FGLTFMeshData* GetParent() const;

	FString Name;
	FMeshDescription Description;
	int32 TexCoord;

	// TODO: find a better name for referencing the mesh-only data (no component)
	const FGLTFMeshData* Parent;
};
