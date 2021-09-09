// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFMaterialConverters.h"
#include "Converters/SKGLTFMaterialUtility.h"
#include "Builders/SKGLTFConvertBuilder.h"
#include "Tasks/SKGLTFMaterialTasks.h"

void FGLTFMaterialConverter::Sanitize(const UMaterialInterface*& Material, const FGLTFMeshData*& MeshData, FGLTFIndexArray& SectionIndices)
{
	if (MeshData == nullptr ||
		Builder.ExportOptions->BakeMaterialInputs != ESKGLTFMaterialBakeMode::UseMeshData ||
		!FGLTFMaterialUtility::NeedsMeshData(Material))
	{
		MeshData = nullptr;
		SectionIndices = {};
	}

	if (MeshData != nullptr)
	{
		const FMeshDescription& MeshDescription = MeshData->GetParent()->Description;

		const float DegenerateUVPercentage = UVDegenerateChecker.GetOrAdd(&MeshDescription, SectionIndices, MeshData->TexCoord);
		if (FMath::IsNearlyEqual(DegenerateUVPercentage, 1))
		{
			FString SectionString = TEXT("mesh section");
			SectionString += SectionIndices.Num() > 1 ? TEXT("s ") : TEXT(" ");
			SectionString += FString::JoinBy(SectionIndices, TEXT(", "), FString::FromInt);

			Builder.AddWarningMessage(FString::Printf(
				TEXT("Material %s uses mesh data from %s but the lightmap UVs (channel %d) are nearly 100%% degenerate (in %s). Simple baking will be used as fallback"),
				*Material->GetName(),
				*MeshData->GetParent()->Name,
				MeshData->TexCoord,
				*SectionString));

			MeshData = nullptr;
			SectionIndices = {};
		}
	}
}

FGLTFJsonMaterialIndex FGLTFMaterialConverter::Convert(const UMaterialInterface* Material, const FGLTFMeshData* MeshData, FGLTFIndexArray SectionIndices)
{
	if (Material == FGLTFMaterialUtility::GetDefault())
	{
		return FGLTFJsonMaterialIndex(INDEX_NONE); // use default gltf definition
	}

	const FGLTFJsonMaterialIndex MaterialIndex = Builder.AddMaterial();
	Builder.SetupTask<FGLTFMaterialTask>(Builder, UVOverlapChecker, Material, MeshData, SectionIndices, MaterialIndex);
	return MaterialIndex;
}
