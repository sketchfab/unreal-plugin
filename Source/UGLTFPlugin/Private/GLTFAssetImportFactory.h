// Copyright 2017 Sketchfab, Inc. All Rights Reserved.

// Based on the USD and FBX Unreal Importers
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "Factories/Factory.h"
#include "GLTFImporter.h"
#include "Factories/ImportSettings.h"

#include "GLTFAssetImportFactory.generated.h"

USTRUCT()
struct FGLTFAssetImportContext : public FGltfImportContext
{
	GENERATED_USTRUCT_BODY()

		virtual void Init(UObject* InParent, const FString& InName, const FString &InBasePath, class tinygltf::Model* InModel);
};

UCLASS(transient)
class UGLTFAssetImportFactory : public UFactory, public IImportSettingsParser
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
	UGLTFImportOptions* ImportOptions;
};
