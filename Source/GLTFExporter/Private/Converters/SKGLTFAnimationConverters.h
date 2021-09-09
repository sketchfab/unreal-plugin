// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Converters/SKGLTFConverter.h"
#include "Converters/SKGLTFBuilderContext.h"
#include "Engine.h"

class ULevelSequence;
class ALevelSequenceActor;

template <typename... InputTypes>
class TGLTFAnimationConverter : public FGLTFBuilderContext, public TGLTFConverter<FGLTFJsonAnimationIndex, InputTypes...>
{
	using FGLTFBuilderContext::FGLTFBuilderContext;
};

class FGLTFAnimationConverter final : public TGLTFAnimationConverter<FGLTFJsonNodeIndex, const USkeletalMesh*, const UAnimSequence*>
{
	using TGLTFAnimationConverter::TGLTFAnimationConverter;

	virtual FGLTFJsonAnimationIndex Convert(FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh, const UAnimSequence* AnimSequence) override;
};

class FGLTFAnimationDataConverter final : public TGLTFAnimationConverter<FGLTFJsonNodeIndex, const USkeletalMeshComponent*>
{
	using TGLTFAnimationConverter::TGLTFAnimationConverter;

	virtual FGLTFJsonAnimationIndex Convert(FGLTFJsonNodeIndex RootNode, const USkeletalMeshComponent* SkeletalMeshComponent) override;
};

class FGLTFLevelSequenceConverter final : public TGLTFAnimationConverter<const ULevel*, const ULevelSequence*>
{
	using TGLTFAnimationConverter::TGLTFAnimationConverter;

	virtual FGLTFJsonAnimationIndex Convert(const ULevel* Level, const ULevelSequence*) override;
};

class FGLTFLevelSequenceDataConverter final : public TGLTFAnimationConverter<const ALevelSequenceActor*>
{
	using TGLTFAnimationConverter::TGLTFAnimationConverter;

	virtual FGLTFJsonAnimationIndex Convert(const ALevelSequenceActor* LevelSequenceActor) override;
};
