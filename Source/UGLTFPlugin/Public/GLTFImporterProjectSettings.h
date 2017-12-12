// Copyright 2017 Sketchfab, Inc. All Rights Reserved.

// Based on the USD and FBX Unreal Importers
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "GLTFPrimResolver.h"
#include "GLTFImporterProjectSettings.generated.h"

UCLASS(config=Editor, meta=(DisplayName=GLTFImporter))
class UGLTFImporterProjectSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category=General)
	TArray<FDirectoryPath> AdditionalPluginDirectories;

	/**
	 * Allows a custom class to be specified that will resolve UsdPrim objects in a usd file to actors and assets
	 */
	UPROPERTY(config, EditAnywhere, Category = General)
	TSubclassOf<UGLTFPrimResolver> CustomPrimResolver;
};