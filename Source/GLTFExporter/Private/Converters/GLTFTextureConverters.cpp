// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFTextureConverters.h"
#include "Builders/SKGLTFContainerBuilder.h"
#include "Tasks/SKGLTFTextureTasks.h"

FGLTFJsonTextureIndex FGLTFTexture2DConverter::Convert(const UTexture2D* Texture2D)
{
	if (Builder.ExportOptions->TextureImageFormat == ESKGLTFTextureImageFormat::None)
	{
		return FGLTFJsonTextureIndex(INDEX_NONE);
	}

	const FGLTFJsonTextureIndex TextureIndex = Builder.AddTexture();
	Builder.SetupTask<FGLTFTexture2DTask>(Builder, Texture2D, TextureIndex);
	return TextureIndex;
}

FGLTFJsonTextureIndex FGLTFTextureCubeConverter::Convert(const UTextureCube* TextureCube, ECubeFace CubeFace)
{
	if (Builder.ExportOptions->TextureImageFormat == ESKGLTFTextureImageFormat::None)
	{
		return FGLTFJsonTextureIndex(INDEX_NONE);
	}

	const FGLTFJsonTextureIndex TextureIndex = Builder.AddTexture();
	Builder.SetupTask<FGLTFTextureCubeTask>(Builder, TextureCube, CubeFace, TextureIndex);
	return TextureIndex;
}

FGLTFJsonTextureIndex FGLTFTextureRenderTarget2DConverter::Convert(const UTextureRenderTarget2D* RenderTarget2D)
{
	if (Builder.ExportOptions->TextureImageFormat == ESKGLTFTextureImageFormat::None)
	{
		return FGLTFJsonTextureIndex(INDEX_NONE);
	}

	const FGLTFJsonTextureIndex TextureIndex = Builder.AddTexture();
	Builder.SetupTask<FGLTFTextureRenderTarget2DTask>(Builder, RenderTarget2D, TextureIndex);
	return TextureIndex;
}

FGLTFJsonTextureIndex FGLTFTextureRenderTargetCubeConverter::Convert(const UTextureRenderTargetCube* RenderTargetCube, ECubeFace CubeFace)
{
	if (Builder.ExportOptions->TextureImageFormat == ESKGLTFTextureImageFormat::None)
	{
		return FGLTFJsonTextureIndex(INDEX_NONE);
	}

	const FGLTFJsonTextureIndex TextureIndex = Builder.AddTexture();
	Builder.SetupTask<FGLTFTextureRenderTargetCubeTask>(Builder, RenderTargetCube, CubeFace, TextureIndex);
	return TextureIndex;
}

FGLTFJsonTextureIndex FGLTFTextureLightMapConverter::Convert(const ULightMapTexture2D* LightMap)
{
	if (Builder.ExportOptions->TextureImageFormat == ESKGLTFTextureImageFormat::None)
	{
		return FGLTFJsonTextureIndex(INDEX_NONE);
	}

	const FGLTFJsonTextureIndex TextureIndex = Builder.AddTexture();
	Builder.SetupTask<FGLTFTextureLightMapTask>(Builder, LightMap, TextureIndex);
	return TextureIndex;
}
