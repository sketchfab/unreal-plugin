
#pragma once

#include "Object.h"
#include "ZULambdaDelegate.generated.h"

UCLASS()
class ZIPUTILITY_API UZULambdaDelegate : public UObject, public IZipUtilityInterface
{
	GENERATED_BODY()

	UZULambdaDelegate();
public:
	void SetOnDoneCallback(TFunction<void()> InOnDoneCallback);
	void SetOnProgessCallback(TFunction<void(float)> InOnProgressCallback);

protected:
	//Zip utility interface
	virtual void OnProgress_Implementation(const FString& archive, float percentage, int32 bytes) override;
	virtual void OnDone_Implementation(const FString& archive, EZipUtilityCompletionState CompletionState) override;
	virtual void OnStartProcess_Implementation(const FString& archive, int32 bytes) override;
	virtual void OnFileDone_Implementation(const FString& archive, const FString& file) override;
	virtual void OnFileFound_Implementation(const FString& archive, const FString& file, int32 size) override;

	TFunction<void()> OnDoneCallback;
	TFunction<void(float)> OnProgressCallback;
};