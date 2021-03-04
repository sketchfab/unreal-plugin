// Copyright Epic Games, Inc. All Rights Reserved.

#include "Exporters/GLTFLevelExporter.h"
#include "Builders/GLTFContainerBuilder.h"
#include "Engine/World.h"

UGLTFLevelExporter::UGLTFLevelExporter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UWorld::StaticClass();
}

bool UGLTFLevelExporter::AddObject(FGLTFContainerBuilder& Builder, const UObject* Object)
{
	const UWorld* World = CastChecked<UWorld>(Object);

	const FGLTFJsonSceneIndex SceneIndex = Builder.GetOrAddScene(World);
	if (SceneIndex == INDEX_NONE)
	{
		Builder.AddErrorMessage(FString::Printf(TEXT("Failed to export level %s"), *World->GetName()));
		return false;
	}

	Builder.DefaultScene = SceneIndex;
	return true;
}
