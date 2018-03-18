
#include "ZipUtilityPrivatePCH.h"
#include "WindowsFileUtilityFunctionLibrary.h"
#include "ZipFileFunctionLibrary.h"
#include "ZULambdaDelegate.h"

UZULambdaDelegate::UZULambdaDelegate()
{
	OnDoneCallback = nullptr;
	OnProgressCallback = nullptr;
}

void UZULambdaDelegate::SetOnDoneCallback(TFunction<void()> InOnDoneCallback)
{
	OnDoneCallback = InOnDoneCallback;
}

void UZULambdaDelegate::SetOnProgessCallback(TFunction<void(float)> InOnProgressCallback)
{
	OnProgressCallback = InOnProgressCallback;
}

void UZULambdaDelegate::OnProgress_Implementation(const FString& archive, float percentage, int32 bytes)
{
	if (OnProgressCallback != nullptr)
	{
		OnProgressCallback(percentage);
	}
}

void UZULambdaDelegate::OnDone_Implementation(const FString& archive, EZipUtilityCompletionState CompletionState)
{
	if (OnDoneCallback != nullptr)
	{
		OnDoneCallback();
	}
}

void UZULambdaDelegate::OnStartProcess_Implementation(const FString& archive, int32 bytes)
{

}

void UZULambdaDelegate::OnFileDone_Implementation(const FString& archive, const FString& file)
{

}

void UZULambdaDelegate::OnFileFound_Implementation(const FString& archive, const FString& file, int32 size)
{

}

