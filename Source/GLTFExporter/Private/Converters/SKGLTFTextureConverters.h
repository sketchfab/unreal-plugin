// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Converters/SKGLTFConverter.h"
#include "Converters/SKGLTFBuilderContext.h"
#include "Engine.h"

template <typename... InputTypes>
class TGLTFTextureConverter : public FGLTFBuilderContext, public TGLTFConverter<FGLTFJsonTextureIndex, InputTypes...>
{
	using FGLTFBuilderContext::FGLTFBuilderContext;
};

class FGLTFTexture2DConverter final : public TGLTFTextureConverter<const UTexture2D*>
{
	using TGLTFTextureConverter::TGLTFTextureConverter;

	virtual FGLTFJsonTextureIndex Convert(const UTexture2D* Texture2D) override;
};

class FGLTFTextureCubeConverter final : public TGLTFTextureConverter<const UTextureCube*, ECubeFace>
{
	using TGLTFTextureConverter::TGLTFTextureConverter;

	virtual FGLTFJsonTextureIndex Convert(const UTextureCube* TextureCube, ECubeFace CubeFace) override;
};

class FGLTFTextureRenderTarget2DConverter final : public TGLTFTextureConverter<const UTextureRenderTarget2D*>
{
	using TGLTFTextureConverter::TGLTFTextureConverter;

	virtual FGLTFJsonTextureIndex Convert(const UTextureRenderTarget2D* RenderTarget2D) override;
};

class FGLTFTextureRenderTargetCubeConverter final : public TGLTFTextureConverter<const UTextureRenderTargetCube*, ECubeFace>
{
	using TGLTFTextureConverter::TGLTFTextureConverter;

	virtual FGLTFJsonTextureIndex Convert(const UTextureRenderTargetCube* RenderTargetCube, ECubeFace CubeFace) override;
};

class FGLTFTextureLightMapConverter final : public TGLTFTextureConverter<const ULightMapTexture2D*>
{
	using TGLTFTextureConverter::TGLTFTextureConverter;

	virtual FGLTFJsonTextureIndex Convert(const ULightMapTexture2D* LightMap) override;
};
