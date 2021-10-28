// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFTextureUtility.h"
#include "NormalMapPreview.h"

bool FGLTFTextureUtility::IsHDR(EPixelFormat Format)
{
	switch (Format)
	{
		case PF_A32B32G32R32F:
		case PF_FloatRGB:
		case PF_FloatRGBA:
		case PF_R32_FLOAT:
		case PF_G16R16F:
		case PF_G16R16F_FILTER:
		case PF_G32R32F:
		case PF_R16F:
		case PF_R16F_FILTER:
		case PF_FloatR11G11B10:
		case PF_BC6H:
		case PF_PLATFORM_HDR_0:
		case PF_PLATFORM_HDR_1:
		case PF_PLATFORM_HDR_2:
			return true;
		default:
			return false;
	}
}

bool FGLTFTextureUtility::IsAlphaless(EPixelFormat PixelFormat)
{
	switch (PixelFormat)
	{
		case PF_ATC_RGB:
		case PF_BC4:
		case PF_BC5:
		case PF_DXT1:
		case PF_ETC1:
		case PF_ETC2_RGB:
		case PF_FloatR11G11B10:
		case PF_FloatRGB:
		case PF_R5G6B5_UNORM:
			// TODO: add more pixel formats that don't support alpha, but beware of formats like PF_G8 (that still seem to return alpha in some cases)
			return true;
		default:
			return false;
	}
}

bool FGLTFTextureUtility::IsCubemap(const UTexture* Texture)
{
	return Texture->IsA<UTextureCube>() || Texture->IsA<UTextureRenderTargetCube>();
}

float FGLTFTextureUtility::GetCubeFaceRotation(ECubeFace CubeFace)
{
	switch (CubeFace)
	{
		case CubeFace_PosX: return 90;
		case CubeFace_NegX: return -90;
		case CubeFace_PosY: return 180;
		case CubeFace_NegY: return 0;
		case CubeFace_PosZ: return 180;
		case CubeFace_NegZ: return 0;
		default:            return 0;
	}
}

TextureFilter FGLTFTextureUtility::GetDefaultFilter(TextureGroup LODGroup)
{
	const UTextureLODSettings* TextureLODSettings = UDeviceProfileManager::Get().GetActiveProfile()->GetTextureLODSettings();
	const ETextureSamplerFilter Filter = TextureLODSettings->GetTextureLODGroup(LODGroup).Filter;

	switch (Filter)
	{
		case ETextureSamplerFilter::Point:             return TF_Nearest;
		case ETextureSamplerFilter::Bilinear:          return TF_Bilinear;
		case ETextureSamplerFilter::Trilinear:         return TF_Trilinear;
		case ETextureSamplerFilter::AnisotropicPoint:  return TF_Trilinear; // A lot of engine code doesn't result in nearest
		case ETextureSamplerFilter::AnisotropicLinear: return TF_Trilinear;
		default:                                       return TF_Default; // Let caller decide fallback
	}
}

int32 FGLTFTextureUtility::GetMipBias(const UTexture* Texture)
{
	if (const UTexture2D* Texture2D = Cast<UTexture2D>(Texture))
	{
		return Texture2D->GetNumMips() - Texture2D->GetNumMipsAllowed(true);
	}

	return Texture->GetCachedLODBias();
}

FIntPoint FGLTFTextureUtility::GetInGameSize(const UTexture* Texture)
{
	const int32 Width = Texture->GetSurfaceWidth();
	const int32 Height = Texture->GetSurfaceHeight();

	const int32 MipBias = GetMipBias(Texture);

	const int32 InGameWidth = FMath::Max(Width >> MipBias, 1);
	const int32 InGameHeight = FMath::Max(Height >> MipBias, 1);

	return { InGameWidth, InGameHeight };
}

TTuple<TextureAddress, TextureAddress> FGLTFTextureUtility::GetAddressXY(const UTexture* Texture)
{
	TextureAddress AddressX;
	TextureAddress AddressY;

	if (const UTexture2D* Texture2D = Cast<UTexture2D>(Texture))
	{
		AddressX = Texture2D->AddressX;
		AddressY = Texture2D->AddressY;
	}
	else if (const UTextureRenderTarget2D* RenderTarget2D = Cast<UTextureRenderTarget2D>(Texture))
	{
		AddressX = RenderTarget2D->AddressX;
		AddressY = RenderTarget2D->AddressY;
	}
	else
	{
		AddressX = TA_MAX;
		AddressY = TA_MAX;
	}

	return MakeTuple(AddressX, AddressY);
}

