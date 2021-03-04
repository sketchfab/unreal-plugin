// Copyright Epic Games, Inc. All Rights Reserved.

#include "Builders/GLTFBuilder.h"
#include "UserData/GLTFMaterialUserData.h"
#include "Builders/GLTFFileUtility.h"

FGLTFBuilder::FGLTFBuilder(const FString& FilePath, const UGLTFExportOptions* ExportOptions)
	: bIsGlbFile(FGLTFFileUtility::IsGlbFile(FilePath))
	, FilePath(FilePath)
	, DirPath(FPaths::GetPath(FilePath))
	, ExportOptions(ExportOptions)
{
}

FIntPoint FGLTFBuilder::GetBakeSizeForMaterialProperty(const UMaterialInterface* Material, const FMaterialPropertyEx& Property) const
{
	const EGLTFMaterialPropertyGroup PropertyGroup = GetPropertyGroup(Property);
	EGLTFMaterialBakeSizePOT DefaultValue = ExportOptions->DefaultMaterialBakeSize;

	if (const FGLTFOverrideMaterialBakeSettings* BakeSettings = ExportOptions->DefaultInputBakeSettings.Find(PropertyGroup))
	{
		if (BakeSettings->bOverrideSize)
		{
			DefaultValue = BakeSettings->Size;
		}
	}

	const EGLTFMaterialBakeSizePOT Size = UGLTFMaterialExportOptions::GetBakeSizeForPropertyGroup(Material, PropertyGroup, DefaultValue);
	const int32 PixelSize = 1 << static_cast<uint8>(Size);
	return { PixelSize, PixelSize };
}

TextureFilter FGLTFBuilder::GetBakeFilterForMaterialProperty(const UMaterialInterface* Material, const FMaterialPropertyEx& Property) const
{
	const EGLTFMaterialPropertyGroup PropertyGroup = GetPropertyGroup(Property);
	TextureFilter DefaultValue = ExportOptions->DefaultMaterialBakeFilter;

	if (const FGLTFOverrideMaterialBakeSettings* BakeSettings = ExportOptions->DefaultInputBakeSettings.Find(PropertyGroup))
	{
		if (BakeSettings->bOverrideFilter)
		{
			DefaultValue = BakeSettings->Filter;
		}
	}

	return UGLTFMaterialExportOptions::GetBakeFilterForPropertyGroup(Material, PropertyGroup, DefaultValue);
}

TextureAddress FGLTFBuilder::GetBakeTilingForMaterialProperty(const UMaterialInterface* Material, const FMaterialPropertyEx& Property) const
{
	const EGLTFMaterialPropertyGroup PropertyGroup = GetPropertyGroup(Property);
	TextureAddress DefaultValue = ExportOptions->DefaultMaterialBakeTiling;

	if (const FGLTFOverrideMaterialBakeSettings* BakeSettings = ExportOptions->DefaultInputBakeSettings.Find(PropertyGroup))
	{
		if (BakeSettings->bOverrideTiling)
		{
			DefaultValue = BakeSettings->Tiling;
		}
	}

	return UGLTFMaterialExportOptions::GetBakeTilingForPropertyGroup(Material, PropertyGroup, DefaultValue);
}

EGLTFJsonHDREncoding FGLTFBuilder::GetTextureHDREncoding() const
{
	switch (ExportOptions->TextureHDREncoding)
	{
		case EGLTFTextureHDREncoding::None: return EGLTFJsonHDREncoding::None;
		case EGLTFTextureHDREncoding::RGBM: return EGLTFJsonHDREncoding::RGBM;
		// TODO: add more encodings (like RGBE) when viewer supports them
		default:
			checkNoEntry();
			return EGLTFJsonHDREncoding::None;
	}
}

bool FGLTFBuilder::ShouldExportLight(EComponentMobility::Type LightMobility) const
{
	const EGLTFSceneMobility AllowedMobility = static_cast<EGLTFSceneMobility>(ExportOptions->ExportLights);
	const EGLTFSceneMobility QueriedMobility = GetSceneMobility(LightMobility);
	return EnumHasAllFlags(AllowedMobility, QueriedMobility);
}

EGLTFSceneMobility FGLTFBuilder::GetSceneMobility(EComponentMobility::Type Mobility)
{
	switch (Mobility)
	{
		case EComponentMobility::Static:     return EGLTFSceneMobility::Static;
		case EComponentMobility::Stationary: return EGLTFSceneMobility::Stationary;
		case EComponentMobility::Movable:    return EGLTFSceneMobility::Movable;
		default:
			checkNoEntry();
			return EGLTFSceneMobility::None;
	}
}

EGLTFMaterialPropertyGroup FGLTFBuilder::GetPropertyGroup(const FMaterialPropertyEx& Property)
{
	switch (Property.Type)
	{
		case MP_BaseColor:
        case MP_Opacity:
        case MP_OpacityMask:
            return EGLTFMaterialPropertyGroup::BaseColorOpacity;
		case MP_Metallic:
        case MP_Roughness:
            return EGLTFMaterialPropertyGroup::MetallicRoughness;
		case MP_EmissiveColor:
			return EGLTFMaterialPropertyGroup::EmissiveColor;
		case MP_Normal:
			return EGLTFMaterialPropertyGroup::Normal;
		case MP_AmbientOcclusion:
			return EGLTFMaterialPropertyGroup::AmbientOcclusion;
		case MP_CustomData0:
        case MP_CustomData1:
            return EGLTFMaterialPropertyGroup::ClearCoatRoughness;
		case MP_CustomOutput:
			if (Property.CustomOutput == TEXT("ClearCoatBottomNormal"))
			{
				return EGLTFMaterialPropertyGroup::ClearCoatBottomNormal;
			}
		default:
			return EGLTFMaterialPropertyGroup::None;
	}
}
