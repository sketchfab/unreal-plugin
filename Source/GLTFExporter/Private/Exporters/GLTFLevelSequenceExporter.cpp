// Copyright Epic Games, Inc. All Rights Reserved.

#include "Exporters/SKGLTFLevelSequenceExporter.h"
#include "Exporters/SKGLTFExporterUtility.h"
#include "Builders/SKGLTFContainerBuilder.h"
#include "LevelSequence.h"

USKGLTFLevelSequenceExporter::USKGLTFLevelSequenceExporter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = ULevelSequence::StaticClass();
}

bool USKGLTFLevelSequenceExporter::AddObject(FGLTFContainerBuilder& Builder, const UObject* Object)
{
	const ULevelSequence* LevelSequence = CastChecked<ULevelSequence>(Object);

	if (!Builder.ExportOptions->bExportLevelSequences)
	{
		Builder.AddErrorMessage(
			FString::Printf(TEXT("Failed to export level sequence %s because level sequences are disabled by export options"),
			*LevelSequence->GetName()));
		return false;
	}

	TArray<UWorld*> Worlds = FGLTFExporterUtility::GetReferencedWorlds(LevelSequence);
	if (Worlds.Num() == 0)
	{
		Builder.AddErrorMessage(
			FString::Printf(TEXT("Failed to export level sequence %s because no level referenced"),
			*LevelSequence->GetName()));
		return false;
	}

	if (Worlds.Num() > 1)
	{
		Builder.AddErrorMessage(
            FString::Printf(TEXT("Failed to export level sequence %s because more than one level referenced"),
            *LevelSequence->GetName()));
		return false;
	}

	UWorld* World = Worlds[0];
	const FGLTFJsonSceneIndex SceneIndex = Builder.GetOrAddScene(World);
	if (SceneIndex == INDEX_NONE)
	{
		Builder.AddErrorMessage(
			FString::Printf(TEXT("Failed to export level %s for level sequence %s"),
			*World->GetName(),
			*LevelSequence->GetName()));
		return false;
	}

	const FGLTFJsonAnimationIndex AnimationIndex = Builder.GetOrAddAnimation(World->PersistentLevel, LevelSequence);
	if (AnimationIndex == INDEX_NONE)
	{
		Builder.AddErrorMessage(
			FString::Printf(TEXT("Failed to export level sequence %s"),
			*LevelSequence->GetName()));
		return false;
	}

	return true;
}
