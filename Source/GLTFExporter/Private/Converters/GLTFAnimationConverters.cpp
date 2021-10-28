// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFAnimationConverters.h"
#include "Builders/SKGLTFConvertBuilder.h"
#include "Tasks/SKGLTFAnimationTasks.h"
#include "Animation/AnimSequence.h"

FGLTFJsonAnimationIndex FGLTFAnimationConverter::Convert(FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh, const UAnimSequence* AnimSequence)
{
#if ENGINE_MAJOR_VERSION == 5
	if (AnimSequence->GetDataModel()->GetNumberOfKeys() < 0)
#else
	if (AnimSequence->GetRawNumberOfFrames() < 0)
#endif
	{
		// TODO: report warning
		return FGLTFJsonAnimationIndex(INDEX_NONE);
	}

	const USkeleton* Skeleton = AnimSequence->GetSkeleton();
	if (Skeleton == nullptr)
	{
		// TODO: report error
		return FGLTFJsonAnimationIndex(INDEX_NONE);
	}

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
	if (Skeleton != SkeletalMesh->Skeleton)
#else
	if (Skeleton != SkeletalMesh->GetSkeleton())
#endif
	{
		// TODO: report error
		return FGLTFJsonAnimationIndex(INDEX_NONE);
	}

	const FGLTFJsonAnimationIndex AnimationIndex = Builder.AddAnimation();
	Builder.SetupTask<FGLTFAnimSequenceTask>(Builder, RootNode, SkeletalMesh, AnimSequence, AnimationIndex);
	return AnimationIndex;
}

FGLTFJsonAnimationIndex FGLTFAnimationDataConverter::Convert(FGLTFJsonNodeIndex RootNode, const USkeletalMeshComponent* SkeletalMeshComponent)
{
	const USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->SkeletalMesh;
	const UAnimSequence* AnimSequence = Cast<UAnimSequence>(SkeletalMeshComponent->AnimationData.AnimToPlay);

	if (SkeletalMesh == nullptr || AnimSequence == nullptr || SkeletalMeshComponent->GetAnimationMode() != EAnimationMode::AnimationSingleNode)
	{
		return FGLTFJsonAnimationIndex(INDEX_NONE);
	}

	const FGLTFJsonAnimationIndex AnimationIndex = Builder.GetOrAddAnimation(RootNode, SkeletalMesh, AnimSequence);
	if (AnimationIndex != INDEX_NONE && Builder.ExportOptions->bExportPlaybackSettings)
	{
		FGLTFJsonAnimation& JsonAnimation = Builder.GetAnimation(AnimationIndex);
		FGLTFJsonAnimationPlayback& JsonPlayback = JsonAnimation.Playback;

		JsonPlayback.bLoop = SkeletalMeshComponent->AnimationData.bSavedLooping;
		JsonPlayback.bAutoPlay = SkeletalMeshComponent->AnimationData.bSavedPlaying;
		JsonPlayback.PlayRate = SkeletalMeshComponent->AnimationData.SavedPlayRate;
		JsonPlayback.StartTime = SkeletalMeshComponent->AnimationData.SavedPosition;
	}

	return AnimationIndex;
}

FGLTFJsonAnimationIndex FGLTFLevelSequenceConverter::Convert(const ULevel* Level, const ULevelSequence* LevelSequence)
{
	const FGLTFJsonAnimationIndex AnimationIndex = Builder.AddAnimation();
	Builder.SetupTask<FGLTFLevelSequenceTask>(Builder, Level, LevelSequence, AnimationIndex);
	return AnimationIndex;
}

FGLTFJsonAnimationIndex FGLTFLevelSequenceDataConverter::Convert(const ALevelSequenceActor* LevelSequenceActor)
{
	const ULevel* Level = LevelSequenceActor->GetLevel();
	const ULevelSequence* LevelSequence = LevelSequenceActor->LoadSequence();

	if (Level == nullptr || LevelSequence == nullptr)
	{
		return FGLTFJsonAnimationIndex(INDEX_NONE);
	}

	const FGLTFJsonAnimationIndex AnimationIndex = Builder.GetOrAddAnimation(Level, LevelSequence);
	if (AnimationIndex != INDEX_NONE && Builder.ExportOptions->bExportPlaybackSettings)
	{
		FGLTFJsonAnimation& JsonAnimation = Builder.GetAnimation(AnimationIndex);
		FGLTFJsonAnimationPlayback& JsonPlayback = JsonAnimation.Playback;

		// TODO: report warning if loop count is not 0 or -1 (infinite)
		JsonPlayback.bLoop = LevelSequenceActor->PlaybackSettings.LoopCount.Value != 0;
		JsonPlayback.bAutoPlay = LevelSequenceActor->PlaybackSettings.bAutoPlay;
		JsonPlayback.PlayRate = LevelSequenceActor->PlaybackSettings.PlayRate;
		JsonPlayback.StartTime = LevelSequenceActor->PlaybackSettings.StartTime;
	}

	return AnimationIndex;
}
