// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "Factories/Factory.h"
#include "SKGLTFImporter.h"
#include "Factories/ImportSettings.h"
//#include "ZipUtilityInterface.h"

#include "SKGLTFAssetImportFactory.generated.h"


USTRUCT()
struct FGLTFAssetImportContext : public FGLTFImportContext
{
	GENERATED_USTRUCT_BODY()

		virtual void Init(UObject* InParent, const FString& InName, const FString &InBasePath, class tinygltf::Model* InModel);
};

UCLASS(transient)
class USKGLTFAssetImportFactory : public UFactory, public IImportSettingsParser//, public IZipUtilityInterface
{
	GENERATED_UCLASS_BODY()

public:
	/** UFactory interface */
	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;
	virtual bool FactoryCanImport(const FString& Filename) override;
	virtual void CleanUp() override;
	virtual IImportSettingsParser* GetImportSettingsParser() override { return this; }

	/** IImportSettingsParser interface */
	virtual void ParseFromJson(TSharedRef<class FJsonObject> ImportSettingsJson) override;

private:
	UPROPERTY()
	FGLTFAssetImportContext ImportContext;

	UPROPERTY()
	USKGLTFImportOptions_SKETCHFAB* ImportOptions;


public:
	/*
	virtual void OnProgress_Implementation(const FString& archive, float percentage, int32 bytes) override;
	virtual void OnDone_Implementation(const FString& archive, EZipUtilityCompletionState CompletionState) override;
	virtual void OnStartProcess_Implementation(const FString& archive, int32 bytes) override;
	virtual void OnFileDone_Implementation(const FString& archive, const FString& file) override;
	virtual void OnFileFound_Implementation(const FString& archive, const FString& file, int32 size) override;
	*/

	FString ZipFileName;
	bool bFoundGLTFInZip;
	bool bZipDone;

};
