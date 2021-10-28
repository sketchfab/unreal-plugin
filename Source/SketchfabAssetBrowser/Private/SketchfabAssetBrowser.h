// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "ISketchfabAssetBrowser.h"
#include "SketchfabAssetBrowser.generated.h"

UCLASS(transient)
class USketchfabWindow : public UObject
{
	GENERATED_UCLASS_BODY()
public:
	bool ShowWindow();
	void OnWindowClosed(const TSharedRef<SWindow>& InWindow);
	virtual TSharedPtr<SWindow> CreateWindow() { return nullptr; };

protected:
	FString WindowTitle;
	int MinWidth, MinHeight;
	int DefaultWidth, DefaultHeight;
	TSharedPtr<SWindow> WindowPtr;
};

DECLARE_LOG_CATEGORY_EXTERN(LogSketchfabAssetBrowser, Log, All);
UCLASS(transient)
class USketchfabAssetBrowser : public USketchfabWindow {
	GENERATED_UCLASS_BODY()
public:
	TSharedPtr<SWindow> CreateWindow();
};

DECLARE_LOG_CATEGORY_EXTERN(LogSketchfabExporter, Log, All);
UCLASS(transient)
class USketchfabExporter : public USketchfabWindow
{
	GENERATED_UCLASS_BODY()
public:
	TSharedPtr<SWindow> CreateWindow();
};
