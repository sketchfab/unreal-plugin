// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tasks/SKGLTFMaterialTasks.h"
#include "Converters/SKGLTFConverterUtility.h"
#include "Converters/SKGLTFNameUtility.h"
#include "Converters/SKGLTFMaterialUtility.h"
#include "Builders/SKGLTFContainerBuilder.h"
#include "MaterialPropertyEx.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"

namespace
{
	// Component masks
	const FLinearColor RedMask(1.0f, 0.0f, 0.0f, 0.0f);
	const FLinearColor GreenMask(0.0f, 1.0f, 0.0f, 0.0f);
	const FLinearColor BlueMask(0.0f, 0.0f, 1.0f, 0.0f);
	const FLinearColor AlphaMask(0.0f, 0.0f, 0.0f, 1.0f);
	const FLinearColor RgbMask = RedMask + GreenMask + BlueMask;
	const FLinearColor RgbaMask = RgbMask + AlphaMask;

	// Property-specific component masks
	const FLinearColor BaseColorMask = RgbMask;
	const FLinearColor OpacityMask = AlphaMask;
	const FLinearColor MetallicMask = BlueMask;
	const FLinearColor RoughnessMask = GreenMask;
	const FLinearColor OcclusionMask = RedMask;
	const FLinearColor ClearCoatMask = RedMask;
	const FLinearColor ClearCoatRoughnessMask = GreenMask;

	// Ideal masks for texture-inputs (doesn't require baking)
	const TArray<FLinearColor> DefaultColorInputMasks = { RgbMask, RgbaMask };
	const TArray<FLinearColor> BaseColorInputMasks = { BaseColorMask };
	const TArray<FLinearColor> OpacityInputMasks = { OpacityMask };
	const TArray<FLinearColor> MetallicInputMasks = { MetallicMask };
	const TArray<FLinearColor> RoughnessInputMasks = { RoughnessMask };
	const TArray<FLinearColor> OcclusionInputMasks = { OcclusionMask };
	const TArray<FLinearColor> ClearCoatInputMasks = { ClearCoatMask };
	const TArray<FLinearColor> ClearCoatRoughnessInputMasks = { ClearCoatRoughnessMask };
}

void FGLTFMaterialTask::Complete()
{
	{
		const UMaterial* ParentMaterial = Material->GetMaterial();

		if (ParentMaterial->MaterialDomain != MD_Surface)
		{
			// TODO: report warning (non-surface materials not supported, will be treated as surface)
		}
	}

	FGLTFJsonMaterial& JsonMaterial = Builder.GetMaterial(MaterialIndex);
	JsonMaterial.Name = GetMaterialName();
	JsonMaterial.AlphaCutoff = Material->GetOpacityMaskClipValue();
	JsonMaterial.DoubleSided = Material->IsTwoSided();

	ConvertAlphaMode(JsonMaterial.AlphaMode, JsonMaterial.BlendMode);
	ConvertShadingModel(JsonMaterial.ShadingModel);

	if (JsonMaterial.ShadingModel != ESKGLTFJsonShadingModel::None)
	{
		const FMaterialPropertyEx BaseColorProperty = JsonMaterial.ShadingModel == ESKGLTFJsonShadingModel::Unlit ? MP_EmissiveColor : MP_BaseColor;
		const FMaterialPropertyEx OpacityProperty = JsonMaterial.AlphaMode == ESKGLTFJsonAlphaMode::Mask ? MP_OpacityMask : MP_Opacity;

		// TODO: check if a property is active before trying to get it (i.e. Material->IsPropertyActive)

		if (JsonMaterial.AlphaMode == ESKGLTFJsonAlphaMode::Opaque)
		{
			if (!TryGetConstantColor(JsonMaterial.PBRMetallicRoughness.BaseColorFactor, BaseColorProperty))
			{
				if (!TryGetSourceTexture(JsonMaterial.PBRMetallicRoughness.BaseColorTexture, BaseColorProperty, DefaultColorInputMasks))
				{
					if (!TryGetBakedMaterialProperty(JsonMaterial.PBRMetallicRoughness.BaseColorTexture, JsonMaterial.PBRMetallicRoughness.BaseColorFactor, BaseColorProperty, TEXT("BaseColor")))
					{
						Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export %s for material %s"), *BaseColorProperty.ToString(), *Material->GetName()));
					}
				}
			}

			JsonMaterial.PBRMetallicRoughness.BaseColorFactor.A = 1.0f; // make sure base color is opaque
		}
		else
		{
			if (!TryGetBaseColorAndOpacity(JsonMaterial.PBRMetallicRoughness, BaseColorProperty, OpacityProperty))
			{
				Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export %s and %s for material %s"), *BaseColorProperty.ToString(), *OpacityProperty.ToString(), *Material->GetName()));
			}
		}

		if (JsonMaterial.ShadingModel == ESKGLTFJsonShadingModel::Default || JsonMaterial.ShadingModel == ESKGLTFJsonShadingModel::ClearCoat)
		{
			const FMaterialPropertyEx MetallicProperty = MP_Metallic;
			const FMaterialPropertyEx RoughnessProperty = MP_Roughness;

			if (!TryGetMetallicAndRoughness(JsonMaterial.PBRMetallicRoughness, MetallicProperty, RoughnessProperty))
			{
				Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export %s and %s for material %s"), *MetallicProperty.ToString(), *RoughnessProperty.ToString(), *Material->GetName()));
			}

			const FMaterialPropertyEx EmissiveProperty = MP_EmissiveColor;
			if (!TryGetEmissive(JsonMaterial, EmissiveProperty))
			{
				Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export %s for material %s"), *EmissiveProperty.ToString(), *Material->GetName()));
			}

			const FMaterialPropertyEx NormalProperty = JsonMaterial.ShadingModel == ESKGLTFJsonShadingModel::ClearCoat ? FMaterialPropertyEx(TEXT("ClearCoatBottomNormal")) : FMaterialPropertyEx(MP_Normal);
			if (IsPropertyNonDefault(NormalProperty))
			{
				if (!TryGetSourceTexture(JsonMaterial.NormalTexture, NormalProperty, DefaultColorInputMasks))
				{
					if (!TryGetBakedMaterialProperty(JsonMaterial.NormalTexture, NormalProperty, TEXT("Normal")))
					{
						Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export %s for material %s"), *NormalProperty.ToString(), *Material->GetName()));
					}
				}
			}

			const FMaterialPropertyEx AmbientOcclusionProperty = MP_AmbientOcclusion;
			if (IsPropertyNonDefault(AmbientOcclusionProperty))
			{
				if (!TryGetSourceTexture(JsonMaterial.OcclusionTexture, AmbientOcclusionProperty, OcclusionInputMasks))
				{
					if (!TryGetBakedMaterialProperty(JsonMaterial.OcclusionTexture, AmbientOcclusionProperty, TEXT("Occlusion"), true))
					{
						Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export %s for material %s"), *AmbientOcclusionProperty.ToString(), *Material->GetName()));
					}
				}
			}

			if (JsonMaterial.ShadingModel == ESKGLTFJsonShadingModel::ClearCoat)
			{
				const FMaterialPropertyEx ClearCoatProperty = MP_CustomData0;
				const FMaterialPropertyEx ClearCoatRoughnessProperty = MP_CustomData1;

				if (!TryGetClearCoatRoughness(JsonMaterial.ClearCoat, ClearCoatProperty, ClearCoatRoughnessProperty))
				{
					Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export %s and %s for material %s"), *ClearCoatProperty.ToString(), *ClearCoatRoughnessProperty.ToString(), *Material->GetName()));
				}

				const FMaterialPropertyEx ClearCoatNormalProperty = MP_Normal;
				if (IsPropertyNonDefault(ClearCoatNormalProperty))
				{
					if (!TryGetSourceTexture(JsonMaterial.ClearCoat.ClearCoatNormalTexture, ClearCoatNormalProperty, DefaultColorInputMasks))
					{
						if (!TryGetBakedMaterialProperty(JsonMaterial.ClearCoat.ClearCoatNormalTexture, ClearCoatNormalProperty, TEXT("ClearCoatNormal")))
						{
							Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export %s for material %s"), *ClearCoatNormalProperty.ToString(), *Material->GetName()));
						}
					}
				}
			}
		}
	}

	// Warn if properties baked using mesh data are using overlapping UVs
	if (MeshData != nullptr && MeshDataBakedProperties.Num() > 0)
	{
		const FGLTFMeshData* ParentMeshData = MeshData->GetParent();

		const float UVOverlapThreshold = 1.0f / 100.0f;
		const float UVOverlap = UVOverlapChecker.GetOrAdd(&ParentMeshData->Description, SectionIndices, MeshData->TexCoord);

		if (UVOverlap > UVOverlapThreshold)
		{
			FString SectionString = TEXT("mesh section");
			SectionString += SectionIndices.Num() > 1 ? TEXT("s ") : TEXT(" ");
			SectionString += FString::JoinBy(SectionIndices, TEXT(", "), FString::FromInt);

			Builder.AddWarningMessage(FString::Printf(
				TEXT("Material %s is baked using mesh data from %s but the lightmap UV (channel %d) are overlapping by %.2f%% (in %s) and may produce incorrect results"),
				*Material->GetName(),
				*ParentMeshData->Name,
				MeshData->TexCoord,
				UVOverlap * 100,
				*SectionString));
		}
	}
}

