// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ZipUtilityInterface.generated.h"

UENUM(BlueprintType)
enum EZipUtilityCompletionState
{
	SUCCESS,
	FAILURE_NOT_FOUND,
	FAILURE_UNKNOWN
};


UINTERFACE(MinimalAPI)
class UZipUtilityInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class ZIPUTILITY_API IZipUtilityInterface
{
	GENERATED_IINTERFACE_BODY()

public:

	/**
	* Called during process as it completes. Currently updates on per file progress.
	* @param percentage - percentage done
	*/
	UFUNCTION(BlueprintNativeEvent, Category = ZipUtilityProgressEvents)
		void OnProgress(const FString& archive, float percentage, int32 bytes);

	/**
	* Called when whole process is complete (e.g. unzipping completed on archive)
	*/
	UFUNCTION(BlueprintNativeEvent, Category = ZipUtilityProgressEvents)
		void OnDone(const FString& archive, EZipUtilityCompletionState CompletionState);

	/**
	* Called at beginning of process (NB this only supports providing size information for up to 2gb) TODO: fix 32bit BP size issue
	*/
	UFUNCTION(BlueprintNativeEvent, Category = ZipUtilityProgressEvents)
		void OnStartProcess(const FString& archive, int32 bytes);

	/**
	* Called when file process is complete
	* @param path - path of the file that finished
	*/
	UFUNCTION(BlueprintNativeEvent, Category = ZipUtilityProgressEvents)
		void OnFileDone(const FString& archive, const FString& file);


	/**
	* Called when a file is found in the archive (e.g. listing the entries in the archive)
	* @param path - path of file
	* @param size - compressed size
	*/
	UFUNCTION(BlueprintNativeEvent, Category = ZipUtilityListEvents)
		void OnFileFound(const FString& archive, const FString& file, int32 size);
};