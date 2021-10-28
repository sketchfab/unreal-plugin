// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFMaterialUtility.h"
#include "Converters/SKGLTFTextureUtility.h"
#include "Converters/SKGLTFNameUtility.h"
#include "SKGLTFMaterialAnalyzer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Modules/ModuleManager.h"
#include "GLTFMaterialBaking/Public/IMaterialBakingModule.h"
#include "GLTFMaterialBaking/Public/MaterialBakingStructures.h"
#include "NormalMapPreview.h"
#include "Materials/MaterialExpressionCustomOutput.h"
#include "Materials/MaterialExpressionClearCoatNormalCustomOutput.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "MeshDescription.h"
#include "Misc/DefaultValueHelper.h"

UMaterialInterface* FGLTFMaterialUtility::GetDefault()
{
	static UMaterialInterface* DefaultMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Sketchfab/Materials/Default.Default"));
	return DefaultMaterial;
}

bool FGLTFMaterialUtility::IsNormalMap(const FMaterialPropertyEx& Property)
{
	return Property == MP_Normal || Property == TEXT("ClearCoatBottomNormal");
}

FVector4 FGLTFMaterialUtility::GetPropertyDefaultValue(const FMaterialPropertyEx& Property)
{
	// TODO: replace with GMaterialPropertyAttributesMap lookup (when public API available)

	switch (Property.Type)
	{
		case MP_EmissiveColor:          return FVector4(0,0,0,0);
		case MP_Opacity:                return FVector4(1,0,0,0);
		case MP_OpacityMask:            return FVector4(1,0,0,0);
		case MP_BaseColor:              return FVector4(0,0,0,0);
		case MP_Metallic:               return FVector4(0,0,0,0);
		case MP_Specular:               return FVector4(.5,0,0,0);
		case MP_Roughness:              return FVector4(.5,0,0,0);
		case MP_Anisotropy:             return FVector4(0,0,0,0);
		case MP_Normal:                 return FVector4(0,0,1,0);
		case MP_Tangent:                return FVector4(1,0,0,0);
		case MP_WorldPositionOffset:    return FVector4(0,0,0,0);
		case MP_WorldDisplacement:      return FVector4(0,0,0,0);
		case MP_TessellationMultiplier: return FVector4(1,0,0,0);
		case MP_SubsurfaceColor:        return FVector4(1,1,1,0);
		case MP_CustomData0:            return FVector4(1,0,0,0);
		case MP_CustomData1:            return FVector4(.1,0,0,0);
		case MP_AmbientOcclusion:       return FVector4(1,0,0,0);
		case MP_Refraction:             return FVector4(1,0,0,0);
		case MP_PixelDepthOffset:       return FVector4(0,0,0,0);
		case MP_ShadingModel:           return FVector4(0,0,0,0);
		default:                        break;
	}

	if (Property >= MP_CustomizedUVs0 && Property <= MP_CustomizedUVs7)
	{
		return FVector4(0,0,0,0);
	}

	if (Property == TEXT("ClearCoatBottomNormal"))
	{
		return FVector4(0,0,1,0);
	}

	check(false);
	return FVector4();
}

FVector4 FGLTFMaterialUtility::GetPropertyMask(const FMaterialPropertyEx& Property)
{
	// TODO: replace with GMaterialPropertyAttributesMap lookup (when public API available)

	switch (Property.Type)
	{
		case MP_EmissiveColor:          return FVector4(1,1,1,0);
		case MP_Opacity:                return FVector4(1,0,0,0);
		case MP_OpacityMask:            return FVector4(1,0,0,0);
		case MP_BaseColor:              return FVector4(1,1,1,0);
		case MP_Metallic:               return FVector4(1,0,0,0);
		case MP_Specular:               return FVector4(1,0,0,0);
		case MP_Roughness:              return FVector4(1,0,0,0);
		case MP_Anisotropy:             return FVector4(1,0,0,0);
		case MP_Normal:                 return FVector4(1,1,1,0);
		case MP_Tangent:                return FVector4(1,1,1,0);
		case MP_WorldPositionOffset:    return FVector4(1,1,1,0);
		case MP_WorldDisplacement:      return FVector4(1,1,1,0);
		case MP_TessellationMultiplier: return FVector4(1,0,0,0);
		case MP_SubsurfaceColor:        return FVector4(1,1,1,0);
		case MP_CustomData0:            return FVector4(1,0,0,0);
		case MP_CustomData1:            return FVector4(1,0,0,0);
		case MP_AmbientOcclusion:       return FVector4(1,0,0,0);
		case MP_Refraction:             return FVector4(1,1,0,0);
		case MP_PixelDepthOffset:       return FVector4(1,0,0,0);
		case MP_ShadingModel:           return FVector4(1,0,0,0);
		default:                        break;
	}

	if (Property >= MP_CustomizedUVs0 && Property <= MP_CustomizedUVs7)
	{
		return FVector4(1,1,0,0);
	}

	if (Property == TEXT("ClearCoatBottomNormal"))
	{
		return FVector4(1,1,1,0);
	}

	check(false);
	return FVector4();
}