void FGLTFMaterialTask::ConvertAlphaMode(ESKGLTFJsonAlphaMode& OutAlphaMode, ESKGLTFJsonBlendMode& OutBlendMode) const
{
	const EBlendMode BlendMode = Material->GetBlendMode();

	OutAlphaMode = FGLTFConverterUtility::ConvertAlphaMode(BlendMode);
	if (OutAlphaMode == ESKGLTFJsonAlphaMode::None)
	{
		OutAlphaMode = ESKGLTFJsonAlphaMode::Blend;
		OutBlendMode = ESKGLTFJsonBlendMode::None;

		Builder.AddWarningMessage(FString::Printf(
			TEXT("Unsupported blend mode (%s) in material %s, will export as %s"),
			*FGLTFNameUtility::GetName(BlendMode),
			*Material->GetName(),
			*FGLTFNameUtility::GetName(BLEND_Translucent)));
		return;
	}

	if (OutAlphaMode == ESKGLTFJsonAlphaMode::Blend)
	{
		OutBlendMode = FGLTFConverterUtility::ConvertBlendMode(BlendMode);
		if (OutBlendMode != ESKGLTFJsonBlendMode::None && !Builder.ExportOptions->bExportExtraBlendModes)
		{
			OutBlendMode = ESKGLTFJsonBlendMode::None;

			Builder.AddWarningMessage(FString::Printf(
				TEXT("Extra blend mode (%s) in material %s disabled by export options, will export as %s"),
				*FGLTFNameUtility::GetName(BlendMode),
				*Material->GetName(),
				*FGLTFNameUtility::GetName(BLEND_Translucent)));
		}
	}
}

void FGLTFMaterialTask::ConvertShadingModel(ESKGLTFJsonShadingModel& OutShadingModel) const
{
	EMaterialShadingModel ShadingModel;
	{
		const FMaterialShadingModelField ShadingModels = Material->GetShadingModels();
		if (Material->IsShadingModelFromMaterialExpression())
		{
			const FMaterialShadingModelField Evaluation = FGLTFMaterialUtility::EvaluateShadingModelExpression(Material);
			if (Evaluation.CountShadingModels() == 1)
			{
				ShadingModel = Evaluation.GetFirstShadingModel();
			}
			else if (Evaluation.CountShadingModels() > 1)
			{
				ShadingModel = FGLTFMaterialUtility::GetRichestShadingModel(Evaluation);
				Builder.AddWarningMessage(FString::Printf(
					TEXT("Evaluation of shading model expression in material %s is inconclusive (%s), will export as %s"),
					*Material->GetName(),
					*FGLTFMaterialUtility::ShadingModelsToString(Evaluation),
					*FGLTFNameUtility::GetName(ShadingModel)));
			}
			else
			{
				ShadingModel = FGLTFMaterialUtility::GetRichestShadingModel(ShadingModels);
				Builder.AddWarningMessage(FString::Printf(
					TEXT("Evaluation of shading model expression in material %s returned none, will export as %s"),
					*Material->GetName(),
					*FGLTFNameUtility::GetName(ShadingModel)));
			}
		}
		else
		{
			ShadingModel = ShadingModels.GetFirstShadingModel();
		}
	}

	OutShadingModel = FGLTFConverterUtility::ConvertShadingModel(ShadingModel);
	if (OutShadingModel == ESKGLTFJsonShadingModel::None)
	{
		OutShadingModel = ESKGLTFJsonShadingModel::Default;

		Builder.AddWarningMessage(FString::Printf(
			TEXT("Unsupported shading model (%s) in material %s, will export as %s"),
			*FGLTFNameUtility::GetName(ShadingModel),
			*Material->GetName(),
			*FGLTFNameUtility::GetName(MSM_DefaultLit)));
		return;
	}

	if (OutShadingModel == ESKGLTFJsonShadingModel::Unlit && !Builder.ExportOptions->bExportUnlitMaterials)
	{
		OutShadingModel = ESKGLTFJsonShadingModel::Default;

		Builder.AddWarningMessage(FString::Printf(
			TEXT("Shading model (%s) in material %s disabled by export options, will export as %s"),
			*FGLTFNameUtility::GetName(ShadingModel),
			*Material->GetName(),
			*FGLTFNameUtility::GetName(MSM_DefaultLit)));
		return;
	}

	if (OutShadingModel == ESKGLTFJsonShadingModel::ClearCoat && !Builder.ExportOptions->bExportClearCoatMaterials)
	{
		OutShadingModel = ESKGLTFJsonShadingModel::Default;

		Builder.AddWarningMessage(FString::Printf(
			TEXT("Shading model (%s) in material %s disabled by export options, will export as %s"),
			*FGLTFNameUtility::GetName(ShadingModel),
			*Material->GetName(),
			*FGLTFNameUtility::GetName(MSM_DefaultLit)));
	}
}

