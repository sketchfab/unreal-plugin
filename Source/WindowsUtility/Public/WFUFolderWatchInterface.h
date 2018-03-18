// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "WFUFolderWatchInterface.generated.h"

UINTERFACE(MinimalAPI)
class UWFUFolderWatchInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class WINDOWSFILEUTILITY_API IWFUFolderWatchInterface
{
	GENERATED_IINTERFACE_BODY()

public:

	/**
	* Called when a file inside the folder has changed
	* @param FilePath Path of the file that has changed
	*/
	UFUNCTION(BlueprintNativeEvent, Category = FolderWatchEvent)
	void OnFileChanged(const FString& FileName, const FString& FilePath);

	/**
	* Called when a directory inside the folder has changed
	* @param FilePath Path of the file that has changed
	*/
	UFUNCTION(BlueprintNativeEvent, Category = FolderWatchEvent)
	void OnDirectoryChanged(const FString& DirectoryName, const FString& DirectoryPath);
};