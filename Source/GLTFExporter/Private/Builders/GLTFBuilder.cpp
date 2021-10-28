// Copyright Epic Games, Inc. All Rights Reserved.

#include "Builders/SKGLTFBuilder.h"
#include "UserData/SKGLTFMaterialUserData.h"
#include "Builders/SKGLTFFileUtility.h"

FGLTFBuilder::FGLTFBuilder(const FString& FilePath, const USKGLTFExportOptions* ExportOptions)
	: bIsGlbFile(FGLTFFileUtility::IsGlbFile(FilePath))
	, FilePath(FilePath)
	, DirPath(FPaths::GetPath(FilePath))
	, ExportOptions(ExportOptions)
{
}

FIntPoint FGLTFBuilder::GetBakeSizeForMaterialProperty(const UMaterialInterface* Material, const FMaterialPropertyEx& Property) const
{
	const ESKGLTFMaterialPropertyGroup PropertyGroup = GetPropertyGroup(Property);
	ESKGLTFMaterialBakeSizePOT DefaultValue = ExportOptions->DefaultMaterialBakeSize;

	if (const FSKGLTFOverrideMaterialBakeSettings* BakeSettings = ExportOptions->DefaultInputBakeSettings.Find(PropertyGroup))
	{
		if (BakeSettings->bOverrideSize)
		{
			DefaultValue = BakeSettings->Size;
		}
	}

	const ESKGLTFMaterialBakeSizePOT Size = USKGLTFMaterialExportOptions::GetBakeSizeForPropertyGroup(Material, PropertyGroup, DefaultValue);
	const int32 PixelSize = 1 << static_cast<uint8>(Size);
	return { PixelSize, PixelSize };
}

TextureFilter FGLTFBuilder::GetBakeFilterForMaterialProperty(const UMaterialInterface* Material, const FMaterialPropertyEx& Property) const
{
	const ESKGLTFMaterialPropertyGroup PropertyGroup = GetPropertyGroup(Property);
	TextureFilter DefaultValue = ExportOptions->DefaultMaterialBakeFilter;

	if (const FSKGLTFOverrideMaterialBakeSettings* BakeSettings = ExportOptions->DefaultInputBakeSettings.Find(PropertyGroup))
	{
		if (BakeSettings->bOverrideFilter)
		{
			DefaultValue = BakeSettings->Filter;
		}
	}

	return USKGLTFMaterialExportOptions::GetBakeFilterForPropertyGroup(Material, PropertyGroup, DefaultValue);
}

TextureAddress FGLTFBuilder::GetBakeTilingForMaterialProperty(const UMaterialInterface* Material, const FMaterialPropertyEx& Property) const
{
	const ESKGLTFMaterialPropertyGroup PropertyGroup = GetPropertyGroup(Property);
	TextureAddress DefaultValue = ExportOptions->DefaultMaterialBakeTiling;

	if (const FSKGLTFOverrideMaterialBakeSettings* BakeSettings = ExportOptions->DefaultInputBakeSettings.Find(PropertyGroup))
	{
		if (BakeSettings->bOverrideTiling)
		{
			DefaultValue = BakeSettings->Tiling;
		}
	}

	return USKGLTFMaterialExportOptions::GetBakeTilingForPropertyGroup(Material, PropertyGroup, DefaultValue);
}

ESKGLTFJsonHDREncoding FGLTFBuilder::GetTextureHDREncoding() const
{
	switch (ExportOptions->TextureHDREncoding)
	{
		case ESKGLTFTextureHDREncoding::None: return ESKGLTFJsonHDREncoding::None;
		case ESKGLTFTextureHDREncoding::RGBM: return ESKGLTFJsonHDREncoding::RGBM;
		// TODO: add more encodings (like RGBE) when viewer supports them
		default:
			checkNoEntry();
			return ESKGLTFJsonHDREncoding::None;
	}
}

bool FGLTFBuilder::ShouldExportLight(EComponentMobility::Type LightMobility) const
{
	const ESKGLTFSceneMobility AllowedMobility = static_cast<ESKGLTFSceneMobility>(ExportOptions->ExportLights);
	const ESKGLTFSceneMobility QueriedMobility = GetSceneMobility(LightMobility);
	return EnumHasAllFlags(AllowedMobility, QueriedMobility);
}

ESKGLTFSceneMobility FGLTFBuilder::GetSceneMobility(EComponentMobility::Type Mobility)
{
	switch (Mobility)
	{
		case EComponentMobility::Static:     return ESKGLTFSceneMobility::Static;
		case EComponentMobility::Stationary: return ESKGLTFSceneMobility::Stationary;
		case EComponentMobility::Movable:    return ESKGLTFSceneMobility::Movable;
		default:
			checkNoEntry();
			return ESKGLTFSceneMobility::None;
	}
}

ESKGLTFMaterialPropertyGroup FGLTFBuilder::GetPropertyGroup(const FMaterialPropertyEx& Property)
{
	switch (Property.Type)
	{
		case MP_BaseColor:
        case MP_Opacity:
        case MP_OpacityMask:
            return ESKGLTFMaterialPropertyGroup::BaseColorOpacity;
		case MP_Metallic:
        case MP_Roughness:
            return ESKGLTFMaterialPropertyGroup::MetallicRoughness;
		case MP_EmissiveColor:
			return ESKGLTFMaterialPropertyGroup::EmissiveColor;
		case MP_Normal:
			return ESKGLTFMaterialPropertyGroup::Normal;
		case MP_AmbientOcclusion:
			return ESKGLTFMaterialPropertyGroup::AmbientOcclusion;
		case MP_CustomData0:
        case MP_CustomData1:
            return ESKGLTFMaterialPropertyGroup::ClearCoatRoughness;
		case MP_CustomOutput:
			if (Property.CustomOutput == TEXT("ClearCoatBottomNormal"))
			{
				return ESKGLTFMaterialPropertyGroup::ClearCoatBottomNormal;
			}
		default:
			return ESKGLTFMaterialPropertyGroup::None;
	}
}
