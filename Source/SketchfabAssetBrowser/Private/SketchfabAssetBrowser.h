// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "ISketchfabAssetBrowser.h"
#include "SketchfabAssetBrowser.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSketchfabAssetBrowser, Log, All);

UCLASS(transient)
class USketchfabAssetBrowser : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	bool ShowWindow();
	void OnWindowClosed(const TSharedRef<SWindow>& InWindow);

private:
	TSharedPtr<SWindow> SketchfabBrowserWindowPtr;
};


