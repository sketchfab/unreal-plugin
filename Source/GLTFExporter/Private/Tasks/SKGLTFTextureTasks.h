// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Tasks/SKGLTFTask.h"
#include "Builders/SKGLTFConvertBuilder.h"
#include "Engine.h"

class FGLTFTexture2DTask : public FGLTFTask
{
public:

	FGLTFTexture2DTask(FGLTFConvertBuilder& Builder, const UTexture2D* Texture2D, FGLTFJsonTextureIndex TextureIndex)
		: FGLTFTask(ESKGLTFTaskPriority::Texture)
		, Builder(Builder)
		, Texture2D(Texture2D)
		, TextureIndex(TextureIndex)
	{
	}

	virtual FString GetName() override
	{
		return Texture2D->GetName();
	}

	virtual void Complete() override;

private:

	FGLTFConvertBuilder& Builder;
	const UTexture2D* Texture2D;
	const FGLTFJsonTextureIndex TextureIndex;
};

class FGLTFTextureCubeTask : public FGLTFTask
{
public:

	FGLTFTextureCubeTask(FGLTFConvertBuilder& Builder, const UTextureCube* TextureCube, ECubeFace CubeFace, FGLTFJsonTextureIndex TextureIndex)
		: FGLTFTask(ESKGLTFTaskPriority::Texture)
		, Builder(Builder)
		, TextureCube(TextureCube)
		, CubeFace(CubeFace)
		, TextureIndex(TextureIndex)
	{
	}

	virtual FString GetName() override
	{
		return TextureCube->GetName();
	}

	virtual void Complete() override;

private:

	FGLTFConvertBuilder& Builder;
	const UTextureCube* TextureCube;
	ECubeFace CubeFace;
	const FGLTFJsonTextureIndex TextureIndex;
};

class FGLTFTextureRenderTarget2DTask : public FGLTFTask
{
public:

	FGLTFTextureRenderTarget2DTask(FGLTFConvertBuilder& Builder, const UTextureRenderTarget2D* RenderTarget2D, FGLTFJsonTextureIndex TextureIndex)
		: FGLTFTask(ESKGLTFTaskPriority::Texture)
		, Builder(Builder)
		, RenderTarget2D(RenderTarget2D)
		, TextureIndex(TextureIndex)
	{
	}

	virtual FString GetName() override
	{
		return RenderTarget2D->GetName();
	}

	virtual void Complete() override;

private:

	FGLTFConvertBuilder& Builder;
	const UTextureRenderTarget2D* RenderTarget2D;
	const FGLTFJsonTextureIndex TextureIndex;
};

class FGLTFTextureRenderTargetCubeTask : public FGLTFTask
{
public:

	FGLTFTextureRenderTargetCubeTask(FGLTFConvertBuilder& Builder, const UTextureRenderTargetCube* RenderTargetCube, ECubeFace CubeFace, FGLTFJsonTextureIndex TextureIndex)
		: FGLTFTask(ESKGLTFTaskPriority::Texture)
		, Builder(Builder)
		, RenderTargetCube(RenderTargetCube)
		, CubeFace(CubeFace)
		, TextureIndex(TextureIndex)
	{
	}

	virtual FString GetName() override
	{
		return RenderTargetCube->GetName();
	}

	virtual void Complete() override;

private:

	FGLTFConvertBuilder& Builder;
	const UTextureRenderTargetCube* RenderTargetCube;
	ECubeFace CubeFace;
	const FGLTFJsonTextureIndex TextureIndex;
};

class FGLTFTextureLightMapTask : public FGLTFTask
{
public:

	FGLTFTextureLightMapTask(FGLTFConvertBuilder& Builder, const ULightMapTexture2D* LightMap, FGLTFJsonTextureIndex TextureIndex)
		: FGLTFTask(ESKGLTFTaskPriority::Texture)
		, Builder(Builder)
		, LightMap(LightMap)
		, TextureIndex(TextureIndex)
	{
	}

	virtual FString GetName() override
	{
		return LightMap->GetName();
	}

	virtual void Complete() override;

private:

	FGLTFConvertBuilder& Builder;
	const ULightMapTexture2D* LightMap;
	const FGLTFJsonTextureIndex TextureIndex;
};