bool FGLTFMaterialTask::TryGetBaseColorAndOpacity(FGLTFJsonPBRMetallicRoughness& OutPBRParams, const FMaterialPropertyEx& BaseColorProperty, const FMaterialPropertyEx& OpacityProperty)
{
	const bool bIsBaseColorConstant = TryGetConstantColor(OutPBRParams.BaseColorFactor, BaseColorProperty);
	const bool bIsOpacityConstant = TryGetConstantScalar(OutPBRParams.BaseColorFactor.A, OpacityProperty);

	if (bIsBaseColorConstant && bIsOpacityConstant)
	{
		return true;
	}

	// NOTE: since we always bake the properties (for now) when atleast property is non-const, we need
	// to reset the constant factors to their defaults. Otherwise the baked value of a constant property
	// would be scaled with the factor, i.e a double scaling.
	OutPBRParams.BaseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };

	const UTexture2D* BaseColorTexture;
	const UTexture2D* OpacityTexture;
	int32 BaseColorTexCoord;
	int32 OpacityTexCoord;
	FGLTFJsonTextureTransform BaseColorTransform;
	FGLTFJsonTextureTransform OpacityTransform;

	const bool bHasBaseColorSourceTexture = TryGetSourceTexture(BaseColorTexture, BaseColorTexCoord, BaseColorTransform, BaseColorProperty, BaseColorInputMasks);
	const bool bHasOpacitySourceTexture = TryGetSourceTexture(OpacityTexture, OpacityTexCoord, OpacityTransform, OpacityProperty, OpacityInputMasks);

	// Detect the "happy path" where both inputs share the same texture and are correctly masked.
	if (bHasBaseColorSourceTexture &&
		bHasOpacitySourceTexture &&
		BaseColorTexture == OpacityTexture &&
		BaseColorTexCoord == OpacityTexCoord &&
		BaseColorTransform == OpacityTransform)
	{
		OutPBRParams.BaseColorTexture.Index = Builder.GetOrAddTexture(BaseColorTexture);
		OutPBRParams.BaseColorTexture.TexCoord = BaseColorTexCoord;
		OutPBRParams.BaseColorTexture.Transform = BaseColorTransform;
		return true;
	}

	if (Builder.ExportOptions->BakeMaterialInputs == ESKGLTFMaterialBakeMode::Disabled)
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("%s and %s for material %s needs to bake, but material baking is disabled by export options"),
			*BaseColorProperty.ToString(),
			*OpacityProperty.ToString(),
			*Material->GetName()));
		return false;
	}

	FIntPoint TextureSize = Builder.GetBakeSizeForMaterialProperty(Material, BaseColorProperty);

	const TextureAddress TextureAddress = Builder.GetBakeTilingForMaterialProperty(Material, BaseColorProperty);
	const ESKGLTFJsonTextureWrap TextureWrapS = FGLTFConverterUtility::ConvertWrap(TextureAddress);
	const ESKGLTFJsonTextureWrap TextureWrapT = FGLTFConverterUtility::ConvertWrap(TextureAddress);

	const TextureFilter TextureFilter = Builder.GetBakeFilterForMaterialProperty(Material, BaseColorProperty);
	const ESKGLTFJsonTextureFilter TextureMinFilter = FGLTFConverterUtility::ConvertMinFilter(TextureFilter);
	const ESKGLTFJsonTextureFilter TextureMagFilter = FGLTFConverterUtility::ConvertMagFilter(TextureFilter);

	const FGLTFPropertyBakeOutput BaseColorBakeOutput = BakeMaterialProperty(BaseColorProperty, BaseColorTexCoord, TextureSize);
	const FGLTFPropertyBakeOutput OpacityBakeOutput = BakeMaterialProperty(OpacityProperty, OpacityTexCoord, TextureSize, true);
	const float BaseColorScale = BaseColorProperty == MP_EmissiveColor ? BaseColorBakeOutput.EmissiveScale : 1;

	// Detect when both baked properties are constants, which means we can avoid exporting a texture
	if (BaseColorBakeOutput.bIsConstant && OpacityBakeOutput.bIsConstant)
	{
		FLinearColor BaseColorFactor(BaseColorBakeOutput.ConstantValue * BaseColorScale);
		BaseColorFactor.A = OpacityBakeOutput.ConstantValue.A;

		OutPBRParams.BaseColorFactor = FGLTFConverterUtility::ConvertColor(BaseColorFactor);
		return true;
	}

	if (Builder.ExportOptions->TextureImageFormat == ESKGLTFTextureImageFormat::None)
	{
		return true;
	}

	int32 TexCoord;
	if (BaseColorBakeOutput.bIsConstant)
	{
		TexCoord = OpacityTexCoord;
	}
	else if (OpacityBakeOutput.bIsConstant)
	{
		TexCoord = BaseColorTexCoord;
	}
	else if (BaseColorTexCoord == OpacityTexCoord)
	{
		TexCoord = BaseColorTexCoord;
	}
	else
	{
		// TODO: report error (texture coordinate conflict)
		return false;
	}

	TextureSize = BaseColorBakeOutput.Size.ComponentMax(OpacityBakeOutput.Size);
	BaseColorTexture = FGLTFMaterialUtility::CreateTransientTexture(BaseColorBakeOutput);
	OpacityTexture = FGLTFMaterialUtility::CreateTransientTexture(OpacityBakeOutput);

	const TArray<FGLTFTextureCombineSource> CombineSources =
	{
		{ OpacityTexture, OpacityMask, SE_BLEND_Opaque },
		{ BaseColorTexture, BaseColorMask }
	};

	const FGLTFJsonTextureIndex TextureIndex = FGLTFMaterialUtility::AddCombinedTexture(
		Builder,
		CombineSources,
		TextureSize,
		false,
		GetBakedTextureName(TEXT("BaseColor")),
		TextureMinFilter,
		TextureMagFilter,
		TextureWrapS,
		TextureWrapT);

	OutPBRParams.BaseColorTexture.TexCoord = TexCoord;
	OutPBRParams.BaseColorTexture.Index = TextureIndex;
	OutPBRParams.BaseColorFactor = { BaseColorScale, BaseColorScale, BaseColorScale };

	return true;
}