const FExpressionInput* FGLTFMaterialUtility::GetInputForProperty(const UMaterialInterface* Material, const FMaterialPropertyEx& Property)
{
	if (Property.IsCustomOutput())
	{
		const UMaterialExpressionCustomOutput* CustomOutput = GetCustomOutputByName(Material, Property.CustomOutput.ToString());
		return CustomOutput != nullptr ? &CastChecked<UMaterialExpressionClearCoatNormalCustomOutput>(CustomOutput)->Input : nullptr;
	}

	UMaterial* UnderlyingMaterial = const_cast<UMaterial*>(Material->GetMaterial());
	return UnderlyingMaterial->GetExpressionInputForProperty(Property.Type);
}

const UMaterialExpressionCustomOutput* FGLTFMaterialUtility::GetCustomOutputByName(const UMaterialInterface* Material, const FString& Name)
{
	// TODO: should we also search inside material functions and attribute layers?

	for (const UMaterialExpression* Expression : Material->GetMaterial()->Expressions)
	{
		const UMaterialExpressionCustomOutput* CustomOutput = Cast<UMaterialExpressionCustomOutput>(Expression);
		if (CustomOutput != nullptr && CustomOutput->GetDisplayName() == Name)
		{
			return CustomOutput;
		}
	}

	return nullptr;
}

UTexture2D* FGLTFMaterialUtility::CreateTransientTexture(const FGLTFPropertyBakeOutput& PropertyBakeOutput, bool bUseSRGB)
{
	return FGLTFTextureUtility::CreateTransientTexture(
		PropertyBakeOutput.Pixels.GetData(),
		PropertyBakeOutput.Pixels.Num() * PropertyBakeOutput.Pixels.GetTypeSize(),
		PropertyBakeOutput.Size,
		PropertyBakeOutput.PixelFormat,
		bUseSRGB);
}

bool FGLTFMaterialUtility::CombineTextures(TArray<FColor>& OutPixels, const TArray<FGLTFTextureCombineSource>& Sources, const FIntPoint& OutputSize, const EPixelFormat OutputPixelFormat)
{
	// NOTE: both bForceLinearGamma and TargetGamma=2.2 seem necessary for exported images to match their source data.
	// It's not entirely clear why gamma must be 2.2 (instead of 0.0) and why bInForceLinearGamma must also be true.
	const bool bForceLinearGamma = true;
	const float TargetGamma = 2.2f;

	UTextureRenderTarget2D* RenderTarget2D = NewObject<UTextureRenderTarget2D>();

	RenderTarget2D->AddToRoot();
	RenderTarget2D->ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
	RenderTarget2D->InitCustomFormat(OutputSize.X, OutputSize.Y, OutputPixelFormat, bForceLinearGamma);
	RenderTarget2D->TargetGamma = TargetGamma;

	FRenderTarget* RenderTarget = RenderTarget2D->GameThread_GetRenderTargetResource();
	FCanvas Canvas(RenderTarget, nullptr, 0.0f, 0.0f, 0.0f, GMaxRHIFeatureLevel);

	Canvas.SetRenderTarget_GameThread(RenderTarget);
	Canvas.Clear({ 0.0f, 0.0f, 0.0f, 0.0f });

	const FVector2D TileSize(OutputSize.X, OutputSize.Y);
	const FVector2D TilePosition(0.0f, 0.0f);

	for (const FGLTFTextureCombineSource& Source: Sources)
	{
		check(Source.Texture);

		TRefCountPtr<FBatchedElementParameters> BatchedElementParameters;

		if (Source.Texture->IsNormalMap())
		{
			BatchedElementParameters = new FNormalMapBatchedElementParameters();
		}

		FCanvasTileItem TileItem(TilePosition, Source.Texture->Resource, TileSize, Source.TintColor);

		TileItem.BatchedElementParameters = BatchedElementParameters;
		TileItem.BlendMode = Source.BlendMode;
		TileItem.Draw(&Canvas);
	}

	Canvas.Flush_GameThread();
	FlushRenderingCommands();
	Canvas.SetRenderTarget_GameThread(nullptr);
	FlushRenderingCommands();

	const bool bReadSuccessful = RenderTarget->ReadPixels(OutPixels);

	// Clean up.
	RenderTarget2D->ReleaseResource();
	RenderTarget2D->RemoveFromRoot();
	RenderTarget2D = nullptr;

	return bReadSuccessful;
}

