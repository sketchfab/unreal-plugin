// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFNameUtility.h"
#include "Animation/SkeletalMeshActor.h"
#include "Engine.h"

namespace
{
	template <typename ActorType>
	FString GetActorNameIfOwnerOfType(const UActorComponent* Component)
	{
		if (const ActorType* Owner = Cast<ActorType>(Component->GetOwner()))
		{
			return Owner->GetName();
		}

		return Component->GetName();
	}
}

FString FGLTFNameUtility::GetName(const USceneComponent* Component)
{
	if (const AActor* Owner = Component->GetOwner())
	{
		return Owner->GetName() + TEXT("_") + Component->GetName();
	}

	return Component->GetName();
}

FString FGLTFNameUtility::GetName(const UStaticMeshComponent* Component)
{
	return GetActorNameIfOwnerOfType<AStaticMeshActor>(Component);
}

FString FGLTFNameUtility::GetName(const USkeletalMeshComponent* Component)
{
	return GetActorNameIfOwnerOfType<ASkeletalMeshActor>(Component);
}

FString FGLTFNameUtility::GetName(const ULightComponent* Component)
{
	return GetActorNameIfOwnerOfType<ALight>(Component);
}

FString FGLTFNameUtility::GetName(const UCameraComponent* Component)
{
	return GetActorNameIfOwnerOfType<ACameraActor>(Component);
}
