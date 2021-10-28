// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class SKGLTFEXPORTER_API FGLTFZipUtility
{
public:

	static TArray<FString> GetAllFiles(const FString& ArchiveFile);

	static bool ExtractAllFiles(const FString& ArchiveFile, const FString& TargetDirectory);
	static bool ExtractOneFile(const FString& ArchiveFile, const FString& FileToExtract, const FString& TargetDirectory);
	static int CompressDirectory(const FString& ArchiveFile, const FString& TargetDirectory);

private:

	static FString GetCurrentFilename(void* ZipHandle);

	static bool ExtractCurrentFile(void* ZipHandle, const FString& TargetFile);
};
