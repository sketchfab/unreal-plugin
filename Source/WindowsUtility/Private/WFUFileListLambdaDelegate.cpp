
#include "WindowsFileUtilityPrivatePCH.h"
#include "WindowsFileUtilityFunctionLibrary.h"
#include "WFUFileListLambdaDelegate.h"

UWFUFileListLambdaDelegate::UWFUFileListLambdaDelegate()
{
	OnDoneCallback = nullptr;
}

void UWFUFileListLambdaDelegate::SetOnDoneCallback(TFunction<void(const TArray<FString>&, const TArray<FString>&)> InOnDoneCallback)
{
	OnDoneCallback = InOnDoneCallback;
}

void UWFUFileListLambdaDelegate::OnListFileFound_Implementation(const FString& FileName, int32 ByteCount, const FString& FilePath)
{

}

void UWFUFileListLambdaDelegate::OnListDirectoryFound_Implementation(const FString& DirectoryName, const FString& FilePath)
{

}

void UWFUFileListLambdaDelegate::OnListDone_Implementation(const FString& DirectoryPath, const TArray<FString>& Files, const TArray<FString>& Folders)
{
	if (OnDoneCallback != nullptr)
	{
		OnDoneCallback(Files, Folders);
	}
}