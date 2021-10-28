// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Builders/SKGLTFBufferBuilder.h"
#include "Builders/SKGLTFBinaryHashKey.h"

class FGLTFImageBuilder : public FGLTFBufferBuilder
{
protected:

	FGLTFImageBuilder(const FString& FilePath, const USKGLTFExportOptions* ExportOptions);

public:

	FGLTFJsonImageIndex AddImage(const TArray<FColor>& Pixels, FIntPoint Size, bool bIgnoreAlpha, ESKGLTFTextureType Type, const FString& Name);
	FGLTFJsonImageIndex AddImage(const FColor* Pixels, int64 ByteLength, FIntPoint Size, bool bIgnoreAlpha, ESKGLTFTextureType Type, const FString& Name);

private:

	FGLTFJsonImageIndex AddImage(const FColor* Pixels, FIntPoint Size, bool bIgnoreAlpha, ESKGLTFTextureType Type, const FString& Name);
	FGLTFJsonImageIndex AddImage(const void* CompressedData, int64 CompressedByteLength, ESKGLTFJsonMimeType MimeType, const FString& Name);

	ESKGLTFJsonMimeType GetImageFormat(const FColor* Pixels, FIntPoint Size, bool bIgnoreAlpha, ESKGLTFTextureType Type) const;

	FString SaveImageToFile(const void* CompressedData, int64 CompressedByteLength, ESKGLTFJsonMimeType MimeType, const FString& Name);

	TSet<FString> UniqueImageUris;
	TMap<FGLTFBinaryHashKey, FGLTFJsonImageIndex> UniqueImageIndices;
};