bool FGLTFMaterialTask::TryGetMetallicAndRoughness(FGLTFJsonPBRMetallicRoughness& OutPBRParams, const FMaterialPropertyEx& MetallicProperty, const FMaterialPropertyEx& RoughnessProperty)
{
	const bool bIsMetallicConstant = TryGetConstantScalar(OutPBRParams.MetallicFactor, MetallicProperty);
	const bool bIsRoughnessConstant = TryGetConstantScalar(OutPBRParams.RoughnessFactor, RoughnessProperty);

	if (bIsMetallicConstant && bIsRoughnessConstant)
	{
		return true;
	}

	// NOTE: since we always bake the properties (for now) when atleast one property is non-const, we need
	// to reset the constant factors to their defaults. Otherwise the baked value of a constant property
	// would be scaled with the factor, i.e a double scaling.
	OutPBRParams.MetallicFactor = 1.0f;
	OutPBRParams.RoughnessFactor = 1.0f;

	const UTexture2D* MetallicTexture;
	const UTexture2D* RoughnessTexture;
	int32 MetallicTexCoord;
	int32 RoughnessTexCoord;
	FGLTFJsonTextureTransform MetallicTransform;
	FGLTFJsonTextureTransform RoughnessTransform;

	const bool bHasMetallicSourceTexture = TryGetSourceTexture(MetallicTexture, MetallicTexCoord, MetallicTransform, MetallicProperty, MetallicInputMasks);
	const bool bHasRoughnessSourceTexture = TryGetSourceTexture(RoughnessTexture, RoughnessTexCoord, RoughnessTransform, RoughnessProperty, RoughnessInputMasks);

	// Detect the "happy path" where both inputs share the same texture and are correctly masked.
	if (bHasMetallicSourceTexture &&
		bHasRoughnessSourceTexture &&
		MetallicTexture == RoughnessTexture &&
		MetallicTexCoord == RoughnessTexCoord &&
		MetallicTransform == RoughnessTransform)
	{
		OutPBRParams.MetallicRoughnessTexture.Index = Builder.GetOrAddTexture(MetallicTexture);
		OutPBRParams.MetallicRoughnessTexture.TexCoord = MetallicTexCoord;
		OutPBRParams.MetallicRoughnessTexture.Transform = MetallicTransform;
		return true;
	}

	if (Builder.ExportOptions->BakeMaterialInputs == ESKGLTFMaterialBakeMode::Disabled)
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("%s and %s for material %s needs to bake, but material baking is disabled by export options"),
			*MetallicProperty.ToString(),
			*RoughnessProperty.ToString(),
			*Material->GetName()));
		return false;
	}

	// TODO: add support for calculating the ideal resolution to use for baking based on connected (texture) nodes
	FIntPoint TextureSize = Builder.GetBakeSizeForMaterialProperty(Material, MetallicProperty);

	const TextureAddress TextureAddress = Builder.GetBakeTilingForMaterialProperty(Material, MetallicProperty);
	const ESKGLTFJsonTextureWrap TextureWrapS = FGLTFConverterUtility::ConvertWrap(TextureAddress);
	const ESKGLTFJsonTextureWrap TextureWrapT = FGLTFConverterUtility::ConvertWrap(TextureAddress);

	const TextureFilter TextureFilter = Builder.GetBakeFilterForMaterialProperty(Material, MetallicProperty);
	const ESKGLTFJsonTextureFilter TextureMinFilter = FGLTFConverterUtility::ConvertMinFilter(TextureFilter);
	const ESKGLTFJsonTextureFilter TextureMagFilter = FGLTFConverterUtility::ConvertMagFilter(TextureFilter);

	FGLTFPropertyBakeOutput MetallicBakeOutput = BakeMaterialProperty(MetallicProperty, MetallicTexCoord, TextureSize);
	FGLTFPropertyBakeOutput RoughnessBakeOutput = BakeMaterialProperty(RoughnessProperty, RoughnessTexCoord, TextureSize);

	// Detect when both baked properties are constants, which means we can use factors and avoid exporting a texture
	if (MetallicBakeOutput.bIsConstant && RoughnessBakeOutput.bIsConstant)
	{
		OutPBRParams.MetallicFactor = MetallicBakeOutput.ConstantValue.R;
		OutPBRParams.RoughnessFactor = RoughnessBakeOutput.ConstantValue.R;
		return true;
	}

	if (Builder.ExportOptions->TextureImageFormat == ESKGLTFTextureImageFormat::None)
	{
		return true;
	}

	int32 TexCoord;
	if (MetallicBakeOutput.bIsConstant)
	{
		TexCoord = RoughnessTexCoord;
	}
	else if (MetallicBakeOutput.bIsConstant)
	{
		TexCoord = MetallicTexCoord;
	}
	else if (MetallicTexCoord == RoughnessTexCoord)
	{
		TexCoord = MetallicTexCoord;
	}
	else
	{
		// TODO: report error (texture coordinate conflict)
		return false;
	}

	FGLTFMaterialUtility::TransformToLinear(MetallicBakeOutput.Pixels);
	FGLTFMaterialUtility::TransformToLinear(RoughnessBakeOutput.Pixels);

	TextureSize = RoughnessBakeOutput.Size.ComponentMax(MetallicBakeOutput.Size);
	MetallicTexture = FGLTFMaterialUtility::CreateTransientTexture(MetallicBakeOutput);
	RoughnessTexture = FGLTFMaterialUtility::CreateTransientTexture(RoughnessBakeOutput);

	const TArray<FGLTFTextureCombineSource> CombineSources =
	{
		{ MetallicTexture, MetallicMask + AlphaMask, SE_BLEND_Opaque },
		{ RoughnessTexture, RoughnessMask }
	};

	const FGLTFJsonTextureIndex TextureIndex = FGLTFMaterialUtility::AddCombinedTexture(
		Builder,
		CombineSources,
		TextureSize,
		true, // NOTE: we can ignore alpha in everything but TryGetBaseColorAndOpacity
		GetBakedTextureName(TEXT("MetallicRoughness")),
		TextureMinFilter,
		TextureMagFilter,
		TextureWrapS,
		TextureWrapT);

	OutPBRParams.MetallicRoughnessTexture.TexCoord = TexCoord;
	OutPBRParams.MetallicRoughnessTexture.Index = TextureIndex;

	return true;
}

