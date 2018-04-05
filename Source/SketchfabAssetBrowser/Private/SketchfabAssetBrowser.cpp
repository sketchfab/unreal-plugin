// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SketchfabAssetBrowser.h"
#include "ModuleManager.h"
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "SlateApplication.h"
#include "SSketchfabAssetBrowserWindow.h"

#define LOCTEXT_NAMESPACE "SketchfabAssetBrowser"

DEFINE_LOG_CATEGORY(LogSketchfabAssetBrowser);

USketchfabAssetBrowser::USketchfabAssetBrowser(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
}

bool USketchfabAssetBrowser::ShowWindow()
{
	TSharedPtr<SWindow> ParentWindow;

	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}

	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("SketchfabAssetBrowserWindow", "Sketchfab Asset Browser"))
		.SizingRule(ESizingRule::UserSized)
		.ClientSize(FVector2D(1024, 800));

	TSharedPtr<SSketchfabAssetBrowserWindow> AssetWindow;
	Window->SetContent
	(
		SAssignNew(AssetWindow, SSketchfabAssetBrowserWindow)
		.WidgetWindow(Window)
	);

	if (ParentWindow.IsValid())
	{
		FSlateApplication::Get().AddWindowAsNativeChild(Window, ParentWindow.ToSharedRef());
	}
	else
	{
		FSlateApplication::Get().AddWindow(Window);
	}


	return true;
}

#undef LOCTEXT_NAMESPACE

