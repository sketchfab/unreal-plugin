// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Builders/SKGLTFContainerBuilder.h"
#include "Engine/Texture2D.h"

struct FGLTFMaterialAnalysis;
struct FMaterialPropertyEx;

struct FGLTFTextureCombineSource
{
	constexpr FORCEINLINE FGLTFTextureCombineSource(const UTexture2D* Texture, FLinearColor TintColor = { 1.0f, 1.0f, 1.0f, 1.0f }, ESimpleElementBlendMode BlendMode = SE_BLEND_Additive)
		: Texture(Texture), TintColor(TintColor), BlendMode(BlendMode)
	{}

	const UTexture2D* Texture;
	FLinearColor TintColor;
	ESimpleElementBlendMode BlendMode;
};

struct FGLTFPropertyBakeOutput
{
	FORCEINLINE FGLTFPropertyBakeOutput(const FMaterialPropertyEx& Property, EPixelFormat PixelFormat, TArray<FColor>& Pixels, FIntPoint Size, float EmissiveScale)
		: Property(Property), PixelFormat(PixelFormat), Pixels(Pixels), Size(Size), EmissiveScale(EmissiveScale), bIsConstant(false)
	{}

	const FMaterialPropertyEx& Property;
	EPixelFormat PixelFormat;
	TArray<FColor> Pixels;
	FIntPoint Size;
	float EmissiveScale;

	bool bIsConstant;
	FLinearColor ConstantValue;
};

struct FGLTFMaterialUtility
{
	static UMaterialInterface* GetDefault();

	static bool IsNormalMap(const FMaterialPropertyEx& Property);

	static FVector4 GetPropertyDefaultValue(const FMaterialPropertyEx& Property);
	static FVector4 GetPropertyMask(const FMaterialPropertyEx& Property);

	static const FExpressionInput* GetInputForProperty(const UMaterialInterface* Material, const FMaterialPropertyEx& Property);

	template<class InputType>
	static const FMaterialInput<InputType>* GetInputForProperty(const UMaterialInterface* Material, const FMaterialPropertyEx& Property)
	{
		const FExpressionInput* ExpressionInput = GetInputForProperty(Material, Property);
		return static_cast<const FMaterialInput<InputType>*>(ExpressionInput);
	}

	static const UMaterialExpressionCustomOutput* GetCustomOutputByName(const UMaterialInterface* Material, const FString& Name);

	static UTexture2D* CreateTransientTexture(const FGLTFPropertyBakeOutput& PropertyBakeOutput, bool bUseSRGB = false);
	static bool CombineTextures(TArray<FColor>& OutPixels, const TArray<FGLTFTextureCombineSource>& Sources, const FIntPoint& OutputSize, EPixelFormat OutputPixelFormat);
	static FGLTFPropertyBakeOutput BakeMaterialProperty(const FIntPoint& OutputSize, const FMaterialPropertyEx& Property, const UMaterialInterface* Material, int32 TexCoord, const FMeshDescription* MeshDescription = nullptr, const FGLTFIndexArray& MeshSectionIndices = {}, bool bCopyAlphaFromRedChannel = false);

	static FGLTFJsonTextureIndex AddCombinedTexture(FGLTFConvertBuilder& Builder, const TArray<FGLTFTextureCombineSource>& CombineSources, const FIntPoint& TextureSize, bool bIgnoreAlpha, const FString& TextureName, ESKGLTFJsonTextureFilter MinFilter, ESKGLTFJsonTextureFilter MagFilter, ESKGLTFJsonTextureWrap WrapS, ESKGLTFJsonTextureWrap WrapT);
	static FGLTFJsonTextureIndex AddTexture(FGLTFConvertBuilder& Builder, const TArray<FColor>& Pixels, const FIntPoint& TextureSize, bool bIgnoreAlpha, bool bIsNormalMap, const FString& TextureName, ESKGLTFJsonTextureFilter MinFilter, ESKGLTFJsonTextureFilter MagFilter, ESKGLTFJsonTextureWrap WrapS, ESKGLTFJsonTextureWrap WrapT);

	static void TransformToLinear(TArray<FColor>& InOutPixels);

	static FLinearColor GetMask(const FExpressionInput& ExpressionInput);
	static uint32 GetMaskComponentCount(const FExpressionInput& ExpressionInput);

	static bool TryGetTextureCoordinateIndex(const UMaterialExpressionTextureSample* TextureSampler, int32& TexCoord, FGLTFJsonTextureTransform& Transform);
	static void GetAllTextureCoordinateIndices(const UMaterialInterface* InMaterial, const FMaterialPropertyEx& InProperty, FGLTFIndexArray& OutTexCoords);

	static FMaterialShadingModelField EvaluateShadingModelExpression(const UMaterialInterface* Material);
	static EMaterialShadingModel GetRichestShadingModel(const FMaterialShadingModelField& ShadingModels);
	static FString ShadingModelsToString(const FMaterialShadingModelField& ShadingModels);

	static bool NeedsMeshData(const UMaterialInterface* Material);
	static bool NeedsMeshData(const TArray<const UMaterialInterface*>& Materials);

	static void AnalyzeMaterialProperty(const UMaterialInterface* Material, const FMaterialPropertyEx& InProperty, FGLTFMaterialAnalysis& OutAnalysis);

	static const UMaterialInterface* GetInterface(const UMaterialInterface* Material);
	static const UMaterialInterface* GetInterface(const FStaticMaterial& Material);
	static const UMaterialInterface* GetInterface(const FSkeletalMaterial& Material);

	static void ResolveOverrides(TArray<const UMaterialInterface*>& Overrides, const TArray<UMaterialInterface*>& Defaults);
	static void ResolveOverrides(TArray<const UMaterialInterface*>& Overrides, const TArray<FStaticMaterial>& Defaults);
	static void ResolveOverrides(TArray<const UMaterialInterface*>& Overrides, const TArray<FSkeletalMaterial>& Defaults);

private:

	template <typename MaterialType>
	static void ResolveOverrides(TArray<const UMaterialInterface*>& Overrides, const TArray<MaterialType>& Defaults);
};