bool FGLTFMaterialTask::TryGetClearCoatRoughness(FGLTFJsonClearCoatExtension& OutExtParams, const FMaterialPropertyEx& IntensityProperty, const FMaterialPropertyEx& RoughnessProperty)
{
	const bool bIsIntensityConstant = TryGetConstantScalar(OutExtParams.ClearCoatFactor, IntensityProperty);
	const bool bIsRoughnessConstant = TryGetConstantScalar(OutExtParams.ClearCoatRoughnessFactor, RoughnessProperty);

	if (bIsIntensityConstant && bIsRoughnessConstant)
	{
		return true;
	}

	// NOTE: since we always bake the properties (for now) when atleast one property is non-const, we need
	// to reset the constant factors to their defaults. Otherwise the baked value of a constant property
	// would be scaled with the factor, i.e a double scaling.
	OutExtParams.ClearCoatFactor = 1.0f;
	OutExtParams.ClearCoatRoughnessFactor = 1.0f;

	const UTexture2D* IntensityTexture;
	const UTexture2D* RoughnessTexture;
	int32 IntensityTexCoord;
	int32 RoughnessTexCoord;
	FGLTFJsonTextureTransform IntensityTransform;
	FGLTFJsonTextureTransform RoughnessTransform;

	const bool bHasIntensitySourceTexture = TryGetSourceTexture(IntensityTexture, IntensityTexCoord, IntensityTransform, IntensityProperty, ClearCoatInputMasks);
	const bool bHasRoughnessSourceTexture = TryGetSourceTexture(RoughnessTexture, RoughnessTexCoord, RoughnessTransform, RoughnessProperty, ClearCoatRoughnessInputMasks);

	// Detect the "happy path" where both inputs share the same texture and are correctly masked.
	if (bHasIntensitySourceTexture &&
		bHasRoughnessSourceTexture &&
		IntensityTexture == RoughnessTexture &&
		IntensityTexCoord == RoughnessTexCoord &&
		IntensityTransform == RoughnessTransform)
	{
		const FGLTFJsonTextureIndex TextureIndex = Builder.GetOrAddTexture(IntensityTexture);
		OutExtParams.ClearCoatTexture.Index = TextureIndex;
		OutExtParams.ClearCoatTexture.TexCoord = IntensityTexCoord;
		OutExtParams.ClearCoatRoughnessTexture.Index = TextureIndex;
		OutExtParams.ClearCoatRoughnessTexture.TexCoord = IntensityTexCoord;
		OutExtParams.ClearCoatRoughnessTexture.Transform = IntensityTransform;
		return true;
	}

	if (Builder.ExportOptions->BakeMaterialInputs == ESKGLTFMaterialBakeMode::Disabled)
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("%s and %s for material %s needs to bake, but material baking is disabled by export options"),
			*IntensityProperty.ToString(),
			*RoughnessProperty.ToString(),
			*Material->GetName()));
		return false;
	}

	FIntPoint TextureSize = Builder.GetBakeSizeForMaterialProperty(Material, IntensityProperty);

	const TextureAddress TextureAddress = Builder.GetBakeTilingForMaterialProperty(Material, IntensityProperty);
	const ESKGLTFJsonTextureWrap TextureWrapS = FGLTFConverterUtility::ConvertWrap(TextureAddress);
	const ESKGLTFJsonTextureWrap TextureWrapT = FGLTFConverterUtility::ConvertWrap(TextureAddress);

	const TextureFilter TextureFilter = Builder.GetBakeFilterForMaterialProperty(Material, IntensityProperty);
	const ESKGLTFJsonTextureFilter TextureMinFilter = FGLTFConverterUtility::ConvertMinFilter(TextureFilter);
	const ESKGLTFJsonTextureFilter TextureMagFilter = FGLTFConverterUtility::ConvertMagFilter(TextureFilter);

	const FGLTFPropertyBakeOutput IntensityBakeOutput = BakeMaterialProperty(IntensityProperty, IntensityTexCoord, TextureSize);
	const FGLTFPropertyBakeOutput RoughnessBakeOutput = BakeMaterialProperty(RoughnessProperty, RoughnessTexCoord, TextureSize);

	// Detect when both baked properties are constants, which means we can use factors and avoid exporting a texture
	if (IntensityBakeOutput.bIsConstant && RoughnessBakeOutput.bIsConstant)
	{
		OutExtParams.ClearCoatFactor = IntensityBakeOutput.ConstantValue.R;
		OutExtParams.ClearCoatRoughnessFactor =  RoughnessBakeOutput.ConstantValue.R;
		return true;
	}

	if (Builder.ExportOptions->TextureImageFormat == ESKGLTFTextureImageFormat::None)
	{
		return true;
	}

	int32 TexCoord;
	if (IntensityBakeOutput.bIsConstant)
	{
		TexCoord = RoughnessTexCoord;
	}
	else if (IntensityBakeOutput.bIsConstant)
	{
		TexCoord = IntensityTexCoord;
	}
	else if (IntensityTexCoord == RoughnessTexCoord)
	{
		TexCoord = IntensityTexCoord;
	}
	else
	{
		// TODO: report error (texture coordinate conflict)
		return false;
	}

	TextureSize = RoughnessBakeOutput.Size.ComponentMax(IntensityBakeOutput.Size);
	IntensityTexture = FGLTFMaterialUtility::CreateTransientTexture(IntensityBakeOutput);
	RoughnessTexture = FGLTFMaterialUtility::CreateTransientTexture(RoughnessBakeOutput);

	const TArray<FGLTFTextureCombineSource> CombineSources =
	{
		{ IntensityTexture, ClearCoatMask + AlphaMask, SE_BLEND_Opaque },
		{ RoughnessTexture, ClearCoatRoughnessMask }
	};

	const FGLTFJsonTextureIndex TextureIndex = FGLTFMaterialUtility::AddCombinedTexture(
		Builder,
		CombineSources,
		TextureSize,
		true, // NOTE: we can ignore alpha in everything but TryGetBaseColorAndOpacity
		GetBakedTextureName(TEXT("ClearCoatRoughness")),
		TextureMinFilter,
		TextureMagFilter,
		TextureWrapS,
		TextureWrapT);

	OutExtParams.ClearCoatTexture.Index = TextureIndex;
	OutExtParams.ClearCoatTexture.TexCoord = TexCoord;
	OutExtParams.ClearCoatRoughnessTexture.Index = TextureIndex;
	OutExtParams.ClearCoatRoughnessTexture.TexCoord = TexCoord;

	return true;
}

bool FGLTFMaterialTask::TryGetEmissive(FGLTFJsonMaterial& JsonMaterial, const FMaterialPropertyEx& EmissiveProperty)
{
	// TODO: right now we allow EmissiveFactor to be > 1.0 to support very bright emission, although it's not valid according to the glTF standard.
	// We may want to change this behaviour and store factors above 1.0 using a custom extension instead.

	if (TryGetConstantColor(JsonMaterial.EmissiveFactor, MP_EmissiveColor))
	{
		return true;
	}

	if (TryGetSourceTexture(JsonMaterial.EmissiveTexture, EmissiveProperty, DefaultColorInputMasks))
	{
		JsonMaterial.EmissiveFactor = FGLTFJsonColor3::White;	// make sure texture is not multiplied with black
		return true;
	}

	if (Builder.ExportOptions->BakeMaterialInputs == ESKGLTFMaterialBakeMode::Disabled)
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("%s for material %s needs to bake, but material baking is disabled by export options"),
			*EmissiveProperty.ToString(),
			*Material->GetName()));
		return false;
	}

	const FGLTFPropertyBakeOutput PropertyBakeOutput = BakeMaterialProperty(EmissiveProperty, JsonMaterial.EmissiveTexture.TexCoord);
	const float EmissiveScale = PropertyBakeOutput.EmissiveScale;

	if (PropertyBakeOutput.bIsConstant)
	{
		const FLinearColor EmissiveColor = PropertyBakeOutput.ConstantValue;
		JsonMaterial.EmissiveFactor = FGLTFConverterUtility::ConvertColor(EmissiveColor * EmissiveScale);
	}
	else
	{
		if (Builder.ExportOptions->TextureImageFormat == ESKGLTFTextureImageFormat::None)
		{
			JsonMaterial.EmissiveTexture.Index = FGLTFJsonTextureIndex(INDEX_NONE);
			return true;
		}

		if (!StoreBakedPropertyTexture(JsonMaterial.EmissiveTexture, PropertyBakeOutput, TEXT("Emissive")))
		{
			return false;
		}

		JsonMaterial.EmissiveFactor = { EmissiveScale, EmissiveScale, EmissiveScale };
	}

	return true;
}

bool FGLTFMaterialTask::IsPropertyNonDefault(const FMaterialPropertyEx& Property) const
{
	const bool bUseMaterialAttributes = Material->GetMaterial()->bUseMaterialAttributes;
	if (bUseMaterialAttributes)
	{
		// TODO: check if attribute property connected, i.e. Material->GetMaterial()->MaterialAttributes.IsConnected(Property)
		return true;
	}

	const FExpressionInput* MaterialInput = FGLTFMaterialUtility::GetInputForProperty(Material, Property);
	if (MaterialInput == nullptr)
	{
		// TODO: report error
		return false;
	}

	const UMaterialExpression* Expression = MaterialInput->Expression;
	if (Expression == nullptr)
	{
		return false;
	}

	return true;
}

bool FGLTFMaterialTask::TryGetConstantColor(FGLTFJsonColor3& OutValue, const FMaterialPropertyEx& Property) const
{
	FLinearColor Value;
	if (TryGetConstantColor(Value, Property))
	{
		OutValue = FGLTFConverterUtility::ConvertColor(Value);
		return true;
	}

	return false;
}

bool FGLTFMaterialTask::TryGetConstantColor(FGLTFJsonColor4& OutValue, const FMaterialPropertyEx& Property) const
{
	FLinearColor Value;
	if (TryGetConstantColor(Value, Property))
	{
		OutValue = FGLTFConverterUtility::ConvertColor(Value);
		return true;
	}

	return false;
}

