// Copyright Epic Games, Inc. All Rights Reserved.

#include "UserData/GLTFMaterialUserData.h"
#include "Materials/MaterialInstance.h"
#include "Engine/Texture.h"

FGLTFOverrideMaterialBakeSettings::FGLTFOverrideMaterialBakeSettings()
	: bOverrideSize(false)
	, Size(EGLTFMaterialBakeSizePOT::POT_1024)
	, bOverrideFilter(false)
	, Filter(TF_Trilinear)
	, bOverrideTiling(false)
	, Tiling(TA_Wrap)
{
}

EGLTFMaterialBakeSizePOT UGLTFMaterialExportOptions::GetBakeSizeForPropertyGroup(const UMaterialInterface* Material, EGLTFMaterialPropertyGroup PropertyGroup, EGLTFMaterialBakeSizePOT DefaultValue)
{
	if (const FGLTFOverrideMaterialBakeSettings* BakeSettings = GetBakeSettingsByPredicate(Material, PropertyGroup, [](const FGLTFOverrideMaterialBakeSettings& Settings) { return Settings.bOverrideSize; }))
	{
		return BakeSettings->Size;
	}

	return DefaultValue;
}

TextureFilter UGLTFMaterialExportOptions::GetBakeFilterForPropertyGroup(const UMaterialInterface* Material, EGLTFMaterialPropertyGroup PropertyGroup, TextureFilter DefaultValue)
{
	if (const FGLTFOverrideMaterialBakeSettings* BakeSettings = GetBakeSettingsByPredicate(Material, PropertyGroup, [](const FGLTFOverrideMaterialBakeSettings& Settings) { return Settings.bOverrideFilter; }))
	{
		return BakeSettings->Filter;
	}

	return DefaultValue;
}

TextureAddress UGLTFMaterialExportOptions::GetBakeTilingForPropertyGroup(const UMaterialInterface* Material, EGLTFMaterialPropertyGroup PropertyGroup, TextureAddress DefaultValue)
{
	if (const FGLTFOverrideMaterialBakeSettings* BakeSettings = GetBakeSettingsByPredicate(Material, PropertyGroup, [](const FGLTFOverrideMaterialBakeSettings& Settings) { return Settings.bOverrideTiling; }))
	{
		return BakeSettings->Tiling;
	}

	return DefaultValue;
}

template <typename Predicate>
const FGLTFOverrideMaterialBakeSettings* UGLTFMaterialExportOptions::GetBakeSettingsByPredicate(const UMaterialInterface* Material, EGLTFMaterialPropertyGroup PropertyGroup, Predicate Pred)
{
	do
	{
		if (const UGLTFMaterialExportOptions* UserData = const_cast<UMaterialInterface*>(Material)->GetAssetUserData<UGLTFMaterialExportOptions>())
		{
			if (const FGLTFOverrideMaterialBakeSettings* BakeSettings = UserData->Inputs.Find(PropertyGroup))
			{
				if (Pred(*BakeSettings))
				{
					return BakeSettings;
				}
			}

			if (Pred(UserData->Default))
			{
				return &UserData->Default;
			}
		}

		const UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Material);
		Material = MaterialInstance != nullptr ? MaterialInstance->Parent : nullptr;
	} while (Material != nullptr);

	return nullptr;
}
