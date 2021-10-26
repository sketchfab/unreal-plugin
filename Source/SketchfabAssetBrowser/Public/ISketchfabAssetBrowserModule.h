// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class ISketchfabAssetBrowserModule : public IModuleInterface
{

public:
	static inline ISketchfabAssetBrowserModule& Get()
	{
		return FModuleManager::LoadModuleChecked< ISketchfabAssetBrowserModule >( "SketchfabAssetBrowser" );
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "SketchfabAssetBrowser" );
	}

	virtual class USketchfabAssetBrowser *GetAssetBrowser() = 0;
	virtual class USketchfabExporter     *GetExporter() = 0;
	FString LicenseInfo;
	FString CurrentModelName;
};