bool FGLTFMaterialTask::TryGetConstantColor(FLinearColor& OutValue, const FMaterialPropertyEx& Property) const
{
	const bool bUseMaterialAttributes = Material->GetMaterial()->bUseMaterialAttributes;
	if (bUseMaterialAttributes)
	{
		// TODO: check if attribute property connected, i.e. Material->GetMaterial()->MaterialAttributes.IsConnected(Property)
		return false;
	}

	const FMaterialInput<FColor>* MaterialInput = FGLTFMaterialUtility::GetInputForProperty<FColor>(Material, Property);
	if (MaterialInput == nullptr)
	{
		// TODO: report error
		return false;
	}

	if (MaterialInput->UseConstant)
	{
		OutValue = { MaterialInput->Constant };
		return true;
	}

	const UMaterialExpression* Expression = MaterialInput->Expression;
	if (Expression == nullptr)
	{
		OutValue = FLinearColor(FGLTFMaterialUtility::GetPropertyDefaultValue(Property));
		return true;
	}

	if (const UMaterialExpressionVectorParameter* VectorParameter = ExactCast<UMaterialExpressionVectorParameter>(Expression))
	{
		FLinearColor Value = VectorParameter->DefaultValue;

		const UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Material);
		if (MaterialInstance != nullptr)
		{
			const FHashedMaterialParameterInfo ParameterInfo(VectorParameter->GetParameterName());
			if (!MaterialInstance->GetVectorParameterValue(ParameterInfo, Value))
			{
				// TODO: how to handle this?
			}
		}

		const uint32 MaskComponentCount = FGLTFMaterialUtility::GetMaskComponentCount(*MaterialInput);

		if (MaskComponentCount > 0)
		{
			const FLinearColor Mask = FGLTFMaterialUtility::GetMask(*MaterialInput);

			Value *= Mask;

			if (MaskComponentCount == 1)
			{
				const float ComponentValue = Value.R + Value.G + Value.B + Value.A;
				Value = { ComponentValue, ComponentValue, ComponentValue, ComponentValue };
			}
		}

		OutValue = Value;
		return true;
	}

	if (const UMaterialExpressionScalarParameter* ScalarParameter = ExactCast<UMaterialExpressionScalarParameter>(Expression))
	{
		float Value = ScalarParameter->DefaultValue;

		const UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Material);
		if (MaterialInstance != nullptr)
		{
			const FHashedMaterialParameterInfo ParameterInfo(ScalarParameter->GetParameterName());
			if (!MaterialInstance->GetScalarParameterValue(ParameterInfo, Value))
			{
				// TODO: how to handle this?
			}
		}

		OutValue = { Value, Value, Value, Value };
		return true;
	}

	if (const UMaterialExpressionConstant4Vector* Constant4Vector = ExactCast<UMaterialExpressionConstant4Vector>(Expression))
	{
		OutValue = Constant4Vector->Constant;
		return true;
	}

	if (const UMaterialExpressionConstant3Vector* Constant3Vector = ExactCast<UMaterialExpressionConstant3Vector>(Expression))
	{
		OutValue = Constant3Vector->Constant;
		return true;
	}

	if (const UMaterialExpressionConstant2Vector* Constant2Vector = ExactCast<UMaterialExpressionConstant2Vector>(Expression))
	{
		OutValue = { Constant2Vector->R, Constant2Vector->G, 0, 0 };
		return true;
	}

	if (const UMaterialExpressionConstant* Constant = ExactCast<UMaterialExpressionConstant>(Expression))
	{
		OutValue = { Constant->R, Constant->R, Constant->R, Constant->R };
		return true;
	}

	return false;
}

bool FGLTFMaterialTask::TryGetConstantScalar(float& OutValue, const FMaterialPropertyEx& Property) const
{
	const bool bUseMaterialAttributes = Material->GetMaterial()->bUseMaterialAttributes;
	if (bUseMaterialAttributes)
	{
		// TODO: check if attribute property connected, i.e. Material->GetMaterial()->MaterialAttributes.IsConnected(Property)
		return false;
	}

	const FMaterialInput<float>* MaterialInput = FGLTFMaterialUtility::GetInputForProperty<float>(Material, Property);
	if (MaterialInput == nullptr)
	{
		// TODO: report error
		return false;
	}

	if (MaterialInput->UseConstant)
	{
		OutValue = MaterialInput->Constant;
		return true;
	}

	const UMaterialExpression* Expression = MaterialInput->Expression;
	if (Expression == nullptr)
	{
		OutValue = FGLTFMaterialUtility::GetPropertyDefaultValue(Property).X;
		return true;
	}

	if (const UMaterialExpressionVectorParameter* VectorParameter = ExactCast<UMaterialExpressionVectorParameter>(Expression))
	{
		FLinearColor Value = VectorParameter->DefaultValue;

		const UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Material);
		if (MaterialInstance != nullptr)
		{
			const FHashedMaterialParameterInfo ParameterInfo(VectorParameter->GetParameterName());
			if (!MaterialInstance->GetVectorParameterValue(ParameterInfo, Value))
			{
				// TODO: how to handle this?
			}
		}

		const uint32 MaskComponentCount = FGLTFMaterialUtility::GetMaskComponentCount(*MaterialInput);

		if (MaskComponentCount > 0)
		{
			const FLinearColor Mask = FGLTFMaterialUtility::GetMask(*MaterialInput);
			Value *= Mask;
		}

		// TODO: is this a correct assumption, that the max component should be used as value?
		OutValue = Value.GetMax();
		return true;
	}

	if (const UMaterialExpressionScalarParameter* ScalarParameter = ExactCast<UMaterialExpressionScalarParameter>(Expression))
	{
		float Value = ScalarParameter->DefaultValue;

		const UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Material);
		if (MaterialInstance != nullptr)
		{
			const FHashedMaterialParameterInfo ParameterInfo(ScalarParameter->GetParameterName());
			if (!MaterialInstance->GetScalarParameterValue(ParameterInfo, Value))
			{
				// TODO: how to handle this?
			}
		}

		OutValue = Value;
		return true;
	}

	if (const UMaterialExpressionConstant4Vector* Constant4Vector = ExactCast<UMaterialExpressionConstant4Vector>(Expression))
	{
		OutValue = Constant4Vector->Constant.R;
		return true;
	}

	if (const UMaterialExpressionConstant3Vector* Constant3Vector = ExactCast<UMaterialExpressionConstant3Vector>(Expression))
	{
		OutValue = Constant3Vector->Constant.R;
		return true;
	}

	if (const UMaterialExpressionConstant2Vector* Constant2Vector = ExactCast<UMaterialExpressionConstant2Vector>(Expression))
	{
		OutValue = Constant2Vector->R;
		return true;
	}

	if (const UMaterialExpressionConstant* Constant = ExactCast<UMaterialExpressionConstant>(Expression))
	{
		OutValue = Constant->R;
		return true;
	}

	return false;
}

bool FGLTFMaterialTask::TryGetSourceTexture(FGLTFJsonTextureInfo& OutTexInfo, const FMaterialPropertyEx& Property, const TArray<FLinearColor>& AllowedMasks) const
{
	const UTexture2D* Texture;
	int32 TexCoord;
	FGLTFJsonTextureTransform Transform;

	if (TryGetSourceTexture(Texture, TexCoord, Transform, Property, AllowedMasks))
	{
		OutTexInfo.Index = Builder.GetOrAddTexture(Texture);
		OutTexInfo.TexCoord = TexCoord;
		OutTexInfo.Transform = Transform;
		return true;
	}

	return false;
}

