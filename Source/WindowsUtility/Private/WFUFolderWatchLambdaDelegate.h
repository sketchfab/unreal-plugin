
#pragma once

#include "Object.h"
#include "WFUFolderWatchLambdaDelegate.generated.h"

UCLASS()
class WINDOWSFILEUTILITY_API UWFUFolderWatchLambdaDelegate : public UObject, public IWFUFolderWatchInterface
{
	GENERATED_BODY()

	UWFUFolderWatchLambdaDelegate();
public:
	void SetOnFileChangedCallback(TFunction<void(FString, FString)> InOnFileChangedCallback);

protected:
	TFunction<void(FString, FString)> OnFileChangedCallback;
	
	//IWFUFolderWatchInterface
	virtual void OnFileChanged_Implementation(const FString& FileName, const FString& FilePath) override;
	virtual void OnDirectoryChanged_Implementation(const FString& DirectoryName, const FString& DirectoryPath) override;
};