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

SSketchfabAssetBrowserWindow::~SSketchfabAssetBrowserWindow()
{
	FSketchfabRESTClient::Get()->Shutdown();
}

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
			.Text(this, &SSketchfabAssetBrowserWindow::GetLoginButtonText)
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
			.Text(LOCTEXT("SSketchfabAssetBrowserWindow_Next", "Next"))
			.OnClicked(this, &SSketchfabAssetBrowserWindow::OnNext)
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
	if (!Token.IsEmpty())
	{
		for (auto AssetIt = ActivatedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
		{
			const FSketchfabAssetData& AssetData = *AssetIt;

			FSketchfabTaskData TaskData;
			TaskData.Token = Token;
			TaskData.CacheFolder = CacheFolder;
			TaskData.ModelUID = AssetData.ModelUID.ToString();
			TaskData.StateLock = new FCriticalSection();

			TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
			Task->SetState(SRS_GETMODELLINK);
			Task->OnModelLink().BindRaw(this, &SSketchfabAssetBrowserWindow::OnModelLink);
			Task->OnTaskFailed().BindRaw(this, &SSketchfabAssetBrowserWindow::OnTaskFailed);
			FSketchfabRESTClient::Get()->AddTask(Task);
		}
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

FText SSketchfabAssetBrowserWindow::GetLoginButtonText() const
{
	if (!LoggedInUser.IsEmpty())
	{
		FString text = LoggedInUser + " Logged In";
		return FText::FromString(text);
	}
	else
	{
		return LOCTEXT("SSketchfabAssetBrowserWindow_OnLogin", "Login");
	}
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

FReply SSketchfabAssetBrowserWindow::OnNext()
{
	g_pageCount = 5;
	Search(NextURL);
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

/*
void SSketchfabAssetBrowserWindow::CreateNewAsset(const FString& DefaultAssetName, const FString& PackagePath, UClass* AssetClass, UFactory* Factory, const FString& ModelAssetUID, const FString& ThumbAssetUID)
{
	AssetViewPtr->CreateNewAsset(DefaultAssetName, PackagePath, AssetClass, Factory, ModelAssetUID, ThumbAssetUID);
}
*/

void SSketchfabAssetBrowserWindow::ForceCreateNewAsset(const FString& DefaultAssetName, const FString& PackagePath, const FString& ModelAssetUID, const FString& ThumbAssetUID)
{
	AssetViewPtr->ForceCreateNewAsset(DefaultAssetName, PackagePath, ModelAssetUID, ThumbAssetUID);
}

//=====================================================
// Direct REST API Calls
//=====================================================

void SSketchfabAssetBrowserWindow::Search()
{
	Search("https://api.sketchfab.com/v3/search?type=models&downloadable=true&staffpicked=true&sort_by=-publishedAt");
}

void SSketchfabAssetBrowserWindow::Search(const FString &url)
{
	FSketchfabTaskData TaskData;
	TaskData.ModelSearchURL = url;
	TaskData.Token = Token;
	TaskData.CacheFolder = CacheFolder;
	TaskData.StateLock = new FCriticalSection();

	TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
	Task->SetState(SRS_SEARCH);
	Task->OnSearch().BindRaw(this, &SSketchfabAssetBrowserWindow::OnSearch);
	Task->OnTaskFailed().BindRaw(this, &SSketchfabAssetBrowserWindow::OnTaskFailed);
	FSketchfabRESTClient::Get()->AddTask(Task);
}

void SSketchfabAssetBrowserWindow::GetUserData()
{
	FSketchfabTaskData TaskData;
	TaskData.Token = Token;
	TaskData.CacheFolder = CacheFolder;
	TaskData.StateLock = new FCriticalSection();

	TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
	Task->SetState(SRS_GETUSERDATA);
	Task->OnUserData().BindRaw(this, &SSketchfabAssetBrowserWindow::OnUserData);
	Task->OnTaskFailed().BindRaw(this, &SSketchfabAssetBrowserWindow::OnTaskFailed);
	FSketchfabRESTClient::Get()->AddTask(Task);
}

//=====================================================
// Callback API calls
//=====================================================
void SSketchfabAssetBrowserWindow::OnTaskFailed(const FSketchfabTask& InTask)
{

}

void SSketchfabAssetBrowserWindow::OnUserData(const FSketchfabTask& InTask)
{
	LoggedInUser = InTask.TaskData.UserName;
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

			TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
			Task->SetState(SRS_GETTHUMBNAIL);
			Task->OnThumbnailDownloaded().BindRaw(this, &SSketchfabAssetBrowserWindow::OnThumbnailDownloaded);
			Task->OnTaskFailed().BindRaw(this, &SSketchfabAssetBrowserWindow::OnTaskFailed);
			FSketchfabRESTClient::Get()->AddTask(Task);
		}
	}

	AssetViewPtr->NeedRefresh();

	NextURL = InTask.TaskData.NextURL;

	if ((--g_pageCount) > 0)
	{
		Search(NextURL);
	}
}

void SSketchfabAssetBrowserWindow::OnModelLink(const FSketchfabTask& InTask)
{
	FSketchfabTaskData TaskData = InTask.TaskData;
	TaskData.StateLock = new FCriticalSection();

	TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
	Task->SetState(SRS_DOWNLOADMODEL);
	Task->OnModelDownloaded().BindRaw(this, &SSketchfabAssetBrowserWindow::OnModelDownloaded);
	Task->OnModelDownloadProgress().BindRaw(this, &SSketchfabAssetBrowserWindow::OnModelDownloadProgress);
	FSketchfabRESTClient::Get()->AddTask(Task);
}

void SSketchfabAssetBrowserWindow::OnModelDownloaded(const FSketchfabTask& InTask)
{
	AssetViewPtr->DownloadProgress(InTask.TaskData.ModelUID, 0.0);
	AssetViewPtr->NeedRefresh();
}

void SSketchfabAssetBrowserWindow::OnModelDownloadProgress(const FSketchfabTask& InTask)
{
	float progress = (float)InTask.TaskData.DownloadedBytes / (float)InTask.TaskData.ModelSize;
	if (progress < 0 || progress > 1.0)
	{
		progress = 1.0;
	}
	AssetViewPtr->DownloadProgress(InTask.TaskData.ModelUID, progress);
	AssetViewPtr->NeedRefresh();
}

#undef LOCTEXT_NAMESPACE

