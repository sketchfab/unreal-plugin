// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SSketchfabAssetBrowserWindow.h"
#include "ScopedSlowTask.h"
#include "SUniformGridPanel.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Editor/MainFrame/Public/Interfaces/IMainFrameModule.h"
#include "ObjectTools.h"
#include "MessageLogModule.h"
#include "IMessageLogListing.h"
#include "PackageTools.h"
#include "Engine/StaticMesh.h"
#include "SBox.h"
#include "SButton.h"
#include "SlateApplication.h"
#include "FileManager.h"

#include "Misc/FileHelper.h"
#include "Materials/MaterialInterface.h"
#include "MaterialExpressionIO.h"
#include "Materials/Material.h"
#include "Factories/MaterialFactoryNew.h"
#include "Engine/Texture.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"

#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionOneMinus.h"

#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "ARFilter.h"
#include "Factories/MaterialImportHelpers.h"
#include "EditorFramework/AssetImportData.h"

#include "SSplitter.h"
#include "SSketchfabAssetView.h"
#include "SOAuthWebBrowser.h"
#include "SketchfabRESTClient.h"

#define LOCTEXT_NAMESPACE "SketchfabAssetBrowser"

DEFINE_LOG_CATEGORY(LogSketchfabAssetBrowserWindow);

int32 g_pageCount = 0;

void SSketchfabAssetBrowserWindow::Construct(const FArguments& InArgs)
{
	g_pageCount = 10;

	//IE C:/Users/user/AppData/Local/UnrealEngine/4.18/SketchfabCache/
	CacheFolder = GetSketchfabCacheDir();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*CacheFolder))
	{
		PlatformFile.CreateDirectory(*CacheFolder);
	}

	Window = InArgs._WidgetWindow;

	TSharedPtr<SBox> DetailsViewBox;
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(2)
		[
			SAssignNew(DetailsViewBox, SBox)
			.MaxDesiredHeight(450.0f)
		.MinDesiredWidth(550.0f)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.Padding(2)
		[
			SNew(SUniformGridPanel)
			.SlotPadding(2)
			+ SUniformGridPanel::Slot(0, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
			.Text(LOCTEXT("SSketchfabAssetBrowserWindow_Login", "Login"))
			.OnClicked(this, &SSketchfabAssetBrowserWindow::OnLogin)
			]
			+ SUniformGridPanel::Slot(1, 0)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("SSketchfabAssetBrowserWindow_Cancel", "Cancel"))
				//.ToolTipText(LOCTEXT("SSketchfabAssetBrowserWindow_Cancel_ToolTip", "Close the window"))
				.OnClicked(this, &SSketchfabAssetBrowserWindow::OnCancel)
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(0)
		[
			SAssignNew(AssetViewPtr, SSketchfabAssetView)
			.ThumbnailLabel(ESketchfabThumbnailLabel::AssetName)
			.ThumbnailScale(0.4f)
			.OnAssetsActivated(this, &SSketchfabAssetBrowserWindow::OnAssetsActivated)
			.OnGetAssetContextMenu(this, &SSketchfabAssetBrowserWindow::OnGetAssetContextMenu)
			.AllowDragging(true)
		]
	];
		
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	TSharedPtr<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	DetailsViewBox->SetContent(DetailsView.ToSharedRef());

	Search();
}

