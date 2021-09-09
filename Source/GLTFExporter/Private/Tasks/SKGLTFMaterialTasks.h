// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Tasks/SKGLTFTask.h"
#include "Builders/SKGLTFConvertBuilder.h"
#include "Converters/SKGLTFUVOverlapChecker.h"
#include "Converters/SKGLTFMeshData.h"
#include "MaterialPropertyEx.h"
#include "Engine.h"

struct FGLTFPropertyBakeOutput;

class FGLTFMaterialTask : public FGLTFTask
{
public:

	FGLTFMaterialTask(FGLTFConvertBuilder& Builder, FGLTFUVOverlapChecker& UVOverlapChecker, const UMaterialInterface* Material, const FGLTFMeshData* MeshData, FGLTFIndexArray SectionIndices, FGLTFJsonMaterialIndex MaterialIndex)
		: FGLTFTask(ESKGLTFTaskPriority::Material)
		, Builder(Builder)
		, UVOverlapChecker(UVOverlapChecker)
		, Material(Material)
		, MeshData(MeshData)
		, SectionIndices(SectionIndices)
		, MaterialIndex(MaterialIndex)
	{
	}

	virtual FString GetName() override
	{
		return Material->GetName();
	}

	virtual void Complete() override;

private:

	FGLTFConvertBuilder& Builder;
	FGLTFUVOverlapChecker& UVOverlapChecker;
	const UMaterialInterface* Material;
	const FGLTFMeshData* MeshData;
	const FGLTFIndexArray SectionIndices;
	const FGLTFJsonMaterialIndex MaterialIndex;

	TSet<FMaterialPropertyEx> MeshDataBakedProperties;

	void ConvertAlphaMode(ESKGLTFJsonAlphaMode& OutAlphaMode, ESKGLTFJsonBlendMode& OutBlendMode) const;
	void ConvertShadingModel(ESKGLTFJsonShadingModel& OutShadingModel) const;

	bool TryGetBaseColorAndOpacity(FGLTFJsonPBRMetallicRoughness& OutPBRParams, const FMaterialPropertyEx& BaseColorProperty, const FMaterialPropertyEx& OpacityProperty);
	bool TryGetMetallicAndRoughness(FGLTFJsonPBRMetallicRoughness& OutPBRParams, const FMaterialPropertyEx& MetallicProperty, const FMaterialPropertyEx& RoughnessProperty);
	bool TryGetClearCoatRoughness(FGLTFJsonClearCoatExtension& OutExtParams, const FMaterialPropertyEx& IntensityProperty, const FMaterialPropertyEx& RoughnessProperty);
	bool TryGetEmissive(FGLTFJsonMaterial& JsonMaterial, const FMaterialPropertyEx& EmissiveProperty);

	bool IsPropertyNonDefault(const FMaterialPropertyEx& Property) const;
	bool TryGetConstantColor(FGLTFJsonColor3& OutValue, const FMaterialPropertyEx& Property) const;
	bool TryGetConstantColor(FGLTFJsonColor4& OutValue, const FMaterialPropertyEx& Property) const;
	bool TryGetConstantColor(FLinearColor& OutValue, const FMaterialPropertyEx& Property) const;
	bool TryGetConstantScalar(float& OutValue, const FMaterialPropertyEx& Property) const;

	bool TryGetSourceTexture(FGLTFJsonTextureInfo& OutTexInfo, const FMaterialPropertyEx& Property, const TArray<FLinearColor>& AllowedMasks = {}) const;
	bool TryGetSourceTexture(const UTexture2D*& OutTexture, int32& OutTexCoord, FGLTFJsonTextureTransform& OutTransform, const FMaterialPropertyEx& Property, const TArray<FLinearColor>& AllowedMasks = {}) const;

	bool TryGetBakedMaterialProperty(FGLTFJsonTextureInfo& OutTexInfo, FGLTFJsonColor3& OutConstant, const FMaterialPropertyEx& Property, const FString& PropertyName, bool bTransformToLinear = false);
	bool TryGetBakedMaterialProperty(FGLTFJsonTextureInfo& OutTexInfo, FGLTFJsonColor4& OutConstant, const FMaterialPropertyEx& Property, const FString& PropertyName, bool bTransformToLinear = false);
	bool TryGetBakedMaterialProperty(FGLTFJsonTextureInfo& OutTexInfo, float& OutConstant, const FMaterialPropertyEx& Property, const FString& PropertyName, bool bTransformToLinear = false);
	bool TryGetBakedMaterialProperty(FGLTFJsonTextureInfo& OutTexInfo, const FMaterialPropertyEx& Property, const FString& PropertyName, bool bTransformToLinear = false);

	FGLTFPropertyBakeOutput BakeMaterialProperty(const FMaterialPropertyEx& Property, int32& OutTexCoord, bool bCopyAlphaFromRedChannel = false);
	FGLTFPropertyBakeOutput BakeMaterialProperty(const FMaterialPropertyEx& Property, int32& OutTexCoord, const FIntPoint& TextureSize, bool bCopyAlphaFromRedChannel = false);

	bool StoreBakedPropertyTexture(FGLTFJsonTextureInfo& OutTexInfo, const FGLTFPropertyBakeOutput& PropertyBakeOutput, const FString& PropertyName) const;

	FString GetMaterialName() const;
	FString GetBakedTextureName(const FString& PropertyName) const;
};
