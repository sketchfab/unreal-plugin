// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Converters/SKGLTFConverter.h"
#include "Converters/SKGLTFBuilderContext.h"
#include "Actors/SKGLTFCameraActor.h"
#include "Actors/SKGLTFHotspotActor.h"

class FGLTFHotspotConverter final : public FGLTFBuilderContext, public TGLTFConverter<FGLTFJsonHotspotIndex, const ASKGLTFHotspotActor*>
{
	using FGLTFBuilderContext::FGLTFBuilderContext;

	virtual FGLTFJsonHotspotIndex Convert(const ASKGLTFHotspotActor* HotspotActor) override;
};
