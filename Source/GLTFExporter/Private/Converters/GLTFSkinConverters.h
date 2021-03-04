// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/GLTFJsonIndex.h"
#include "Converters/GLTFConverter.h"
#include "Converters/GLTFBuilderContext.h"
#include "Engine.h"

class FGLTFSkinConverter final : public FGLTFBuilderContext, public TGLTFConverter<FGLTFJsonSkinIndex, FGLTFJsonNodeIndex, const USkeletalMesh*>
{
	using FGLTFBuilderContext::FGLTFBuilderContext;

	virtual FGLTFJsonSkinIndex Convert(FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh) override;
};
