// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SketchfabAssetBrowserPrivatePCH.h"
#include "Misc/Paths.h"
#include "UObject/ObjectMacros.h"
#include "UObject/GCObject.h"
#include "SketchfabAssetBrowser.h"
#include "Developer/Settings/Public/ISettingsModule.h"

#include "Engine/Engine.h"
#include "LevelEditor.h"
#include "SketchfabRESTClient.h"

#define LOCTEXT_NAMESPACE "SketchfabAssetBrowser"

class FSketchfabAssetBrowserModule : public ISketchfabAssetBrowserModule, public FGCObject
{
public:

	//Notes:
	//For menu example see: class FFunctionalTestingEditorModule : public IFunctionalTestingEditorModule

	/** IModuleInterface implementation */
	virtual void StartupModule() override
	{
		AssetBrowser = NewObject<USketchfabAssetBrowser>();

		// make an extension to add the Orion function menu
		Extender = MakeShareable(new FExtender());
		Extender->AddMenuExtension(
			"General",
			EExtensionHook::After,
			nullptr,
			FMenuExtensionDelegate::CreateRaw(this, &FSketchfabAssetBrowserModule::AddMenuEntry));

		// add the menu extension to the editor
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(Extender);

		FSketchfabRESTClient::Get(); //Initialize
	}

	virtual void ShutdownModule() override
	{
		FSketchfabRESTClient::Shutdown();
		AssetBrowser = nullptr;
	}

	class USketchfabAssetBrowser* GetAssetBrowser() override
	{
		return AssetBrowser;
	}

	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(AssetBrowser);
	}

private:
	void AddMenuEntry(FMenuBuilder& MenuBuilder)
	{
		MenuBuilder.BeginSection("Sketchfab", LOCTEXT("SketchfabMenu", "Sketchfab"));
		MenuBuilder.AddMenuEntry(
			LOCTEXT("SketchfabMenu_AssetBrowser", "Asset Browser"),
			LOCTEXT("SketchfabMenu_Tooltip", "Launch the Sketchfab Asset Browser."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FSketchfabAssetBrowserModule::MenuCallbackOpenBrowser)));
		MenuBuilder.EndSection();
	}

	void MenuCallbackOpenBrowser() {
		UE_LOG(LogSketchfabAssetBrowser, Log, TEXT("MenuCallbackOpenBrowser"));

		if (AssetBrowser)
		{
			AssetBrowser->ShowWindow();
		}
	}

private:
	USketchfabAssetBrowser* AssetBrowser;
	TSharedPtr<FExtender> Extender;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSketchfabAssetBrowserModule, SketchfabAssetBrowser);

