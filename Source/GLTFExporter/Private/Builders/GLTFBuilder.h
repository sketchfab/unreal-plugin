// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GLTFExportOptions.h"
#include "Json/GLTFJsonEnums.h"
#include "MaterialPropertyEx.h"

class FGLTFBuilder
{
protected:

	FGLTFBuilder(const FString& FilePath, const UGLTFExportOptions* ExportOptions);

public:

	const bool bIsGlbFile;
	const FString FilePath;
	const FString DirPath;

	// TODO: make ExportOptions private and expose each option via getters to ease overriding settings in future
	const UGLTFExportOptions* const ExportOptions;

	FIntPoint GetBakeSizeForMaterialProperty(const UMaterialInterface* Material, const FMaterialPropertyEx& Property) const;
	TextureFilter GetBakeFilterForMaterialProperty(const UMaterialInterface* Material, const FMaterialPropertyEx& Property) const;
	TextureAddress GetBakeTilingForMaterialProperty(const UMaterialInterface* Material, const FMaterialPropertyEx& Property) const;

	EGLTFJsonHDREncoding GetTextureHDREncoding() const;

	bool ShouldExportLight(EComponentMobility::Type LightMobility) const;

private:

	static EGLTFSceneMobility GetSceneMobility(EComponentMobility::Type Mobility);
	static EGLTFMaterialPropertyGroup GetPropertyGroup(const FMaterialPropertyEx& Property);
};
