// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Converters/SKGLTFConverter.h"
#include "Converters/SKGLTFBuilderContext.h"
#include "Engine.h"

class FGLTFLightMapConverter final : public FGLTFBuilderContext, public TGLTFConverter<FGLTFJsonLightMapIndex, const UStaticMeshComponent*>
{
	using FGLTFBuilderContext::FGLTFBuilderContext;

	virtual FGLTFJsonLightMapIndex Convert(const UStaticMeshComponent* StaticMeshComponent) override;
};