FGLTFPropertyBakeOutput FGLTFMaterialUtility::BakeMaterialProperty(const FIntPoint& OutputSize, const FMaterialPropertyEx& Property, const UMaterialInterface* Material, int32 TexCoord, const FMeshDescription* MeshDescription, const FGLTFIndexArray& MeshSectionIndices, bool bCopyAlphaFromRedChannel)
{
	FMeshData MeshSet;
	MeshSet.TextureCoordinateBox = { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
	MeshSet.TextureCoordinateIndex = TexCoord;
	MeshSet.RawMeshDescription = const_cast<FMeshDescription*>(MeshDescription);
	MeshSet.MaterialIndices = MeshSectionIndices; // NOTE: MaterialIndices is actually section indices

	FMaterialDataEx MatSet;
	MatSet.Material = const_cast<UMaterialInterface*>(Material);
	MatSet.PropertySizes.Add(Property, OutputSize);

	TArray<FMeshData*> MeshSettings;
	TArray<FMaterialDataEx*> MatSettings;
	MeshSettings.Add(&MeshSet);
	MatSettings.Add(&MatSet);

	TArray<FBakeOutputEx> BakeOutputs;
	ISKMaterialBakingModule& Module = FModuleManager::Get().LoadModuleChecked<ISKMaterialBakingModule>("SKGLTFMaterialBaking");

	Module.BakeMaterials(MatSettings, MeshSettings, BakeOutputs);

	FBakeOutputEx& BakeOutput = BakeOutputs[0];

	TArray<FColor> BakedPixels = MoveTemp(BakeOutput.PropertyData.FindChecked(Property));
	const FIntPoint BakedSize = BakeOutput.PropertySizes.FindChecked(Property);
	const float EmissiveScale = BakeOutput.EmissiveScale;

	if (bCopyAlphaFromRedChannel)
	{
		for (FColor& Pixel: BakedPixels)
		{
			Pixel.A = Pixel.R;
		}
	}
	else
	{
		// NOTE: alpha is 0 by default after baking a property, but we prefer 255 (1.0).
		// It makes it easier to view the exported textures.
		for (FColor& Pixel: BakedPixels)
		{
			Pixel.A = 255;
		}
	}

	if (IsNormalMap(Property))
	{
		// Convert normalmaps to use +Y (OpenGL / WebGL standard)
		FGLTFTextureUtility::FlipGreenChannel(BakedPixels);
	}

	FGLTFPropertyBakeOutput PropertyBakeOutput(Property, PF_B8G8R8A8, BakedPixels, BakedSize, EmissiveScale);

	if (BakedPixels.Num() == 1)
	{
		const FColor& Pixel = BakedPixels[0];

		PropertyBakeOutput.bIsConstant = true;

		// TODO: is the current conversion from sRGB => linear correct?
		// It seems to give correct results for some properties, but not all.
		PropertyBakeOutput.ConstantValue = Pixel;
	}

	return PropertyBakeOutput;
}

FGLTFJsonTextureIndex FGLTFMaterialUtility::AddCombinedTexture(FGLTFConvertBuilder& Builder, const TArray<FGLTFTextureCombineSource>& CombineSources, const FIntPoint& TextureSize, bool bIgnoreAlpha, const FString& TextureName, ESKGLTFJsonTextureFilter MinFilter, ESKGLTFJsonTextureFilter MagFilter, ESKGLTFJsonTextureWrap WrapS, ESKGLTFJsonTextureWrap WrapT)
{
	check(CombineSources.Num() > 0);

	TArray<FColor> Pixels;
	const EPixelFormat PixelFormat = CombineSources[0].Texture->GetPixelFormat(); // TODO: should we really assume pixel format like this?

	if (!CombineTextures(Pixels, CombineSources, TextureSize, PixelFormat))
	{
		return FGLTFJsonTextureIndex(INDEX_NONE);
	}

	return AddTexture(Builder, Pixels, TextureSize, bIgnoreAlpha, false, TextureName, MinFilter, MagFilter, WrapS, WrapT);
}

FGLTFJsonTextureIndex FGLTFMaterialUtility::AddTexture(FGLTFConvertBuilder& Builder, const TArray<FColor>& Pixels, const FIntPoint& TextureSize, bool bIgnoreAlpha, bool bIsNormalMap, const FString& TextureName, ESKGLTFJsonTextureFilter MinFilter, ESKGLTFJsonTextureFilter MagFilter, ESKGLTFJsonTextureWrap WrapS, ESKGLTFJsonTextureWrap WrapT)
{
	// TODO: maybe we should reuse existing samplers?
	FGLTFJsonSampler JsonSampler;
	JsonSampler.Name = TextureName;
	JsonSampler.MinFilter = MinFilter;
	JsonSampler.MagFilter = MagFilter;
	JsonSampler.WrapS = WrapS;
	JsonSampler.WrapT = WrapT;

	// TODO: reuse same texture index when image is the same
	FGLTFJsonTexture JsonTexture;
	JsonTexture.Name = TextureName;
	JsonTexture.Sampler = Builder.AddSampler(JsonSampler);
	JsonTexture.Source = Builder.AddImage(Pixels, TextureSize, bIgnoreAlpha, bIsNormalMap ? ESKGLTFTextureType::Normalmaps : ESKGLTFTextureType::None, TextureName);

	return Builder.AddTexture(JsonTexture);
}

void FGLTFMaterialUtility::TransformToLinear(TArray<FColor>& InOutPixels)
{
	for (FColor& Pixel: InOutPixels)
	{
		Pixel = FLinearColor(Pixel).ToFColor(false);
	}
}

FLinearColor FGLTFMaterialUtility::GetMask(const FExpressionInput& ExpressionInput)
{
	return FLinearColor(ExpressionInput.MaskR, ExpressionInput.MaskG, ExpressionInput.MaskB, ExpressionInput.MaskA);
}

uint32 FGLTFMaterialUtility::GetMaskComponentCount(const FExpressionInput& ExpressionInput)
{
	return ExpressionInput.MaskR + ExpressionInput.MaskG + ExpressionInput.MaskB + ExpressionInput.MaskA;
}

bool FGLTFMaterialUtility::TryGetTextureCoordinateIndex(const UMaterialExpressionTextureSample* TextureSampler, int32& TexCoord, FGLTFJsonTextureTransform& Transform)
{
	const UMaterialExpression* Expression = TextureSampler->Coordinates.Expression;
	if (Expression == nullptr)
	{
		TexCoord = TextureSampler->ConstCoordinate;
		Transform = {};
		return true;
	}

	if (const UMaterialExpressionTextureCoordinate* TextureCoordinate = Cast<UMaterialExpressionTextureCoordinate>(Expression))
	{
		TexCoord = TextureCoordinate->CoordinateIndex;
		Transform.Offset.X = TextureCoordinate->UnMirrorU ? TextureCoordinate->UTiling * 0.5 : 0.0;
		Transform.Offset.Y = TextureCoordinate->UnMirrorV ? TextureCoordinate->VTiling * 0.5 : 0.0;
		Transform.Scale.X = TextureCoordinate->UTiling * (TextureCoordinate->UnMirrorU ? 0.5 : 1.0);
		Transform.Scale.Y = TextureCoordinate->VTiling * (TextureCoordinate->UnMirrorV ? 0.5 : 1.0);
		Transform.Rotation = 0;
		return true;
	}

	// TODO: add support for advanced expression tree (ex UMaterialExpressionTextureCoordinate -> UMaterialExpressionMultiply -> UMaterialExpressionAdd)

	return false;
}

void FGLTFMaterialUtility::GetAllTextureCoordinateIndices(const UMaterialInterface* InMaterial, const FMaterialPropertyEx& InProperty, FGLTFIndexArray& OutTexCoords)
{
	FGLTFMaterialAnalysis Analysis;
	AnalyzeMaterialProperty(InMaterial, InProperty, Analysis);

	const TBitArray<>& TexCoords = Analysis.TextureCoordinates;
	for (int32 Index = 0; Index < TexCoords.Num(); Index++)
	{
		if (TexCoords[Index])
		{
			OutTexCoords.Add(Index);
		}
	}
}

FMaterialShadingModelField FGLTFMaterialUtility::EvaluateShadingModelExpression(const UMaterialInterface* Material)
{
	FGLTFMaterialAnalysis Analysis;
	AnalyzeMaterialProperty(Material, MP_ShadingModel, Analysis);

	int32 Value;
	if (FDefaultValueHelper::ParseInt(Analysis.ParameterCode, Value))
	{
		return static_cast<EMaterialShadingModel>(Value);
    }

	return Analysis.ShadingModels;
}

EMaterialShadingModel FGLTFMaterialUtility::GetRichestShadingModel(const FMaterialShadingModelField& ShadingModels)
{
	if (ShadingModels.HasShadingModel(MSM_ClearCoat))
	{
		return MSM_ClearCoat;
	}

	if (ShadingModels.HasShadingModel(MSM_DefaultLit))
	{
		return MSM_DefaultLit;
	}

	if (ShadingModels.HasShadingModel(MSM_Unlit))
	{
		return MSM_Unlit;
	}

	// TODO: add more shading models when conversion supported

	return ShadingModels.GetFirstShadingModel();
}

FString FGLTFMaterialUtility::ShadingModelsToString(const FMaterialShadingModelField& ShadingModels)
{
	FString Result;

	for (uint32 Index = 0; Index < MSM_NUM; Index++)
	{
		const EMaterialShadingModel ShadingModel = static_cast<EMaterialShadingModel>(Index);
		if (ShadingModels.HasShadingModel(ShadingModel))
		{
			FString Name = FGLTFNameUtility::GetName(ShadingModel);
			Result += Result.IsEmpty() ? Name : TEXT(", ") + Name;
		}
	}

	return Result;
}

bool FGLTFMaterialUtility::NeedsMeshData(const UMaterialInterface* Material)
{
	if (Material == nullptr)
	{
		return false;
	}

	// TODO: only analyze properties that will be needed for this specific material
	const TArray<FMaterialPropertyEx> Properties =
	{
		MP_BaseColor,
		MP_EmissiveColor,
		MP_Opacity,
		MP_OpacityMask,
		MP_Metallic,
		MP_Roughness,
		MP_Normal,
		MP_AmbientOcclusion,
		MP_CustomData0,
		MP_CustomData1,
		TEXT("ClearCoatBottomNormal"),
	};

	bool bRequiresVertexData = false;
	FGLTFMaterialAnalysis Analysis;

	for (const FMaterialPropertyEx& Property: Properties)
	{
		AnalyzeMaterialProperty(Material, Property, Analysis);
		bRequiresVertexData |= Analysis.bRequiresVertexData;
	}

	return bRequiresVertexData;
}

bool FGLTFMaterialUtility::NeedsMeshData(const TArray<const UMaterialInterface*>& Materials)
{
	for (const UMaterialInterface* Material: Materials)
	{
		if (NeedsMeshData(Material))
		{
			return true;
		}
	}

	return false;
}

void FGLTFMaterialUtility::AnalyzeMaterialProperty(const UMaterialInterface* InMaterial, const FMaterialPropertyEx& InProperty, FGLTFMaterialAnalysis& OutAnalysis)
{
	USKGLTFMaterialAnalyzer::AnalyzeMaterialPropertyEx(InMaterial, InProperty.Type, InProperty.CustomOutput.ToString(), OutAnalysis);
}

const UMaterialInterface* FGLTFMaterialUtility::GetInterface(const UMaterialInterface* Material)
{
	return Material;
}

const UMaterialInterface* FGLTFMaterialUtility::GetInterface(const FStaticMaterial& Material)
{
	return Material.MaterialInterface;
}

const UMaterialInterface* FGLTFMaterialUtility::GetInterface(const FSkeletalMaterial& Material)
{
	return Material.MaterialInterface;
}

void FGLTFMaterialUtility::ResolveOverrides(TArray<const UMaterialInterface*>& Overrides, const TArray<UMaterialInterface*>& Defaults)
{
	ResolveOverrides<UMaterialInterface*>(Overrides, Defaults);
}

void FGLTFMaterialUtility::ResolveOverrides(TArray<const UMaterialInterface*>& Overrides, const TArray<FStaticMaterial>& Defaults)
{
	ResolveOverrides<FStaticMaterial>(Overrides, Defaults);
}

void FGLTFMaterialUtility::ResolveOverrides(TArray<const UMaterialInterface*>& Overrides, const TArray<FSkeletalMaterial>& Defaults)
{
	ResolveOverrides<FSkeletalMaterial>(Overrides, Defaults);
}

template <typename MaterialType>
void FGLTFMaterialUtility::ResolveOverrides(TArray<const UMaterialInterface*>& Overrides, const TArray<MaterialType>& Defaults)
{
	const int32 Count = Defaults.Num();
	Overrides.SetNumZeroed(Count);

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const UMaterialInterface*& Element = Overrides[Index];
		if (Element == nullptr)
		{
			Element = GetInterface(Defaults[Index]);
			if (Element == nullptr)
			{
				Element = UMaterial::GetDefaultMaterial(MD_Surface);
			}
		}
	}
}