void SSketchfabAssetBrowserWindow::OnAssetsActivated(const TArray<FSketchfabAssetData>& ActivatedAssets, EAssetTypeActivationMethod::Type ActivationMethod)
{
	const FText LoadingTemplate = LOCTEXT("LoadingAssetName", "Loading {0}...");
	const FText DefaultText = ActivatedAssets.Num() == 1 ? FText::Format(LoadingTemplate, FText::FromName(ActivatedAssets[0].ObjectUID)) : LOCTEXT("DownloadingModels", "Downloading Models...");
	FScopedSlowTask SlowTask(100, DefaultText);

	for (auto AssetIt = ActivatedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
	{
		const FSketchfabAssetData& AssetData = *AssetIt;

		FSketchfabTaskData TaskData;
		TaskData.Token = Token;
		TaskData.CacheFolder = CacheFolder;
		TaskData.ModelUID = AssetData.ObjectUID.ToString();
		TaskData.StateLock = new FCriticalSection();

		TSharedPtr<FSketchfabTask> SwarmTask = MakeShareable(new FSketchfabTask(TaskData));
		SwarmTask->SetState(SRS_GETMODELLINK);
		SwarmTask->OnModelLink().BindRaw(this, &SSketchfabAssetBrowserWindow::OnModelLink);
		SwarmTask->OnTaskFailed().BindRaw(this, &SSketchfabAssetBrowserWindow::OnTaskFailed);
		FSketchfabRESTClient::Get()->AddTask(SwarmTask);
	}
}

TSharedPtr<SWidget> SSketchfabAssetBrowserWindow::OnGetAssetContextMenu(const TArray<FSketchfabAssetData>& SelectedAssets)
{
	if (SelectedAssets.Num() == 0)
	{
		//return MakeAddNewContextMenu(false, true);
	}
	else
	{
		//return AssetContextMenu->MakeContextMenu(SelectedAssets, AssetViewPtr->GetSourcesData(), Commands);
	}

	return NULL;
}

FReply SSketchfabAssetBrowserWindow::OnLogin()
{
	TSharedPtr<SWindow> ParentWindow;

	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}

	TSharedRef<SWindow> OAuthWindow = SNew(SWindow)
		.Title(LOCTEXT("SketchfabAssetBrowser", "Sketchfab Login Window"))
		.SizingRule(ESizingRule::FixedSize)
		.ClientSize(FVector2D(400, 600));

	OAuthWindowPtr = OAuthWindow;

	FString initialURL = "https://sketchfab.com/oauth2/authorize/?state=123456789&response_type=token&client_id=lZklEgZh9G5LcFqiCUUXTzXyNIZaO7iX35iXODqO";

	TSharedPtr<SOAuthWebBrowser> OAuthBrowser;
	OAuthWindow->SetContent
	(
		SAssignNew(OAuthBrowser, SOAuthWebBrowser)
		.ParentWindow(OAuthWindow)
		.InitialURL(initialURL)
		.OnUrlChanged(this, &SSketchfabAssetBrowserWindow::OnUrlChanged)
	);

	FSlateApplication::Get().AddWindow(OAuthWindow);

	return FReply::Handled();
}


FReply SSketchfabAssetBrowserWindow::OnCancel()
{
	if (Window.IsValid())
	{
		Window.Pin()->RequestDestroyWindow();
	}
	return FReply::Handled();
}


void SSketchfabAssetBrowserWindow::OnUrlChanged(const FText &url)
{
	FString data = url.ToString();

	//If successfully logging in and access given based on the client_id then my redirect URL will be sent here along with 
	//an authorization code. I then get the code from the URL and use that to request an Access Token, which will have an expiration time.
	//I then store this Access Token and the expiration time and use the token for all further communication with the server.

	//https://sketchfab.com/developers/oauth#registering-your-app

	//BEFORE the token expires I can use the "refresh token", which comes with the access token, to obtain a new access token before it expires.

	//https://sketchfab.com/oauth2/success#access_token=DsFGAAK86kLQ1gfUjQQD6gLPhNP6lm&token_type=Bearer&state=123456789&expires_in=36000&scope=read+write


	FString leftBlock = TEXT("https://sketchfab.com/oauth2/success#");
	FString params = data.RightChop(leftBlock.Len());

	FString accesstoken;
	FString tokentype;
	FString state;
	FString expiresin;
	FString scope;

	TArray<FString> parameters;
	if (params.ParseIntoArray(parameters, TEXT("&"), true) > 0)
	{
		for (int a = 0; a < parameters.Num(); a++)
		{
			const FString &param = parameters[a];

			FString left, right;
			if (param.Split(TEXT("="), &left, &right))
			{
				if (left == "access_token")
					accesstoken = right;
				else if (left == "token_type")
					tokentype = right;
				else if (left == "state")
					state = right;
				else if (left == "expires_in")
					expiresin = right;
				else if (left == "scope")
					scope = right;
			}
		}
	}

	if (accesstoken.Len() != 0)
	{
		TSharedPtr<SWindow> loginWindow = OAuthWindowPtr.Pin();
		if (loginWindow.IsValid())
		{
			loginWindow->DestroyWindowImmediately();
		}

		Token = accesstoken;
		GetUserData();
	}

	return;
}

FReply SSketchfabAssetBrowserWindow::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		return OnCancel();
	}

	return FReply::Unhandled();
}

void SSketchfabAssetBrowserWindow::CreateNewAsset(const FString& DefaultAssetName, const FString& PackagePath, UClass* AssetClass, UFactory* Factory, const FString& ModelAssetUID, const FString& ThumbAssetUID)
{
	AssetViewPtr->CreateNewAsset(DefaultAssetName, PackagePath, AssetClass, Factory, ModelAssetUID, ThumbAssetUID);
}

void SSketchfabAssetBrowserWindow::ForceCreateNewAsset(const FString& DefaultAssetName, const FString& PackagePath, const FString& ModelAssetUID, const FString& ThumbAssetUID)
{
	AssetViewPtr->ForceCreateNewAsset(DefaultAssetName, PackagePath, ModelAssetUID, ThumbAssetUID);
}

