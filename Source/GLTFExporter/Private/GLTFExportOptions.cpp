// Copyright Epic Games, Inc. All Rights Reserved.

#include "SKGLTFExportOptions.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Object.h"

USKGLTFExportOptions::USKGLTFExportOptions(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ResetToDefault();
}

void USKGLTFExportOptions::ResetToDefault()
{
	ExportUniformScale = 0.01;
	bExportPreviewMesh = true;
	bBundleWebViewer = true;
	bShowFilesWhenDone = true;
	bExportUnlitMaterials = true;
	bExportClearCoatMaterials = true;
	bExportExtraBlendModes = true;
	BakeMaterialInputs = ESKGLTFMaterialBakeMode::UseMeshData;
	DefaultMaterialBakeSize = ESKGLTFMaterialBakeSizePOT::POT_1024;
	DefaultMaterialBakeFilter = TF_Trilinear;
	DefaultMaterialBakeTiling = TA_Wrap;
	bExportVertexColors = false;
	bExportVertexSkinWeights = true;
	bExportMeshQuantization = true;
	bExportLevelSequences = true;
	bExportAnimationSequences = true;
	bRetargetBoneTransforms = true;
	bExportPlaybackSettings = true;
	TextureImageFormat = ESKGLTFTextureImageFormat::PNG;
	TextureImageQuality = 0;
	NoLossyImageFormatFor = static_cast<int32>(ESKGLTFTextureType::All);
	bExportTextureTransforms = true;
	bExportLightmaps = true;
	TextureHDREncoding = ESKGLTFTextureHDREncoding::RGBM;
	bExportHiddenInGame = false;
	ExportLights = static_cast<int32>(ESKGLTFSceneMobility::Stationary | ESKGLTFSceneMobility::Movable);
	bExportCameras = true;
	bExportCameraControls = true;
	bExportAnimationHotspots = true;
	bExportHDRIBackdrops = true;
	bExportSkySpheres = true;
	bExportVariantSets = true;
	ExportMaterialVariants = ESKGLTFMaterialVariantMode::UseMeshData;
	bExportMeshVariants = true;
	bExportVisibilityVariants = true;
}
