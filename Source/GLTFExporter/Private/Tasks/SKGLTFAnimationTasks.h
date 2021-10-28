// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Tasks/SKGLTFTask.h"
#include "Builders/SKGLTFConvertBuilder.h"
#include "LevelSequenceActor.h"
#include "LevelSequence.h"

class FGLTFAnimSequenceTask : public FGLTFTask
{
public:

	FGLTFAnimSequenceTask(FGLTFConvertBuilder& Builder, FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh, const UAnimSequence* AnimSequence, FGLTFJsonAnimationIndex AnimationIndex)
		: FGLTFTask(ESKGLTFTaskPriority::Animation)
		, Builder(Builder)
		, RootNode(RootNode)
		, SkeletalMesh(SkeletalMesh)
		, AnimSequence(AnimSequence)
		, AnimationIndex(AnimationIndex)
	{
	}

	virtual FString GetName() override
	{
		return AnimSequence->GetName();
	}

	virtual void Complete() override;

private:

	FGLTFConvertBuilder& Builder;
	FGLTFJsonNodeIndex RootNode;
	const USkeletalMesh* SkeletalMesh;
	const UAnimSequence* AnimSequence;
	const FGLTFJsonAnimationIndex AnimationIndex;
};

class FGLTFLevelSequenceTask : public FGLTFTask
{
public:

	FGLTFLevelSequenceTask(FGLTFConvertBuilder& Builder, const ULevel* Level, const ULevelSequence* LevelSequence, FGLTFJsonAnimationIndex AnimationIndex)
		: FGLTFTask(ESKGLTFTaskPriority::Animation)
		, Builder(Builder)
		, Level(Level)
		, LevelSequence(LevelSequence)
		, AnimationIndex(AnimationIndex)
	{
	}

	virtual FString GetName() override
	{
		return LevelSequence->GetName();
	}

	virtual void Complete() override;

private:

	FGLTFConvertBuilder& Builder;
	const ULevel* Level;
	const ULevelSequence* LevelSequence;
	const FGLTFJsonAnimationIndex AnimationIndex;
};