UTexture2D* FGLTFTextureUtility::CreateTransientTexture(const void* RawData, int64 ByteLength, const FIntPoint& Size, EPixelFormat Format, bool bSRGB)
{
	check(CalculateImageBytes(Size.X, Size.Y, 0, Format) == ByteLength);

	// TODO: do these temp textures need to be part of the root set to avoid garbage collection?
	UTexture2D* Texture = UTexture2D::CreateTransient(Size.X, Size.Y, Format);

	void* MipData = Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(MipData, RawData, ByteLength);
	Texture->PlatformData->Mips[0].BulkData.Unlock();

	Texture->SRGB = bSRGB ? 1 : 0;
	Texture->CompressionNone = true;
	Texture->MipGenSettings = TMGS_NoMipmaps;

	Texture->UpdateResource();
	return Texture;
}

UTextureRenderTarget2D* FGLTFTextureUtility::CreateRenderTarget(const FIntPoint& Size, bool bHDR)
{
	// TODO: instead of PF_FloatRGBA (i.e. RTF_RGBA16f) use PF_A32B32G32R32F (i.e. RTF_RGBA32f) to avoid accuracy loss
	const EPixelFormat PixelFormat = bHDR ? PF_FloatRGBA : PF_B8G8R8A8;

	// NOTE: both bForceLinearGamma and TargetGamma=2.2 seem necessary for exported images to match their source data.
	// It's not entirely clear why gamma must be 2.2 (instead of 0.0) and why bInForceLinearGamma must also be true.
	const bool bForceLinearGamma = true;
	const float TargetGamma = 2.2f;

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>();
	RenderTarget->InitCustomFormat(Size.X, Size.Y, PixelFormat, bForceLinearGamma);

	RenderTarget->TargetGamma = TargetGamma;
	return RenderTarget;
}

bool FGLTFTextureUtility::DrawTexture(UTextureRenderTarget2D* OutTarget, const UTexture2D* InSource, const FVector2D& InPosition, const FVector2D& InSize, const FMatrix& InTransform)
{
	FRenderTarget* RenderTarget = OutTarget->GameThread_GetRenderTargetResource();
	if (RenderTarget == nullptr)
	{
		return false;
	}

	// Fully stream in the texture before drawing it.
	const_cast<UTexture2D*>(InSource)->SetForceMipLevelsToBeResident(30.0f, true);
	const_cast<UTexture2D*>(InSource)->WaitForStreaming();

	TRefCountPtr<FBatchedElementParameters> BatchedElementParameters;

	if (InSource->IsNormalMap())
	{
		BatchedElementParameters = new FNormalMapBatchedElementParameters();
	}

	FCanvas Canvas(RenderTarget, nullptr, 0.0f, 0.0f, 0.0f, GMaxRHIFeatureLevel);
	FCanvasTileItem TileItem(InPosition, InSource->Resource, InSize, FLinearColor::White);
	TileItem.BatchedElementParameters = BatchedElementParameters;

	Canvas.PushAbsoluteTransform(InTransform);
	TileItem.Draw(&Canvas);
	Canvas.PopTransform();

	Canvas.Flush_GameThread();
	FlushRenderingCommands();
	Canvas.SetRenderTarget_GameThread(nullptr);
	FlushRenderingCommands();

	return true;
}

bool FGLTFTextureUtility::RotateTexture(UTextureRenderTarget2D* OutTarget, const UTexture2D* InSource, const FVector2D& InPosition, const FVector2D& InSize, float InDegrees)
{
	FMatrix Transform = FMatrix::Identity;
	if (InDegrees != 0)
	{
		const FVector Center = FVector(InSize.X / 2.0f, InSize.Y / 2.0f, 0);
		Transform = FTranslationMatrix(-Center) * FRotationMatrix({ 0, InDegrees, 0 }) * FTranslationMatrix(Center);
	}

	return DrawTexture(OutTarget, InSource, InPosition, InSize, Transform);
}

UTexture2D* FGLTFTextureUtility::CreateTextureFromCubeFace(const UTextureCube* TextureCube, ECubeFace CubeFace)
{
	const FIntPoint Size(TextureCube->GetSizeX(), TextureCube->GetSizeY());
	const EPixelFormat Format = TextureCube->GetPixelFormat();

	if (!LoadPlatformData(const_cast<UTextureCube*>(TextureCube)))
	{
		return nullptr;
	}

	const FByteBulkData& BulkData = TextureCube->PlatformData->Mips[0].BulkData;
	const int64 MipSize = BulkData.GetBulkDataSize() / 6;

	const void* MipDataPtr = BulkData.LockReadOnly();
	const void* FaceDataPtr =  static_cast<const uint8*>(MipDataPtr) + MipSize * CubeFace;
	UTexture2D* FaceTexture = CreateTransientTexture(FaceDataPtr, MipSize, Size, Format, TextureCube->SRGB);
	BulkData.Unlock();

	return FaceTexture;
}

