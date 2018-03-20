// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "WFUFileListInterface.generated.h"

UINTERFACE(MinimalAPI)
class UWFUFileListInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class WINDOWSFILEUTILITY_API IWFUFileListInterface
{
	GENERATED_IINTERFACE_BODY()

public:

	/**
	* Called when a file has been found inside the folder of choice
	* @param FileName of the found file.
	* @param Size in bytes of the found file.
	* @param FilePath of the file that was found
	*/
	UFUNCTION(BlueprintNativeEvent, Category = FolderWatchEvent)
	void OnListFileFound(const FString& FileName, int32 ByteCount, const FString& FilePath);

	/**
	* Called when a directory has been found inside the folder of choice
	* @param DirectoryName of the found directory.
	* @param FilePath of the file that was found
	*/
	UFUNCTION(BlueprintNativeEvent, Category = FolderWatchEvent)
	void OnListDirectoryFound(const FString& DirectoryName, const FString& FilePath);

	/**
	* Called when the listing operation has completed.
	* @param DirectoryPath Path of the directory
	* @param Files array of files found
	*/
	UFUNCTION(BlueprintNativeEvent, Category = FolderWatchEvent)
	void OnListDone(const FString& DirectoryPath, const TArray<FString>& Files, const TArray<FString>& Folders);
};