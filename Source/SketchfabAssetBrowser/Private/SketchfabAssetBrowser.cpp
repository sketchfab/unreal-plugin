// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SketchfabAssetBrowser.h"
#include "Modules/ModuleManager.h"
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "Framework/Application/SlateApplication.h"
#include "SSketchfabAssetBrowserWindow.h"
#include "SSketchfabExporterWindow.h"

#define LOCTEXT_NAMESPACE "SketchfabAssetBrowser"


USketchfabWindow::USketchfabWindow(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
}

bool USketchfabWindow::ShowWindow()
{
	if (WindowPtr.IsValid())
	{
		WindowPtr->BringToFront();
	}
	else
	{
		TSharedPtr<SWindow> ParentWindow;

		if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
		{
			IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
			ParentWindow = MainFrame.GetParentWindow();
		}

		WindowPtr = CreateWindow();

		// Set the closed callback
		WindowPtr->SetOnWindowClosed(FOnWindowClosed::CreateUObject(this, &USketchfabWindow::OnWindowClosed));

		if (ParentWindow.IsValid())
		{
			FSlateApplication::Get().AddWindowAsNativeChild(WindowPtr.ToSharedRef(), ParentWindow.ToSharedRef());
		}
		else
		{
			FSlateApplication::Get().AddWindow(WindowPtr.ToSharedRef());
		}
	}

	return true;
}

void USketchfabWindow::OnWindowClosed(const TSharedRef<SWindow>& InWindow)
{
	WindowPtr = NULL;
}


DEFINE_LOG_CATEGORY(LogSketchfabAssetBrowser);
USketchfabAssetBrowser::USketchfabAssetBrowser(const FObjectInitializer& Initializer)
	: Super(Initializer)
{}
TSharedPtr<SWindow> USketchfabAssetBrowser::CreateWindow() {
	return SNew(SWindow)
		.SizingRule(ESizingRule::UserSized)
		.Title(FText::FromString("Sketchfab Asset Browser"))
		.ClientSize(FVector2D(1024, 800))
		.MinHeight(600)
		.MinWidth(800)
		[
			SNew(SSketchfabAssetBrowserWindow).WidgetWindow(WindowPtr)
		];
}


DEFINE_LOG_CATEGORY(LogSketchfabExporter);
USketchfabExporter::USketchfabExporter(const FObjectInitializer& Initializer)
	: Super(Initializer)
{}
TSharedPtr<SWindow> USketchfabExporter::CreateWindow() {
	return SNew(SWindow)
		.SizingRule(ESizingRule::UserSized)
		.Title(FText::FromString("Sketchfab Exporter"))
		.ClientSize(FVector2D(600, 625))
		.MinHeight(525)
		.MinWidth(500)
		[
			SNew(SSketchfabExporterWindow).WidgetWindow(WindowPtr)
		];
}


#undef LOCTEXT_NAMESPACE

