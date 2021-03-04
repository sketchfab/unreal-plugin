// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/GLTFJsonIndex.h"
#include "Converters/GLTFConverter.h"
#include "Converters/GLTFBuilderContext.h"
#include "Engine.h"

class FGLTFCameraConverter final : public FGLTFBuilderContext, public TGLTFConverter<FGLTFJsonCameraIndex, const UCameraComponent*>
{
	using FGLTFBuilderContext::FGLTFBuilderContext;

	virtual FGLTFJsonCameraIndex Convert(const UCameraComponent* CameraComponent) override;
};