UTexture2D* FGLTFTextureUtility::CreateTextureFromCubeFace(const UTextureRenderTargetCube* RenderTargetCube, ECubeFace CubeFace)
{
	FTextureRenderTargetCubeResource* Resource = static_cast<FTextureRenderTargetCubeResource*>(RenderTargetCube->Resource);
	if (Resource == nullptr)
	{
		return nullptr;
	}

	const FIntPoint Size(RenderTargetCube->SizeX, RenderTargetCube->SizeX);
	UTexture2D* FaceTexture;

	if (IsHDR(RenderTargetCube->GetFormat()))
	{
		TArray<FFloat16Color> Pixels;
		if (!Resource->ReadPixels(Pixels, FReadSurfaceDataFlags(RCM_UNorm, CubeFace)))
		{
			return nullptr;
		}

		FaceTexture = CreateTransientTexture(Pixels.GetData(), Pixels.Num() * sizeof(FFloat16Color), Size, PF_FloatRGBA, false);
	}
	else
	{
		TArray<FColor> Pixels;
		if (!Resource->ReadPixels(Pixels, FReadSurfaceDataFlags(RCM_UNorm, CubeFace)))
		{
			return nullptr;
		}

		FaceTexture = CreateTransientTexture(Pixels.GetData(), Pixels.Num() * sizeof(FColor), Size, PF_B8G8R8A8, false);
	}

	return FaceTexture;
}

bool FGLTFTextureUtility::ReadPixels(const UTextureRenderTarget2D* InRenderTarget, TArray<FColor>& OutPixels, ESKGLTFJsonHDREncoding Encoding)
{
	FTextureRenderTarget2DResource* Resource = static_cast<FTextureRenderTarget2DResource*>(InRenderTarget->Resource);
	if (Resource == nullptr)
	{
		return false;
	}

	if (Encoding == ESKGLTFJsonHDREncoding::None)
	{
		return Resource->ReadPixels(OutPixels);
	}

	TArray<FLinearColor> HDRPixels;
	if (!Resource->ReadLinearColorPixels(HDRPixels))
	{
		return false;
	}

	switch (Encoding)
	{
		case ESKGLTFJsonHDREncoding::RGBM:
			EncodeRGBM(HDRPixels, OutPixels);
			break;
		case ESKGLTFJsonHDREncoding::RGBE:
			EncodeRGBE(HDRPixels, OutPixels);
			break;
		default:
			checkNoEntry();
			break;
	}

	return true;
}

void FGLTFTextureUtility::EncodeRGBM(const TArray<FLinearColor>& InPixels, TArray<FColor>& OutPixels, float MaxRange)
{
	OutPixels.AddUninitialized(InPixels.Num());

	for (int32 Index = 0; Index < InPixels.Num(); ++Index)
	{
		const FLinearColor& Color = InPixels[Index];
		FLinearColor RGBM;

		RGBM.R = FMath::Sqrt(Color.R);
		RGBM.G = FMath::Sqrt(Color.G);
		RGBM.B = FMath::Sqrt(Color.B);

		RGBM.R /= MaxRange;
		RGBM.G /= MaxRange;
		RGBM.B /= MaxRange;

		RGBM.A = FMath::Max(FMath::Max(RGBM.R, RGBM.G), FMath::Max(RGBM.B, 1.0f / 255.0f));
		RGBM.A = FMath::CeilToFloat(RGBM.A * 255.0f) / 255.0f;

		RGBM.R /= RGBM.A;
		RGBM.G /= RGBM.A;
		RGBM.B /= RGBM.A;

		OutPixels[Index] = RGBM.ToFColor(false);
	}
}

void FGLTFTextureUtility::EncodeRGBE(const TArray<FLinearColor>& InPixels, TArray<FColor>& OutPixels)
{
	OutPixels.AddUninitialized(InPixels.Num());

	for (int32 Index = 0; Index < InPixels.Num(); ++Index)
	{
		OutPixels[Index] = InPixels[Index].ToRGBE();
	}
}

bool FGLTFTextureUtility::LoadPlatformData(UTexture2D* Texture)
{
	if (Texture->PlatformData == nullptr || Texture->PlatformData->Mips.Num() == 0)
	{
		return false;
	}

	if (Texture->PlatformData->Mips[0].BulkData.GetBulkDataSize() == 0)
	{
		// TODO: is this correct handling?
		Texture->ForceRebuildPlatformData();
		if (Texture->PlatformData->Mips[0].BulkData.GetBulkDataSize() == 0)
		{
			return false;
		}
	}

	return true;
}

bool FGLTFTextureUtility::LoadPlatformData(UTextureCube* TextureCube)
{
	if (TextureCube->PlatformData == nullptr || TextureCube->PlatformData->Mips.Num() == 0)
	{
		return false;
	}

	if (TextureCube->PlatformData->Mips[0].BulkData.GetBulkDataSize() == 0)
	{
		// TODO: is this correct handling?
		TextureCube->ForceRebuildPlatformData();
		if (TextureCube->PlatformData->Mips[0].BulkData.GetBulkDataSize() == 0)
		{
			return false;
		}
	}

	return true;
}

void FGLTFTextureUtility::FlipGreenChannel(TArray<FColor>& Pixels)
{
	for (FColor& Pixel: Pixels)
	{
		Pixel.G = 255 - Pixel.G;
	}
}
