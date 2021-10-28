// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SKGLTFExportOptions.h"
#include "Json/SKGLTFJsonEnums.h"
#include "MaterialPropertyEx.h"

class FGLTFBuilder
{
protected:

	FGLTFBuilder(const FString& FilePath, const USKGLTFExportOptions* ExportOptions);

public:

	const bool bIsGlbFile;
	const FString FilePath;
	const FString DirPath;

	// TODO: make ExportOptions private and expose each option via getters to ease overriding settings in future
	const USKGLTFExportOptions* const ExportOptions;

	FIntPoint GetBakeSizeForMaterialProperty(const UMaterialInterface* Material, const FMaterialPropertyEx& Property) const;
	TextureFilter GetBakeFilterForMaterialProperty(const UMaterialInterface* Material, const FMaterialPropertyEx& Property) const;
	TextureAddress GetBakeTilingForMaterialProperty(const UMaterialInterface* Material, const FMaterialPropertyEx& Property) const;

	ESKGLTFJsonHDREncoding GetTextureHDREncoding() const;

	bool ShouldExportLight(EComponentMobility::Type LightMobility) const;

private:

	static ESKGLTFSceneMobility GetSceneMobility(EComponentMobility::Type Mobility);
	static ESKGLTFMaterialPropertyGroup GetPropertyGroup(const FMaterialPropertyEx& Property);
};