bool FGLTFMaterialTask::TryGetSourceTexture(const UTexture2D*& OutTexture, int32& OutTexCoord, FGLTFJsonTextureTransform& OutTransform, const FMaterialPropertyEx& Property, const TArray<FLinearColor>& AllowedMasks) const
{
	if (Builder.ExportOptions->TextureImageFormat == ESKGLTFTextureImageFormat::None)
	{
		return false;
	}

	const FExpressionInput* MaterialInput = FGLTFMaterialUtility::GetInputForProperty(Material, Property);
	if (MaterialInput == nullptr)
	{
		// TODO: report error
		return false;
	}

	const UMaterialExpression* Expression = MaterialInput->Expression;
	if (Expression == nullptr)
	{
		return false;
	}

	const FLinearColor InputMask = FGLTFMaterialUtility::GetMask(*MaterialInput);
	if (AllowedMasks.Num() > 0 && !AllowedMasks.Contains(InputMask))
	{
		return false;
	}

	// TODO: add support or warning for texture sampler settings that override texture asset addressing (i.e. wrap, clamp etc)?

	if (const UMaterialExpressionTextureSampleParameter2D* TextureParameter = ExactCast<UMaterialExpressionTextureSampleParameter2D>(Expression))
	{
		UTexture* ParameterValue = TextureParameter->Texture;

		const UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Material);
		if (MaterialInstance != nullptr)
		{
			const FHashedMaterialParameterInfo ParameterInfo(TextureParameter->GetParameterName());
			if (!MaterialInstance->GetTextureParameterValue(ParameterInfo, ParameterValue))
			{
				// TODO: how to handle this?
			}
		}

		OutTexture = Cast<UTexture2D>(ParameterValue);

		if (OutTexture == nullptr)
		{
			if (ParameterValue == nullptr)
			{
				// TODO: report error (no texture parameter assigned)
			}
			else
			{
				// TODO: report error (incorrect texture type)
			}
			return false;
		}

		if (!FGLTFMaterialUtility::TryGetTextureCoordinateIndex(TextureParameter, OutTexCoord, OutTransform))
		{
			// TODO: report error (failed to identify texture coordinate index)
			return false;
		}

		if (!Builder.ExportOptions->bExportTextureTransforms && OutTransform != FGLTFJsonTextureTransform())
		{
			Builder.AddWarningMessage(FString::Printf(
				TEXT("Texture coordinates [%d] in %s for material %s are transformed, but texture transform is disabled by export options"),
				OutTexCoord,
				*Property.ToString(),
				*Material->GetName()));
			OutTransform = {};
		}

		return true;
	}

	if (const UMaterialExpressionTextureSample* TextureSampler = ExactCast<UMaterialExpressionTextureSample>(Expression))
	{
		// TODO: add support for texture object input expression
		OutTexture = Cast<UTexture2D>(TextureSampler->Texture);

		if (OutTexture == nullptr)
		{
			if (TextureSampler->Texture == nullptr)
			{
				// TODO: report error (no texture sample assigned)
			}
			else
			{
				// TODO: report error (incorrect texture type)
			}
			return false;
		}

		if (!FGLTFMaterialUtility::TryGetTextureCoordinateIndex(TextureSampler, OutTexCoord, OutTransform))
		{
			// TODO: report error (failed to identify texture coordinate index)
			return false;
		}

		if (!Builder.ExportOptions->bExportTextureTransforms && OutTransform != FGLTFJsonTextureTransform())
		{
			Builder.AddWarningMessage(FString::Printf(
				TEXT("Texture coordinates [%d] in %s for material %s are transformed, but texture transform is disabled by export options"),
				OutTexCoord,
				*Property.ToString(),
				*Material->GetName()));
			OutTransform = {};
		}

		return true;
	}

	return false;
}

bool FGLTFMaterialTask::TryGetBakedMaterialProperty(FGLTFJsonTextureInfo& OutTexInfo, FGLTFJsonColor3& OutConstant, const FMaterialPropertyEx& Property, const FString& PropertyName, bool bTransformToLinear)
{
	if (Builder.ExportOptions->BakeMaterialInputs == ESKGLTFMaterialBakeMode::Disabled)
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("%s for material %s needs to bake, but material baking is disabled by export options"),
			*Property.ToString(),
			*Material->GetName()));
		return false;
	}

	FGLTFPropertyBakeOutput PropertyBakeOutput = BakeMaterialProperty(Property, OutTexInfo.TexCoord);

	if (PropertyBakeOutput.bIsConstant)
	{
		OutConstant = FGLTFConverterUtility::ConvertColor(PropertyBakeOutput.ConstantValue);
		return true;
	}

	if (Builder.ExportOptions->TextureImageFormat == ESKGLTFTextureImageFormat::None)
	{
		OutTexInfo.Index = FGLTFJsonTextureIndex(INDEX_NONE);
		return true;
	}

	if (bTransformToLinear)
	{
		FGLTFMaterialUtility::TransformToLinear(PropertyBakeOutput.Pixels);
	}

	if (StoreBakedPropertyTexture(OutTexInfo, PropertyBakeOutput, PropertyName))
	{
		OutConstant = FGLTFJsonColor3::White; // make sure property is not zero
		return true;
	}

	return false;
}

bool FGLTFMaterialTask::TryGetBakedMaterialProperty(FGLTFJsonTextureInfo& OutTexInfo, FGLTFJsonColor4& OutConstant, const FMaterialPropertyEx& Property, const FString& PropertyName, bool bTransformToLinear)
{
	if (Builder.ExportOptions->BakeMaterialInputs == ESKGLTFMaterialBakeMode::Disabled)
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("%s for material %s needs to bake, but material baking is disabled by export options"),
			*Property.ToString(),
			*Material->GetName()));
		return false;
	}

	FGLTFPropertyBakeOutput PropertyBakeOutput = BakeMaterialProperty(Property, OutTexInfo.TexCoord);

	if (PropertyBakeOutput.bIsConstant)
	{
		OutConstant = FGLTFConverterUtility::ConvertColor(PropertyBakeOutput.ConstantValue);
		return true;
	}

	if (Builder.ExportOptions->TextureImageFormat == ESKGLTFTextureImageFormat::None)
	{
		OutTexInfo.Index = FGLTFJsonTextureIndex(INDEX_NONE);
		return true;
	}

	if (bTransformToLinear)
	{
		FGLTFMaterialUtility::TransformToLinear(PropertyBakeOutput.Pixels);
	}

	if (StoreBakedPropertyTexture(OutTexInfo, PropertyBakeOutput, PropertyName))
	{
		OutConstant = FGLTFJsonColor4::White; // make sure property is not zero
		return true;
	}

	return false;
}

inline bool FGLTFMaterialTask::TryGetBakedMaterialProperty(FGLTFJsonTextureInfo& OutTexInfo, float& OutConstant, const FMaterialPropertyEx& Property, const FString& PropertyName, bool bTransformToLinear)
{
	if (Builder.ExportOptions->BakeMaterialInputs == ESKGLTFMaterialBakeMode::Disabled)
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("%s for material %s needs to bake, but material baking is disabled by export options"),
			*Property.ToString(),
			*Material->GetName()));
		return false;
	}

	FGLTFPropertyBakeOutput PropertyBakeOutput = BakeMaterialProperty(Property, OutTexInfo.TexCoord);

	if (PropertyBakeOutput.bIsConstant)
	{
		OutConstant = PropertyBakeOutput.ConstantValue.R;
		return true;
	}

	if (Builder.ExportOptions->TextureImageFormat == ESKGLTFTextureImageFormat::None)
	{
		OutTexInfo.Index = FGLTFJsonTextureIndex(INDEX_NONE);
		return true;
	}

	if (bTransformToLinear)
	{
		FGLTFMaterialUtility::TransformToLinear(PropertyBakeOutput.Pixels);
	}

	if (StoreBakedPropertyTexture(OutTexInfo, PropertyBakeOutput, PropertyName))
	{
		OutConstant = 1; // make sure property is not zero
		return true;
	}

	return false;
}

