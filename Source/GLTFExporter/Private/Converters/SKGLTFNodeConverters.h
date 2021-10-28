// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Converters/SKGLTFConverter.h"
#include "Converters/SKGLTFBuilderContext.h"
#include "Engine.h"

template <typename... InputTypes>
class TGLTFNodeConverter : public FGLTFBuilderContext, public TGLTFConverter<FGLTFJsonNodeIndex, InputTypes...>
{
	using FGLTFBuilderContext::FGLTFBuilderContext;
};

class FGLTFActorConverter final : public TGLTFNodeConverter<const AActor*>
{
	using TGLTFNodeConverter::TGLTFNodeConverter;

	virtual FGLTFJsonNodeIndex Convert(const AActor* Actor) override;
};

class FGLTFComponentConverter final : public TGLTFNodeConverter<const USceneComponent*>
{
	using TGLTFNodeConverter::TGLTFNodeConverter;

	virtual FGLTFJsonNodeIndex Convert(const USceneComponent* SceneComponent) override;
};

class FGLTFComponentSocketConverter final : public TGLTFNodeConverter<const USceneComponent*, FName>
{
	using TGLTFNodeConverter::TGLTFNodeConverter;

	virtual FGLTFJsonNodeIndex Convert(const USceneComponent* SceneComponent, FName SocketName) override;
};

class FGLTFStaticSocketConverter final : public TGLTFNodeConverter<FGLTFJsonNodeIndex, const UStaticMesh*, FName>
{
	using TGLTFNodeConverter::TGLTFNodeConverter;

	virtual FGLTFJsonNodeIndex Convert(FGLTFJsonNodeIndex RootNode, const UStaticMesh* StaticMesh, FName SocketName) override;
};

class FGLTFSkeletalSocketConverter final : public TGLTFNodeConverter<FGLTFJsonNodeIndex, const USkeletalMesh*, FName>
{
	using TGLTFNodeConverter::TGLTFNodeConverter;

	virtual FGLTFJsonNodeIndex Convert(FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh, FName SocketName) override;
};

class FGLTFSkeletalBoneConverter final : public TGLTFNodeConverter<FGLTFJsonNodeIndex, const USkeletalMesh*, int32>
{
	using TGLTFNodeConverter::TGLTFNodeConverter;

	virtual FGLTFJsonNodeIndex Convert(FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh, int32 BoneIndex) override;
};