//=====================================================
// Direct REST API Calls
//=====================================================

void SSketchfabAssetBrowserWindow::Search()
{
	FSketchfabTaskData TaskData;
	TaskData.ModelSearchURL = "https://api.sketchfab.com/v3/search?type=models&downloadable=true&staffpicked=true&sort_by=-publishedAt";
	TaskData.Token = Token;
	TaskData.CacheFolder = CacheFolder;
	TaskData.StateLock = new FCriticalSection();

	TSharedPtr<FSketchfabTask> SwarmTask = MakeShareable(new FSketchfabTask(TaskData));
	SwarmTask->SetState(SRS_SEARCH);
	SwarmTask->OnSearch().BindRaw(this, &SSketchfabAssetBrowserWindow::OnSearch);
	SwarmTask->OnTaskFailed().BindRaw(this, &SSketchfabAssetBrowserWindow::OnTaskFailed);
	FSketchfabRESTClient::Get()->AddTask(SwarmTask);
}

void SSketchfabAssetBrowserWindow::GetUserData()
{
	FSketchfabTaskData TaskData;
	TaskData.Token = Token;
	TaskData.CacheFolder = CacheFolder;
	TaskData.StateLock = new FCriticalSection();

	TSharedPtr<FSketchfabTask> SwarmTask = MakeShareable(new FSketchfabTask(TaskData));
	SwarmTask->SetState(SRS_GETUSERDATA);
	SwarmTask->OnUserData().BindRaw(this, &SSketchfabAssetBrowserWindow::OnUserData);
	SwarmTask->OnTaskFailed().BindRaw(this, &SSketchfabAssetBrowserWindow::OnTaskFailed);
	FSketchfabRESTClient::Get()->AddTask(SwarmTask);
}

//=====================================================
// Callback API calls
//=====================================================
void SSketchfabAssetBrowserWindow::OnTaskFailed(const FSketchfabTask& InTask)
{

}

void SSketchfabAssetBrowserWindow::OnUserData(const FSketchfabTask& InTask)
{
}

void SSketchfabAssetBrowserWindow::OnThumbnailDownloaded(const FSketchfabTask& InTask)
{
	AssetViewPtr->NeedRefresh();
}

void SSketchfabAssetBrowserWindow::OnSearch(const FSketchfabTask& InTask)
{
	for (int32 Index = 0; Index < InTask.SearchData.Num(); Index++)
	{
		const TSharedPtr<FSketchfabTaskData>& Data = InTask.SearchData[Index];

		ForceCreateNewAsset(Data->ModelName, Data->CacheFolder, Data->ModelUID, Data->ThumbnailUID);

		FString jpg = Data->ThumbnailUID + ".jpg";
		FString FileName = Data->CacheFolder / jpg;
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!PlatformFile.FileExists(*FileName))
		{
			FSketchfabTaskData TaskData = (*Data);
			TaskData.StateLock = new FCriticalSection();

			TSharedPtr<FSketchfabTask> SwarmTask = MakeShareable(new FSketchfabTask(TaskData));
			SwarmTask->SetState(SRS_GETTHUMBNAIL);
			SwarmTask->OnThumbnailDownloaded().BindRaw(this, &SSketchfabAssetBrowserWindow::OnThumbnailDownloaded);
			SwarmTask->OnTaskFailed().BindRaw(this, &SSketchfabAssetBrowserWindow::OnTaskFailed);
			FSketchfabRESTClient::Get()->AddTask(SwarmTask);
		}
	}

	AssetViewPtr->NeedRefresh();
}

void SSketchfabAssetBrowserWindow::OnModelLink(const FSketchfabTask& InTask)
{
	FSketchfabTaskData TaskData = InTask.TaskData;
	TaskData.StateLock = new FCriticalSection();

	TSharedPtr<FSketchfabTask> SwarmTask = MakeShareable(new FSketchfabTask(TaskData));
	SwarmTask->SetState(SRS_DOWNLOADMODEL);
	SwarmTask->OnModelDownloaded().BindRaw(this, &SSketchfabAssetBrowserWindow::OnModelDownloaded);
	SwarmTask->OnModelDownloadProgress().BindRaw(this, &SSketchfabAssetBrowserWindow::OnModelDownloadProgress);
	FSketchfabRESTClient::Get()->AddTask(SwarmTask);
}

void SSketchfabAssetBrowserWindow::OnModelDownloaded(const FSketchfabTask& InTask)
{
	AssetViewPtr->NeedRefresh();
}

void SSketchfabAssetBrowserWindow::OnModelDownloadProgress(const FSketchfabTask& InTask)
{
	AssetViewPtr->NeedRefresh();
}

#undef LOCTEXT_NAMESPACE

