// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Tasks/SKGLTFTask.h"
#include "Builders/SKGLTFConvertBuilder.h"
#include "Converters/SKGLTFMeshSectionConverters.h"
#include "Converters/SKGLTFMaterialArray.h"
#include "Converters/SKGLTFNameUtility.h"
#include "Engine.h"

class FGLTFStaticMeshTask : public FGLTFTask
{
public:

	FGLTFStaticMeshTask(FGLTFConvertBuilder& Builder, FGLTFStaticMeshSectionConverter& MeshSectionConverter, const UStaticMesh* StaticMesh, const UStaticMeshComponent* StaticMeshComponent, FGLTFMaterialArray Materials, int32 LODIndex, FGLTFJsonMeshIndex MeshIndex)
		: FGLTFTask(ESKGLTFTaskPriority::Mesh)
		, Builder(Builder)
		, MeshSectionConverter(MeshSectionConverter)
		, StaticMesh(StaticMesh)
		, StaticMeshComponent(StaticMeshComponent)
		, Materials(Materials)
		, LODIndex(LODIndex)
		, MeshIndex(MeshIndex)
	{
	}

	virtual FString GetName() override
	{
		return StaticMeshComponent != nullptr ? FGLTFNameUtility::GetName(StaticMeshComponent) : StaticMesh->GetName();
	}

	virtual void Complete() override;

private:

	FGLTFConvertBuilder& Builder;
	FGLTFStaticMeshSectionConverter& MeshSectionConverter;
	const UStaticMesh* StaticMesh;
	const UStaticMeshComponent* StaticMeshComponent;
	const FGLTFMaterialArray Materials;
	const int32 LODIndex;
	const FGLTFJsonMeshIndex MeshIndex;
};

class FGLTFSkeletalMeshTask : public FGLTFTask
{
public:

	FGLTFSkeletalMeshTask(FGLTFConvertBuilder& Builder, FGLTFSkeletalMeshSectionConverter& MeshSectionConverter, const USkeletalMesh* SkeletalMesh, const USkeletalMeshComponent* SkeletalMeshComponent, FGLTFMaterialArray Materials, int32 LODIndex, FGLTFJsonMeshIndex MeshIndex)
		: FGLTFTask(ESKGLTFTaskPriority::Mesh)
		, Builder(Builder)
		, MeshSectionConverter(MeshSectionConverter)
		, SkeletalMesh(SkeletalMesh)
		, SkeletalMeshComponent(SkeletalMeshComponent)
		, Materials(Materials)
		, LODIndex(LODIndex)
		, MeshIndex(MeshIndex)
	{
	}

	virtual FString GetName() override
	{
		return SkeletalMeshComponent != nullptr ? FGLTFNameUtility::GetName(SkeletalMeshComponent) : SkeletalMesh->GetName();
	}

	virtual void Complete() override;

private:

	FGLTFConvertBuilder& Builder;
	FGLTFSkeletalMeshSectionConverter& MeshSectionConverter;
	const USkeletalMesh* SkeletalMesh;
	const USkeletalMeshComponent* SkeletalMeshComponent;
	const FGLTFMaterialArray Materials;
	const int32 LODIndex;
	const FGLTFJsonMeshIndex MeshIndex;
};
