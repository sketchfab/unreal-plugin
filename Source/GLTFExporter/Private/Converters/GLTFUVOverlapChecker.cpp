// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFUVOverlapChecker.h"
#include "StaticMeshAttributes.h"
#include "MeshDescription.h"
#include "Modules/ModuleManager.h"
#include "IMaterialBakingModule.h"
#include "MaterialBakingStructures.h"

void FGLTFUVOverlapChecker::Sanitize(const FMeshDescription*& Description, FGLTFIndexArray& SectionIndices, int32& TexCoord)
{
	if (Description != nullptr)
	{
		const TVertexInstanceAttributesConstRef<FVector2D> VertexInstanceUVs =
			Description->VertexInstanceAttributes().GetAttributesRef<FVector2D>(MeshAttribute::VertexInstance::TextureCoordinate);
#if ENGINE_MAJOR_VERSION == 5
		const int32 TexCoordCount = VertexInstanceUVs.GetNumChannels();
#else
		const int32 TexCoordCount = VertexInstanceUVs.GetNumIndices();
#endif

		if (TexCoord < 0 || TexCoord >= TexCoordCount)
		{
			Description = nullptr;
		}

		const int32 MinSectionIndex = FMath::Min(SectionIndices);
		const int32 MaxSectionIndex = FMath::Max(SectionIndices);
		const int32 SectionCount = Description->PolygonGroups().GetArraySize();

		if (MinSectionIndex < 0 || MaxSectionIndex >= SectionCount)
		{
			Description = nullptr;
		}
	}
}

float FGLTFUVOverlapChecker::Convert(const FMeshDescription* Description, FGLTFIndexArray SectionIndices, int32 TexCoord)
{
	if (Description == nullptr)
	{
		// TODO: report warning?
		return -1;
	}

	// TODO: investigate if the fixed size is high enough to properly calculate overlap
	const FIntPoint TextureSize(512, 512);
	const FMaterialPropertyEx Property = MP_EmissiveColor;

	FMeshData MeshSet;
	MeshSet.TextureCoordinateBox = { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
	MeshSet.TextureCoordinateIndex = TexCoord;
	MeshSet.RawMeshDescription = const_cast<FMeshDescription*>(Description);
	MeshSet.MaterialIndices = SectionIndices; // NOTE: MaterialIndices is actually section indices

	FMaterialDataEx MatSet;
	MatSet.Material = GetMaterial();
	MatSet.PropertySizes.Add(Property, TextureSize);
	MatSet.BlendMode = MatSet.Material->GetBlendMode();
	MatSet.bPerformBorderSmear = false;

	TArray<FMeshData*> MeshSettings;
	TArray<FMaterialDataEx*> MatSettings;
	MeshSettings.Add(&MeshSet);
	MatSettings.Add(&MatSet);

	TArray<FBakeOutputEx> BakeOutputs;
	ISKMaterialBakingModule& Module = FModuleManager::Get().LoadModuleChecked<ISKMaterialBakingModule>("SKGLTFMaterialBaking");

	Module.BakeMaterials(MatSettings, MeshSettings, BakeOutputs);

	const FBakeOutputEx& BakeOutput = BakeOutputs[0];
	const TArray<FColor>& BakedPixels = BakeOutput.PropertyData.FindChecked(Property);

	if (BakedPixels.Num() <= 0)
	{
		return -1;
	}

	// NOTE: the emissive value of each pixel will be incremented by 10 each time a triangle is drawn on it.
	// Therefore a value of 0.0 indicates an unreferenced pixel, a value of 10.0 indicates a uniquely referenced pixel,
	// and a value of 20+ indicates an overlapping pixel.
	// The value may differ somewhat from what was specified in the material due to conversions (color-space, gamma, float => uint etc),
	// so to be safe we set the limit at 15 instead of 10 to prevent incorrectly flagging some pixels as overlapping.
	const float EmissiveThreshold = 15.0f;

	int32 ValidCount = 0;
	int32 OverlapCount = 0;

	if (BakeOutput.EmissiveScale > EmissiveThreshold)
	{
		// This should never overflow since we know that the quotient will be <= 1.0
		const uint8 ColorThreshold = FMath::RoundToInt((EmissiveThreshold / BakeOutput.EmissiveScale) * 255.0f);

		bool bIsMagenta;
		bool bIsOverlapping;

		for (const FColor& Pixel: BakedPixels)
		{
			bIsMagenta = Pixel == FColor::Magenta;
			bIsOverlapping = Pixel.G > ColorThreshold;

			ValidCount += !bIsMagenta ? 1 : 0;
			OverlapCount += !bIsMagenta && bIsOverlapping ? 1 : 0;
		}
	}

	if (ValidCount == 0)
	{
		return -1;
	}

	if (ValidCount == OverlapCount)
	{
		return 1;
	}

	return static_cast<float>(OverlapCount) / static_cast<float>(ValidCount);
}

UMaterialInterface* FGLTFUVOverlapChecker::GetMaterial()
{
	static UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Sketchfab/Materials/OverlappingUVs.OverlappingUVs"));
	check(Material);
	return Material;
}
