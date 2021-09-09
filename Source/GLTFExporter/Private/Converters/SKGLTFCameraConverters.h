// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Converters/SKGLTFConverter.h"
#include "Converters/SKGLTFBuilderContext.h"
#include "Engine.h"

class FGLTFCameraConverter final : public FGLTFBuilderContext, public TGLTFConverter<FGLTFJsonCameraIndex, const UCameraComponent*>
{
	using FGLTFBuilderContext::FGLTFBuilderContext;

	virtual FGLTFJsonCameraIndex Convert(const UCameraComponent* CameraComponent) override;
};
