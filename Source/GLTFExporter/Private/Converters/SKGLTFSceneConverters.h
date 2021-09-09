// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Converters/SKGLTFConverter.h"
#include "Converters/SKGLTFBuilderContext.h"
#include "Engine.h"

class FGLTFSceneConverter final : public FGLTFBuilderContext, public TGLTFConverter<FGLTFJsonSceneIndex, const UWorld*>
{
	using FGLTFBuilderContext::FGLTFBuilderContext;

	virtual FGLTFJsonSceneIndex Convert(const UWorld* Level) override;
};
