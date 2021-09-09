// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Converters/SKGLTFConverter.h"
#include "Converters/SKGLTFBuilderContext.h"
#include "Converters/SKGLTFMeshSectionConverters.h"
#include "Converters/SKGLTFMeshDataConverters.h"
#include "Converters/SKGLTFMaterialArray.h"
#include "Engine.h"

template <typename... InputTypes>
class TGLTFMeshConverter : public FGLTFBuilderContext, public TGLTFConverter<FGLTFJsonMeshIndex, InputTypes...>
{
	using FGLTFBuilderContext::FGLTFBuilderContext;
};

class FGLTFStaticMeshConverter final : public TGLTFMeshConverter<const UStaticMesh*, const UStaticMeshComponent*, FGLTFMaterialArray, int32>
{
	using TGLTFMeshConverter::TGLTFMeshConverter;

	virtual void Sanitize(const UStaticMesh*& StaticMesh, const UStaticMeshComponent*& StaticMeshComponent, FGLTFMaterialArray& Materials, int32& LODIndex) override;

	virtual FGLTFJsonMeshIndex Convert(const UStaticMesh* StaticMesh, const UStaticMeshComponent* StaticMeshComponent, FGLTFMaterialArray Materials, int32 LODIndex) override;

	FGLTFStaticMeshSectionConverter MeshSectionConverter;
};

class FGLTFSkeletalMeshConverter final : public TGLTFMeshConverter<const USkeletalMesh*, const USkeletalMeshComponent*, FGLTFMaterialArray, int32>
{
	using TGLTFMeshConverter::TGLTFMeshConverter;

	virtual void Sanitize(const USkeletalMesh*& SkeletalMesh, const USkeletalMeshComponent*& SkeletalMeshComponent, FGLTFMaterialArray& Materials, int32& LODIndex) override;

	virtual FGLTFJsonMeshIndex Convert(const USkeletalMesh* SkeletalMesh, const USkeletalMeshComponent* SkeletalMeshComponent, FGLTFMaterialArray Materials, int32 LODIndex) override;

	FGLTFSkeletalMeshSectionConverter MeshSectionConverter;
};
