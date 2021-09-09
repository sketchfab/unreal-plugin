// Copyright Epic Games, Inc. All Rights Reserved.

#include "Builders/SKGLTFFileUtility.h"
#include "SKGLTFExporterModule.h"
#include "Interfaces/IPluginManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"

#if PLATFORM_LINUX || PLATFORM_MAC
// TODO: uncomment when GLTFLaunchHelper added for linux and macos
// #include <sys/stat.h>
#endif

FString FGLTFFileUtility::GetPluginDir()
{
	return IPluginManager::Get().FindPlugin(SKGLTFEXPORTER_MODULE_NAME)->GetBaseDir();
}

const TCHAR* FGLTFFileUtility::GetFileExtension(ESKGLTFJsonMimeType MimeType)
{
	switch (MimeType)
	{
		case ESKGLTFJsonMimeType::PNG:  return TEXT(".png");
		case ESKGLTFJsonMimeType::JPEG: return TEXT(".jpg");
		default:
			checkNoEntry();
			return TEXT("");
	}
}

FString FGLTFFileUtility::GetUniqueFilename(const FString& BaseFilename, const FString& FileExtension, const TSet<FString>& UniqueFilenames)
{
	FString Filename = BaseFilename + FileExtension;
	if (!UniqueFilenames.Contains(Filename))
	{
		return Filename;
	}

	int32 Suffix = 1;
	do
	{
		Filename = BaseFilename + TEXT('_') + FString::FromInt(Suffix) + FileExtension;
		Suffix++;
	}
	while (UniqueFilenames.Contains(Filename));

	return Filename;
}

bool FGLTFFileUtility::IsGlbFile(const FString& Filename)
{
	return FPaths::GetExtension(Filename).Equals(TEXT("glb"), ESearchCase::IgnoreCase);
}

bool FGLTFFileUtility::ReadJsonFile(const FString& FilePath, TSharedPtr<FJsonObject>& JsonObject)
{
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
	{
		return false;
	}

	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	return FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid();
}

bool FGLTFFileUtility::WriteJsonFile(const FString& FilePath, const TSharedRef<FJsonObject>& JsonObject)
{
	FString JsonContent;
	const TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(&JsonContent);

	if (!FJsonSerializer::Serialize(JsonObject, JsonWriter))
	{
		return false;
	}

	return FFileHelper::SaveStringToFile(JsonContent, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8);
}

bool FGLTFFileUtility::SetExecutable(const TCHAR* Filename, bool bIsExecutable)
{
/* TODO: uncomment when GLTFLaunchHelper added for linux and macos
#if PLATFORM_LINUX || PLATFORM_MAC
	struct stat FileInfo;
	FTCHARToUTF8 FilenameUtf8(Filename);
	bool bSuccess = false;
	// Set executable permission bit
	if (stat(FilenameUtf8.Get(), &FileInfo) == 0)
	{
		mode_t ExeFlags = S_IXUSR | S_IXGRP | S_IXOTH;
		FileInfo.st_mode = bIsExecutable ? (FileInfo.st_mode | ExeFlags) : (FileInfo.st_mode & ~ExeFlags);
		bSuccess = chmod(FilenameUtf8.Get(), FileInfo.st_mode) == 0;
	}
	return bSuccess;
#else
	return true;
#endif
*/
	return true;
}
