// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Json/SKGLTFJsonEnums.h"

struct FGLTFFileUtility
{
	static FString GetPluginDir();

	static const TCHAR* GetFileExtension(ESKGLTFJsonMimeType MimeType);

	static FString GetUniqueFilename(const FString& BaseFilename, const FString& FileExtension, const TSet<FString>& UniqueFilenames);

	static bool IsGlbFile(const FString& Filename);

	static bool ReadJsonFile(const FString& FilePath, TSharedPtr<FJsonObject>& JsonObject);
	static bool WriteJsonFile(const FString& FilePath, const TSharedRef<FJsonObject>& JsonObject);

	static bool SetExecutable(const TCHAR* Filename, bool bIsExecutable);
};
