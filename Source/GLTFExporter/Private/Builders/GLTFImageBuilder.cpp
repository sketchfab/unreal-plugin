// Copyright Epic Games, Inc. All Rights Reserved.

#include "Builders/SKGLTFImageBuilder.h"
#include "Builders/SKGLTFFileUtility.h"
#include "Builders/SKGLTFImageUtility.h"
#include "Misc/FileHelper.h"

FGLTFImageBuilder::FGLTFImageBuilder(const FString& FilePath, const USKGLTFExportOptions* ExportOptions)
	: FGLTFBufferBuilder(FilePath, ExportOptions)
{
}

FGLTFJsonImageIndex FGLTFImageBuilder::AddImage(const TArray<FColor>& Pixels, FIntPoint Size, bool bIgnoreAlpha, ESKGLTFTextureType Type, const FString& Name)
{
	check(Pixels.Num() == Size.X * Size.Y);
	return AddImage(Pixels.GetData(), Size, bIgnoreAlpha, Type, Name);
}

FGLTFJsonImageIndex FGLTFImageBuilder::AddImage(const FColor* Pixels, int64 ByteLength, FIntPoint Size, bool bIgnoreAlpha, ESKGLTFTextureType Type, const FString& Name)
{
	check(ByteLength == Size.X * Size.Y * sizeof(FColor));
	return AddImage(Pixels, Size, bIgnoreAlpha, Type, Name);
}

FGLTFJsonImageIndex FGLTFImageBuilder::AddImage(const FColor* Pixels, FIntPoint Size, bool bIgnoreAlpha, ESKGLTFTextureType Type, const FString& Name)
{
	TArray64<uint8> CompressedData;

	const ESKGLTFJsonMimeType ImageFormat = GetImageFormat(Pixels, Size, bIgnoreAlpha, Type);
	switch (ImageFormat)
	{
		case ESKGLTFJsonMimeType::None:
			return FGLTFJsonImageIndex(INDEX_NONE);

		case ESKGLTFJsonMimeType::PNG:
			FGLTFImageUtility::CompressToPNG(Pixels, Size, CompressedData);
			break;

		case ESKGLTFJsonMimeType::JPEG:
			FGLTFImageUtility::CompressToJPEG(Pixels, Size, ExportOptions->TextureImageQuality, CompressedData);
			break;

		default:
			checkNoEntry();
			break;
	}

	return AddImage(CompressedData.GetData(), CompressedData.Num(), ImageFormat, Name);
}

FGLTFJsonImageIndex FGLTFImageBuilder::AddImage(const void* CompressedData, int64 CompressedByteLength, ESKGLTFJsonMimeType MimeType, const FString& Name)
{
	// TODO: should this function be renamed to GetOrAddImage?

	const FGLTFBinaryHashKey HashKey(CompressedData, CompressedByteLength);
	FGLTFJsonImageIndex& ImageIndex = UniqueImageIndices.FindOrAdd(HashKey);

	if (ImageIndex == INDEX_NONE)
	{
		FGLTFJsonImage JsonImage;

		if (bIsGlbFile)
		{
			JsonImage.Name = Name;
			JsonImage.MimeType = MimeType;
			JsonImage.BufferView = AddBufferView(CompressedData, CompressedByteLength);
		}
		else
		{
			JsonImage.Uri = SaveImageToFile(CompressedData, CompressedByteLength, MimeType, Name);
		}

		ImageIndex = FGLTFJsonBuilder::AddImage(JsonImage);
	}

	return ImageIndex;
}

ESKGLTFJsonMimeType FGLTFImageBuilder::GetImageFormat(const FColor* Pixels, FIntPoint Size, bool bIgnoreAlpha, ESKGLTFTextureType Type) const
{
	switch (ExportOptions->TextureImageFormat)
	{
		case ESKGLTFTextureImageFormat::None:
			return ESKGLTFJsonMimeType::None;

		case ESKGLTFTextureImageFormat::PNG:
			return ESKGLTFJsonMimeType::PNG;

		case ESKGLTFTextureImageFormat::JPEG:
			return
				!EnumHasAllFlags(static_cast<ESKGLTFTextureType>(ExportOptions->NoLossyImageFormatFor), Type) &&
				(bIgnoreAlpha || FGLTFImageUtility::NoAlphaNeeded(Pixels, Size)) ?
				ESKGLTFJsonMimeType::JPEG : ESKGLTFJsonMimeType::PNG;

		default:
			checkNoEntry();
			return ESKGLTFJsonMimeType::None;
	}
}

FString FGLTFImageBuilder::SaveImageToFile(const void* CompressedData, int64 CompressedByteLength, ESKGLTFJsonMimeType MimeType, const FString& Name)
{
	const TCHAR* Extension = FGLTFFileUtility::GetFileExtension(MimeType);
	const FString ImageUri = FGLTFFileUtility::GetUniqueFilename(Name, Extension, UniqueImageUris);

	const TArrayView<const uint8> ImageData(static_cast<const uint8*>(CompressedData), CompressedByteLength);
	const FString ImagePath = FPaths::Combine(DirPath, ImageUri);

	if (!FFileHelper::SaveArrayToFile(ImageData, *ImagePath))
	{
		AddErrorMessage(FString::Printf(TEXT("Failed to save image %s to file: %s"), *Name, *ImagePath));
		return TEXT("");
	}

	UniqueImageUris.Add(ImageUri);
	return ImageUri;
}
