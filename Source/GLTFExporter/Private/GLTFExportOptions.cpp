// Copyright Epic Games, Inc. All Rights Reserved.

#include "GLTFExportOptions.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Object.h"

UGLTFExportOptions::UGLTFExportOptions(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ResetToDefault();
}

void UGLTFExportOptions::ResetToDefault()
{
	ExportUniformScale = 0.01;
	bExportPreviewMesh = true;
	bBundleWebViewer = true;
	bShowFilesWhenDone = true;
	bExportUnlitMaterials = true;
	bExportClearCoatMaterials = true;
	bExportExtraBlendModes = true;
	BakeMaterialInputs = EGLTFMaterialBakeMode::UseMeshData;
	DefaultMaterialBakeSize = EGLTFMaterialBakeSizePOT::POT_1024;
	DefaultMaterialBakeFilter = TF_Trilinear;
	DefaultMaterialBakeTiling = TA_Wrap;
	bExportVertexColors = false;
	bExportVertexSkinWeights = true;
	bExportMeshQuantization = true;
	bExportLevelSequences = true;
	bExportAnimationSequences = true;
	bRetargetBoneTransforms = true;
	bExportPlaybackSettings = true;
	TextureImageFormat = EGLTFTextureImageFormat::PNG;
	TextureImageQuality = 0;
	NoLossyImageFormatFor = static_cast<int32>(EGLTFTextureType::All);
	bExportTextureTransforms = true;
	bExportLightmaps = true;
	TextureHDREncoding = EGLTFTextureHDREncoding::RGBM;
	bExportHiddenInGame = false;
	ExportLights = static_cast<int32>(EGLTFSceneMobility::Stationary | EGLTFSceneMobility::Movable);
	bExportCameras = true;
	bExportCameraControls = true;
	bExportAnimationHotspots = true;
	bExportHDRIBackdrops = true;
	bExportSkySpheres = true;
	bExportVariantSets = true;
	ExportMaterialVariants = EGLTFMaterialVariantMode::UseMeshData;
	bExportMeshVariants = true;
	bExportVisibilityVariants = true;
}
