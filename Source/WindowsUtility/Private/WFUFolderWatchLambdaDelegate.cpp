
#include "WindowsFileUtilityPrivatePCH.h"
#include "WindowsFileUtilityFunctionLibrary.h"
#include "WFUFolderWatchLambdaDelegate.h"

UWFUFolderWatchLambdaDelegate::UWFUFolderWatchLambdaDelegate()
{
	OnFileChangedCallback = nullptr;
}


void UWFUFolderWatchLambdaDelegate::SetOnFileChangedCallback(TFunction<void(FString, FString)> InOnFileChangedCallback)
{
	OnFileChangedCallback = InOnFileChangedCallback;
}


void UWFUFolderWatchLambdaDelegate::OnFileChanged_Implementation(const FString& FileName, const FString& FilePath)
{
	if (OnFileChangedCallback != nullptr)
	{
		OnFileChangedCallback(FileName, FilePath);
	}
}

void UWFUFolderWatchLambdaDelegate::OnDirectoryChanged_Implementation(const FString& DirectoryName, const FString& DirectoryPath)
{
	if (OnFileChangedCallback != nullptr)
	{
		OnFileChangedCallback(DirectoryName, DirectoryPath);
	}
}