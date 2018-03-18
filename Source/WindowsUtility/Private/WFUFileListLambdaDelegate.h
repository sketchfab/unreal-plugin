
#pragma once

#include "Object.h"
#include "WFUFileListLambdaDelegate.generated.h"

UCLASS()
class WINDOWSFILEUTILITY_API UWFUFileListLambdaDelegate : public UObject, public IWFUFileListInterface
{
	GENERATED_BODY()

	UWFUFileListLambdaDelegate();
public:
	void SetOnDoneCallback(TFunction<void(const TArray<FString>&, const TArray<FString>&)> InOnDoneCallback);

protected:
	//File List Interface

	virtual void OnListFileFound_Implementation(const FString& FileName, int32 ByteCount, const FString& FilePath) override;
	virtual void OnListDirectoryFound_Implementation(const FString& DirectoryName, const FString& FilePath) override;
	virtual void OnListDone_Implementation(const FString& DirectoryPath, const TArray<FString>& Files, const TArray<FString>& Folders) override;

	TFunction<void(const TArray<FString>&, const TArray<FString>&)> OnDoneCallback;
};