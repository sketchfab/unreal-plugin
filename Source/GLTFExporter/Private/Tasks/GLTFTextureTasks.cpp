// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tasks/SKGLTFTextureTasks.h"
#include "Builders/SKGLTFContainerBuilder.h"
#include "Converters/SKGLTFConverterUtility.h"
#include "Converters/SKGLTFTextureUtility.h"
#include "Converters/SKGLTFNameUtility.h"

void FGLTFTexture2DTask::Complete()
{
	FGLTFJsonTexture& JsonTexture = Builder.GetTexture(TextureIndex);
	Texture2D->GetName(JsonTexture.Name);

	const bool bIsHDR = FGLTFTextureUtility::IsHDR(Texture2D->GetPixelFormat());
	const FIntPoint Size = FGLTFTextureUtility::GetInGameSize(Texture2D);
	UTextureRenderTarget2D* RenderTarget = FGLTFTextureUtility::CreateRenderTarget(Size, bIsHDR);

	// TODO: preserve maximum image quality (avoid compression artifacts) by copying source data (and adjustments) to a temp texture
	FGLTFTextureUtility::DrawTexture(RenderTarget, Texture2D, FVector2D::ZeroVector, Size);

	if (!Texture2D->IsNormalMap() && bIsHDR)
	{
		JsonTexture.Encoding = Builder.GetTextureHDREncoding();
	}

	TArray<FColor> Pixels;
	if (!FGLTFTextureUtility::ReadPixels(RenderTarget, Pixels, JsonTexture.Encoding))
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Failed to read pixels for 2D texture %s"), *JsonTexture.Name));
		return;
	}

	if (Texture2D->IsNormalMap())
	{
		FGLTFTextureUtility::FlipGreenChannel(Pixels);
	}

	const bool bIgnoreAlpha = FGLTFTextureUtility::IsAlphaless(Texture2D->GetPixelFormat());
	const ESKGLTFTextureType Type =
		Texture2D->IsNormalMap() ? ESKGLTFTextureType::Normalmaps :
		bIsHDR ? ESKGLTFTextureType::HDR : ESKGLTFTextureType::None;

	JsonTexture.Source = Builder.AddImage(Pixels, Size, bIgnoreAlpha, Type, JsonTexture.Name);
	JsonTexture.Sampler = Builder.GetOrAddSampler(Texture2D);
}

void FGLTFTextureCubeTask::Complete()
{
	FGLTFJsonTexture& JsonTexture = Builder.GetTexture(TextureIndex);
	JsonTexture.Name = TextureCube->GetName() + TEXT("_") + FGLTFJsonUtility::ToString(FGLTFConverterUtility::ConvertCubeFace(CubeFace));

	// TODO: add optimized "happy path" if cube face doesn't need rotation and has suitable pixel format

	// TODO: preserve maximum image quality (avoid compression artifacts) by copying source data (and adjustments) to a temp texture
	const UTexture2D* FaceTexture = FGLTFTextureUtility::CreateTextureFromCubeFace(TextureCube, CubeFace);
	if (FaceTexture == nullptr)
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Failed to extract cube face %d for cubemap texture %s"), CubeFace, *TextureCube->GetName()));
		return;
	}

	const bool bIsHDR = FGLTFTextureUtility::IsHDR(TextureCube->GetPixelFormat());
	const FIntPoint Size = { TextureCube->GetSizeX(), TextureCube->GetSizeY() };
	UTextureRenderTarget2D* RenderTarget = FGLTFTextureUtility::CreateRenderTarget(Size, bIsHDR);

	const float FaceRotation = FGLTFTextureUtility::GetCubeFaceRotation(CubeFace);
	FGLTFTextureUtility::RotateTexture(RenderTarget, FaceTexture, FVector2D::ZeroVector, Size, FaceRotation);

	if (bIsHDR)
	{
		JsonTexture.Encoding = Builder.GetTextureHDREncoding();
	}

	TArray<FColor> Pixels;
	if (!FGLTFTextureUtility::ReadPixels(RenderTarget, Pixels, JsonTexture.Encoding))
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Failed to read pixels (cube face %d) for cubemap texture %s"), CubeFace, *TextureCube->GetName()));
		return;
	}

	const bool bIgnoreAlpha = FGLTFTextureUtility::IsAlphaless(TextureCube->GetPixelFormat());
	const ESKGLTFTextureType Type = bIsHDR ? ESKGLTFTextureType::HDR : ESKGLTFTextureType::None;

	JsonTexture.Source = Builder.AddImage(Pixels, Size, bIgnoreAlpha, Type, JsonTexture.Name);
	JsonTexture.Sampler = Builder.GetOrAddSampler(TextureCube);
}