bool FGLTFMaterialTask::TryGetBakedMaterialProperty(FGLTFJsonTextureInfo& OutTexInfo, const FMaterialPropertyEx& Property, const FString& PropertyName, bool bTransformToLinear)
{
	if (Builder.ExportOptions->BakeMaterialInputs == ESKGLTFMaterialBakeMode::Disabled)
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("%s for material %s needs to bake, but material baking is disabled by export options"),
			*Property.ToString(),
			*Material->GetName()));
		return false;
	}

	FGLTFPropertyBakeOutput PropertyBakeOutput = BakeMaterialProperty(Property, OutTexInfo.TexCoord);

	if (!PropertyBakeOutput.bIsConstant)
	{
		if (Builder.ExportOptions->TextureImageFormat == ESKGLTFTextureImageFormat::None)
		{
			OutTexInfo.Index = FGLTFJsonTextureIndex(INDEX_NONE);
			return true;
		}

		if (bTransformToLinear)
		{
			FGLTFMaterialUtility::TransformToLinear(PropertyBakeOutput.Pixels);
		}

		return StoreBakedPropertyTexture(OutTexInfo, PropertyBakeOutput, PropertyName);
	}

	const FVector4 MaskedConstant = FVector4(PropertyBakeOutput.ConstantValue) * FGLTFMaterialUtility::GetPropertyMask(Property);
	if (MaskedConstant == FGLTFMaterialUtility::GetPropertyDefaultValue(Property))
	{
		// Constant value is the same as the property's default so we can set gltf to default.
		OutTexInfo.Index = FGLTFJsonTextureIndex(INDEX_NONE);
		return true;
	}

	if (FGLTFMaterialUtility::IsNormalMap(Property))
	{
		// TODO: In some cases baking normal can result in constant vector that differs slight from default (i.e 0,0,1).
		// Yet often, when looking at such a material, it should be exactly default. Needs further investigation.
		// Maybe because of incorrect sRGB conversion? For now, assume a constant normal is always default.
		OutTexInfo.Index = FGLTFJsonTextureIndex(INDEX_NONE);
		return true;
	}

	// TODO: let function fail and investigate why in some cases a constant baking result is returned for a property
	// that is non-constant. This happens (for example) when baking AmbientOcclusion for a translucent material,
	// even though the same material when set to opaque will properly bake AmbientOcclusion to a texture.
	// For now, create a 1x1 texture with the constant value.

	const FGLTFJsonTextureIndex TextureIndex = FGLTFMaterialUtility::AddTexture(
		Builder,
		PropertyBakeOutput.Pixels,
		PropertyBakeOutput.Size,
		true, // NOTE: we can ignore alpha in everything but TryGetBaseColorAndOpacity
		false, // Normal and ClearCoatBottomNormal are handled above
		GetBakedTextureName(PropertyName),
		ESKGLTFJsonTextureFilter::Nearest,
		ESKGLTFJsonTextureFilter::Nearest,
		ESKGLTFJsonTextureWrap::ClampToEdge,
		ESKGLTFJsonTextureWrap::ClampToEdge);

	OutTexInfo.Index = TextureIndex;
	return true;
}

FGLTFPropertyBakeOutput FGLTFMaterialTask::BakeMaterialProperty(const FMaterialPropertyEx& Property, int32& OutTexCoord, bool bCopyAlphaFromRedChannel)
{
	const FIntPoint TextureSize = Builder.GetBakeSizeForMaterialProperty(Material, Property);
	return BakeMaterialProperty(Property, OutTexCoord, TextureSize, bCopyAlphaFromRedChannel);
}

FGLTFPropertyBakeOutput FGLTFMaterialTask::BakeMaterialProperty(const FMaterialPropertyEx& Property, int32& OutTexCoord, const FIntPoint& TextureSize, bool bCopyAlphaFromRedChannel)
{
	if (MeshData == nullptr)
	{
		FGLTFIndexArray TexCoords;
		FGLTFMaterialUtility::GetAllTextureCoordinateIndices(Material, Property, TexCoords);

		if (TexCoords.Num() > 0)
		{
			OutTexCoord = TexCoords[0];

			if (TexCoords.Num() > 1)
			{
				Builder.AddWarningMessage(FString::Printf(
					TEXT("%s for material %s uses multiple texture coordinates (%s), baked texture will be sampled using only the first (%d)"),
					*Property.ToString(),
					*Material->GetName(),
					*FString::JoinBy(TexCoords, TEXT(", "), FString::FromInt),
					OutTexCoord));
			}
		}
		else
		{
			OutTexCoord = 0;
		}
	}
	else
	{
		OutTexCoord = MeshData->TexCoord;
		MeshDataBakedProperties.Add(Property);
	}

	// TODO: add support for calculating the ideal resolution to use for baking based on connected (texture) nodes

	const FGLTFPropertyBakeOutput PropertyBakeOutput = FGLTFMaterialUtility::BakeMaterialProperty(
		TextureSize,
		Property,
		Material,
		OutTexCoord,
		MeshData != nullptr ? &MeshData->Description : nullptr,
		SectionIndices,
		bCopyAlphaFromRedChannel);

	return PropertyBakeOutput;
}

bool FGLTFMaterialTask::StoreBakedPropertyTexture(FGLTFJsonTextureInfo& OutTexInfo, const FGLTFPropertyBakeOutput& PropertyBakeOutput, const FString& PropertyName) const
{
	const TextureAddress TextureAddress = Builder.GetBakeTilingForMaterialProperty(Material, PropertyBakeOutput.Property);
	const ESKGLTFJsonTextureWrap TextureWrapS = FGLTFConverterUtility::ConvertWrap(TextureAddress);
	const ESKGLTFJsonTextureWrap TextureWrapT = FGLTFConverterUtility::ConvertWrap(TextureAddress);

	const TextureFilter TextureFilter = Builder.GetBakeFilterForMaterialProperty(Material, PropertyBakeOutput.Property);
	const ESKGLTFJsonTextureFilter TextureMinFilter = FGLTFConverterUtility::ConvertMinFilter(TextureFilter);
	const ESKGLTFJsonTextureFilter TextureMagFilter = FGLTFConverterUtility::ConvertMagFilter(TextureFilter);

	const FGLTFJsonTextureIndex TextureIndex = FGLTFMaterialUtility::AddTexture(
		Builder,
		PropertyBakeOutput.Pixels,
		PropertyBakeOutput.Size,
		true, // NOTE: we can ignore alpha in everything but TryGetBaseColorAndOpacity
		FGLTFMaterialUtility::IsNormalMap(PropertyBakeOutput.Property),
		GetBakedTextureName(PropertyName),
		TextureMinFilter,
		TextureMagFilter,
		TextureWrapS,
		TextureWrapT);

	OutTexInfo.Index = TextureIndex;
	return true;
}

FString FGLTFMaterialTask::GetMaterialName() const
{
	FString MaterialName = Material->GetName();

	if (MeshData != nullptr)
	{
		MaterialName += TEXT("_") + MeshData->Name;
	}

	return MaterialName;
}

FString FGLTFMaterialTask::GetBakedTextureName(const FString& PropertyName) const
{
	return GetMaterialName() + TEXT("_") + PropertyName;
}
