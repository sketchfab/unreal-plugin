// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SketchfabAssetBrowser.h"
#include "Modules/ModuleManager.h"
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "Framework/Application/SlateApplication.h"
#include "SSketchfabAssetBrowserWindow.h"

#define LOCTEXT_NAMESPACE "SketchfabAssetBrowser"

DEFINE_LOG_CATEGORY(LogSketchfabAssetBrowser);

USketchfabAssetBrowser::USketchfabAssetBrowser(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
}

bool USketchfabAssetBrowser::ShowWindow()
{
	if (SketchfabBrowserWindowPtr.IsValid())
	{
		SketchfabBrowserWindowPtr->BringToFront();
	}
	else
	{
		TSharedPtr<SWindow> ParentWindow;

		if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
		{
			IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
			ParentWindow = MainFrame.GetParentWindow();
		}

		SketchfabBrowserWindowPtr = SNew(SWindow)
			.Title(LOCTEXT("SketchfabAssetBrowserWindow", "Sketchfab Asset Browser"))
			.SizingRule(ESizingRule::UserSized)
			.ClientSize(FVector2D(1024, 800))
			[
				SNew(SSketchfabAssetBrowserWindow)
				.WidgetWindow(SketchfabBrowserWindowPtr)
			];

		// Set the closed callback
		SketchfabBrowserWindowPtr->SetOnWindowClosed(FOnWindowClosed::CreateUObject(this, &USketchfabAssetBrowser::OnWindowClosed));

		if (ParentWindow.IsValid())
		{
			FSlateApplication::Get().AddWindowAsNativeChild(SketchfabBrowserWindowPtr.ToSharedRef(), ParentWindow.ToSharedRef());
		}
		else
		{
			FSlateApplication::Get().AddWindow(SketchfabBrowserWindowPtr.ToSharedRef());
		}
	}

	return true;
}

void USketchfabAssetBrowser::OnWindowClosed(const TSharedRef<SWindow>& InWindow)
{
	SketchfabBrowserWindowPtr = NULL;
}

#undef LOCTEXT_NAMESPACE