void FGLTFTextureRenderTarget2DTask::Complete()
{
	FGLTFJsonTexture& JsonTexture = Builder.GetTexture(TextureIndex);
	RenderTarget2D->GetName(JsonTexture.Name);

	const bool bIsHDR = FGLTFTextureUtility::IsHDR(RenderTarget2D->GetFormat());
	const FIntPoint Size = { RenderTarget2D->SizeX, RenderTarget2D->SizeY };

	if (bIsHDR)
	{
		JsonTexture.Encoding = Builder.GetTextureHDREncoding();
	}

	TArray<FColor> Pixels;
	if (!FGLTFTextureUtility::ReadPixels(RenderTarget2D, Pixels, JsonTexture.Encoding))
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Failed to read pixels for 2D render target %s"), *JsonTexture.Name));
		return;
	}

	const bool bIgnoreAlpha = FGLTFTextureUtility::IsAlphaless(RenderTarget2D->GetFormat());
	const ESKGLTFTextureType Type = bIsHDR ? ESKGLTFTextureType::HDR : ESKGLTFTextureType::None;

	JsonTexture.Source = Builder.AddImage(Pixels, Size, bIgnoreAlpha, Type, JsonTexture.Name);
	JsonTexture.Sampler = Builder.GetOrAddSampler(RenderTarget2D);
}

void FGLTFTextureRenderTargetCubeTask::Complete()
{
	FGLTFJsonTexture& JsonTexture = Builder.GetTexture(TextureIndex);
	JsonTexture.Name = RenderTargetCube->GetName() + TEXT("_") + FGLTFJsonUtility::ToString(FGLTFConverterUtility::ConvertCubeFace(CubeFace));

	// TODO: add optimized "happy path" if cube face doesn't need rotation

	const UTexture2D* FaceTexture = FGLTFTextureUtility::CreateTextureFromCubeFace(RenderTargetCube, CubeFace);
	if (FaceTexture == nullptr)
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Failed to extract cube face %d for cubemap render target %s"), CubeFace, *RenderTargetCube->GetName()));
		return;
	}

	const bool bIsHDR = FGLTFTextureUtility::IsHDR(RenderTargetCube->GetFormat());
	const FIntPoint Size = { RenderTargetCube->SizeX, RenderTargetCube->SizeX };
	UTextureRenderTarget2D* RenderTarget = FGLTFTextureUtility::CreateRenderTarget(Size, bIsHDR);

	const float FaceRotation = FGLTFTextureUtility::GetCubeFaceRotation(CubeFace);
	FGLTFTextureUtility::RotateTexture(RenderTarget, FaceTexture, FVector2D::ZeroVector, Size, FaceRotation);

	if (bIsHDR)
	{
		JsonTexture.Encoding = Builder.GetTextureHDREncoding();
	}

	TArray<FColor> Pixels;
	if (!FGLTFTextureUtility::ReadPixels(RenderTarget, Pixels, JsonTexture.Encoding))
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Failed to read pixels (cube face %d) for cubemap render target %s"), CubeFace, *RenderTargetCube->GetName()));
		return;
	}

	const bool bIgnoreAlpha = FGLTFTextureUtility::IsAlphaless(RenderTargetCube->GetFormat());
	const ESKGLTFTextureType Type = bIsHDR ? ESKGLTFTextureType::HDR : ESKGLTFTextureType::None;

	JsonTexture.Source = Builder.AddImage(Pixels, Size, bIgnoreAlpha, Type, JsonTexture.Name);
	JsonTexture.Sampler = Builder.GetOrAddSampler(RenderTargetCube);
}

void FGLTFTextureLightMapTask::Complete()
{
	FGLTFJsonTexture& JsonTexture = Builder.GetTexture(TextureIndex);
	LightMap->GetName(JsonTexture.Name);

	// NOTE: export of lightmaps via source data is used to work around issues with
	// quality-loss due to incorrect gamma transformation when rendering to a canvas.

	FTextureSource& Source = const_cast<FTextureSource&>(LightMap->Source);
	if (!Source.IsValid())
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export lightmap texture %s because of missing source data"), *JsonTexture.Name));
		return;
	}

	const ETextureSourceFormat SourceFormat = Source.GetFormat();
	if (SourceFormat != TSF_BGRA8)
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Failed to export lightmap texture %s because of unsupported source format %s"), *JsonTexture.Name, *FGLTFNameUtility::GetName(SourceFormat)));
		return;
	}

	const FIntPoint Size = { Source.GetSizeX(), Source.GetSizeY() };
	const int64 ByteLength = Source.CalcMipSize(0);

	const bool bIgnoreAlpha = false;
	const ESKGLTFTextureType Type = ESKGLTFTextureType::Lightmaps;

	const void* RawData = Source.LockMip(0);
	JsonTexture.Source = Builder.AddImage(static_cast<const FColor*>(RawData), ByteLength, Size, bIgnoreAlpha, Type, JsonTexture.Name);
	Source.UnlockMip(0);

	JsonTexture.Sampler = Builder.GetOrAddSampler(LightMap);
}
