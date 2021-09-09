// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonEnums.h"
#include "Engine.h"

struct FGLTFTextureUtility
{
	static bool IsHDR(EPixelFormat Format);
	static bool IsAlphaless(EPixelFormat PixelFormat);

	static bool IsCubemap(const UTexture* Texture);

	static float GetCubeFaceRotation(ECubeFace CubeFace);

	static TextureFilter GetDefaultFilter(TextureGroup Group);

	static int32 GetMipBias(const UTexture* Texture);

	static FIntPoint GetInGameSize(const UTexture* Texture);

	static TTuple<TextureAddress, TextureAddress> GetAddressXY(const UTexture* Texture);

	static UTexture2D* CreateTransientTexture(const void* RawData, int64 ByteLength, const FIntPoint& Size, EPixelFormat Format, bool bSRGB = false);

	static UTextureRenderTarget2D* CreateRenderTarget(const FIntPoint& Size, bool bIsHDR);

	static bool DrawTexture(UTextureRenderTarget2D* OutTarget, const UTexture2D* InSource, const FVector2D& InPosition, const FVector2D& InSize, const FMatrix& InTransform = FMatrix::Identity);
	static bool RotateTexture(UTextureRenderTarget2D* OutTarget, const UTexture2D* InSource, const FVector2D& InPosition, const FVector2D& InSize, float InDegrees);

	static UTexture2D* CreateTextureFromCubeFace(const UTextureCube* TextureCube, ECubeFace CubeFace);
	static UTexture2D* CreateTextureFromCubeFace(const UTextureRenderTargetCube* RenderTargetCube, ECubeFace CubeFace);

	static bool ReadPixels(const UTextureRenderTarget2D* InRenderTarget, TArray<FColor>& OutPixels, ESKGLTFJsonHDREncoding Encoding);

	static void EncodeRGBM(const TArray<FLinearColor>& InPixels, TArray<FColor>& OutPixels, float MaxRange = 8);
	static void EncodeRGBE(const TArray<FLinearColor>& InPixels, TArray<FColor>& OutPixels);

	// TODO: maybe use template specialization to avoid the need for duplicated functions
	static bool LoadPlatformData(UTexture2D* Texture);
	static bool LoadPlatformData(UTextureCube* TextureCube);

	static void FlipGreenChannel(TArray<FColor>& Pixels);
};
