// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "UObject/Class.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/Package.h"
#include "UObject/ObjectRedirector.h"
#include "Misc/PackageName.h"
#include "UObject/LinkerLoad.h"
#include "PrimaryAssetId.h"

#include "SketchfabAssetData.generated.h"

ASSETREGISTRY_API DECLARE_LOG_CATEGORY_EXTERN(LogSketchfabAssetData, Log, All);

USTRUCT()
struct FSketchfabAssetData
{
	GENERATED_BODY()
public:
	FName ModelUID;
	FName ContentFolder;
	FName AssetName;
	FName ThumbUID;
	float DownloadProgress;

public:
	/** Default constructor */
	FSketchfabAssetData()
	{}

	/** Constructor */
	FSketchfabAssetData(FName InContentFolder, FName InAssetName, FName InModelUID, FName InThumbUID)
		: ContentFolder(InContentFolder)
		, AssetName(InAssetName)
		, ModelUID(InModelUID)
		, ThumbUID(InThumbUID)
		, DownloadProgress(0)
	{
	}

	/** FAssetDatas are equal if their object paths match */
	bool operator==(const FSketchfabAssetData& Other) const
	{
		return ModelUID == Other.ModelUID;
	}

	bool operator!=(const FSketchfabAssetData& Other) const
	{
		return ModelUID != Other.ModelUID;
	}

	bool operator>(const FSketchfabAssetData& Other) const
	{
		return ModelUID > Other.ModelUID;
	}

	bool operator<(const FSketchfabAssetData& Other) const
	{
		return ModelUID < Other.ModelUID;
	}

	/** Checks to see if this AssetData refers to an asset or is NULL */
	bool IsValid() const
	{
		return ModelUID != NAME_None;
	}

	/** Returns the full name for the asset in the form: Class ObjectPath */
	FString GetFullName() const
	{
		FString FullName;
		GetFullName(FullName);
		return FullName;
	}

	void GetFullName(FString& OutFullName) const
	{
		OutFullName.Reset();
		ContentFolder.AppendString(OutFullName);
		ModelUID.AppendString(OutFullName);
	}

	/** Returns true if the asset is loaded */
	bool IsAssetLoaded() const
	{
		return IsValid() && FindObject<UObject>(NULL, *ModelUID.ToString()) != NULL;
	}
};