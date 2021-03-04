// Copyright Epic Games, Inc. All Rights Reserved.

#include "Exporters/GLTFLevelVariantSetsExporter.h"
#include "Exporters/GLTFExporterUtility.h"
#include "Builders/GLTFContainerBuilder.h"
#include "LevelVariantSets.h"

UGLTFLevelVariantSetsExporter::UGLTFLevelVariantSetsExporter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = ULevelVariantSets::StaticClass();
}

bool UGLTFLevelVariantSetsExporter::AddObject(FGLTFContainerBuilder& Builder, const UObject* Object)
{
	const ULevelVariantSets* LevelVariantSets = CastChecked<ULevelVariantSets>(Object);

	if (!Builder.ExportOptions->bExportVariantSets)
	{
		Builder.AddErrorMessage(
			FString::Printf(TEXT("Failed to export level variant sets %s because variant sets are disabled by export options"),
			*LevelVariantSets->GetName()));
		return false;
	}

	TArray<UWorld*> Worlds = FGLTFExporterUtility::GetReferencedWorlds(LevelVariantSets);
	if (Worlds.Num() == 0)
	{
		Builder.AddErrorMessage(
			FString::Printf(TEXT("Failed to export level variant sets %s because no level referenced"),
			*LevelVariantSets->GetName()));
		return false;
	}

	if (Worlds.Num() > 1)
	{
		Builder.AddErrorMessage(
            FString::Printf(TEXT("Failed to export level variant sets %s because more than one level referenced"),
            *LevelVariantSets->GetName()));
		return false;
	}

	UWorld* World = Worlds[0];
	const FGLTFJsonSceneIndex SceneIndex = Builder.GetOrAddScene(World);
	if (SceneIndex == INDEX_NONE)
	{
		Builder.AddErrorMessage(
			FString::Printf(TEXT("Failed to export level %s for level variant sets %s"),
			*World->GetName(),
			*LevelVariantSets->GetName()));
		return false;
	}

	const FGLTFJsonVariationIndex VariationIndex = Builder.GetOrAddVariation(LevelVariantSets);
	if (VariationIndex == INDEX_NONE)
	{
		Builder.AddErrorMessage(
			FString::Printf(TEXT("Failed to export level variant sets %s"),
			*LevelVariantSets->GetName()));
		return false;
	}

	Builder.GetScene(SceneIndex).Variations.AddUnique(VariationIndex);

	Builder.DefaultScene = SceneIndex;
	return true;
}
