// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Converters/SKGLTFConverter.h"
#include "Converters/SKGLTFBuilderContext.h"
#include "Engine.h"

class FGLTFBackdropConverter final : public FGLTFBuilderContext, public TGLTFConverter<FGLTFJsonBackdropIndex, const AActor*>
{
	using FGLTFBuilderContext::FGLTFBuilderContext;

	virtual FGLTFJsonBackdropIndex Convert(const AActor* BackdropActor) override;
};
