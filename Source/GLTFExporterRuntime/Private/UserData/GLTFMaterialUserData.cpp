// Copyright Epic Games, Inc. All Rights Reserved.

#include "UserData/SKGLTFMaterialUserData.h"
#include "Materials/MaterialInstance.h"
#include "Engine/Texture.h"

FSKGLTFOverrideMaterialBakeSettings::FSKGLTFOverrideMaterialBakeSettings()
	: bOverrideSize(false)
	, Size(ESKGLTFMaterialBakeSizePOT::POT_1024)
	, bOverrideFilter(false)
	, Filter(TF_Trilinear)
	, bOverrideTiling(false)
	, Tiling(TA_Wrap)
{
}

ESKGLTFMaterialBakeSizePOT USKGLTFMaterialExportOptions::GetBakeSizeForPropertyGroup(const UMaterialInterface* Material, ESKGLTFMaterialPropertyGroup PropertyGroup, ESKGLTFMaterialBakeSizePOT DefaultValue)
{
	if (const FSKGLTFOverrideMaterialBakeSettings* BakeSettings = GetBakeSettingsByPredicate(Material, PropertyGroup, [](const FSKGLTFOverrideMaterialBakeSettings& Settings) { return Settings.bOverrideSize; }))
	{
		return BakeSettings->Size;
	}

	return DefaultValue;
}

TextureFilter USKGLTFMaterialExportOptions::GetBakeFilterForPropertyGroup(const UMaterialInterface* Material, ESKGLTFMaterialPropertyGroup PropertyGroup, TextureFilter DefaultValue)
{
	if (const FSKGLTFOverrideMaterialBakeSettings* BakeSettings = GetBakeSettingsByPredicate(Material, PropertyGroup, [](const FSKGLTFOverrideMaterialBakeSettings& Settings) { return Settings.bOverrideFilter; }))
	{
		return BakeSettings->Filter;
	}

	return DefaultValue;
}

TextureAddress USKGLTFMaterialExportOptions::GetBakeTilingForPropertyGroup(const UMaterialInterface* Material, ESKGLTFMaterialPropertyGroup PropertyGroup, TextureAddress DefaultValue)
{
	if (const FSKGLTFOverrideMaterialBakeSettings* BakeSettings = GetBakeSettingsByPredicate(Material, PropertyGroup, [](const FSKGLTFOverrideMaterialBakeSettings& Settings) { return Settings.bOverrideTiling; }))
	{
		return BakeSettings->Tiling;
	}

	return DefaultValue;
}

template <typename Predicate>
const FSKGLTFOverrideMaterialBakeSettings* USKGLTFMaterialExportOptions::GetBakeSettingsByPredicate(const UMaterialInterface* Material, ESKGLTFMaterialPropertyGroup PropertyGroup, Predicate Pred)
{
	do
	{
		if (const USKGLTFMaterialExportOptions* UserData = const_cast<UMaterialInterface*>(Material)->GetAssetUserData<USKGLTFMaterialExportOptions>())
		{
			if (const FSKGLTFOverrideMaterialBakeSettings* BakeSettings = UserData->Inputs.Find(PropertyGroup))
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
