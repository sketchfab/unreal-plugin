// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Builders/GLTFBufferBuilder.h"
#include "Builders/GLTFBinaryHashKey.h"

class FGLTFImageBuilder : public FGLTFBufferBuilder
{
protected:

	FGLTFImageBuilder(const FString& FilePath, const UGLTFExportOptions* ExportOptions);

public:

	FGLTFJsonImageIndex AddImage(const TArray<FColor>& Pixels, FIntPoint Size, bool bIgnoreAlpha, EGLTFTextureType Type, const FString& Name);
	FGLTFJsonImageIndex AddImage(const FColor* Pixels, int64 ByteLength, FIntPoint Size, bool bIgnoreAlpha, EGLTFTextureType Type, const FString& Name);

private:

	FGLTFJsonImageIndex AddImage(const FColor* Pixels, FIntPoint Size, bool bIgnoreAlpha, EGLTFTextureType Type, const FString& Name);
	FGLTFJsonImageIndex AddImage(const void* CompressedData, int64 CompressedByteLength, EGLTFJsonMimeType MimeType, const FString& Name);

	EGLTFJsonMimeType GetImageFormat(const FColor* Pixels, FIntPoint Size, bool bIgnoreAlpha, EGLTFTextureType Type) const;

	FString SaveImageToFile(const void* CompressedData, int64 CompressedByteLength, EGLTFJsonMimeType MimeType, const FString& Name);

	TSet<FString> UniqueImageUris;
	TMap<FGLTFBinaryHashKey, FGLTFJsonImageIndex> UniqueImageIndices;
};
