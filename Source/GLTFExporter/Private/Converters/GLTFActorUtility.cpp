// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFActorUtility.h"
#include "Engine/Blueprint.h"

bool FGLTFActorUtility::IsRootActor(const AActor* Actor, bool bSelectedOnly)
{
	const AActor* ParentActor = Actor->GetAttachParentActor();
	return ParentActor == nullptr || (bSelectedOnly && !ParentActor->IsSelected());
}

UBlueprint* FGLTFActorUtility::GetBlueprintFromActor(const AActor* Actor)
{
	return UBlueprint::GetBlueprintFromClass(Actor->GetClass());
}

bool FGLTFActorUtility::IsSkySphereBlueprint(const UBlueprint* Blueprint)
{
	// TODO: what if a blueprint inherits BP_Sky_Sphere?
	return Blueprint != nullptr && Blueprint->GetPathName().Equals(TEXT("/Engine/EngineSky/BP_Sky_Sphere.BP_Sky_Sphere"));
}

bool FGLTFActorUtility::IsHDRIBackdropBlueprint(const UBlueprint* Blueprint)
{
	// TODO: what if a blueprint inherits HDRIBackdrop?
	return Blueprint != nullptr && Blueprint->GetPathName().Equals(TEXT("/HDRIBackdrop/Blueprints/HDRIBackdrop.HDRIBackdrop"));
}
