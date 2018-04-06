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
#include "Engine/Texture.h"
#include "Factories/TextureFactory.h"
#include "Engine/Texture2D.h"

#include "EditorFramework/AssetImportData.h"

#include "SSplitter.h"
#include "SSketchfabAssetView.h"
#include "SketchfabRESTClient.h"
#include "SComboButton.h"
#include "MultiBoxBuilder.h"

#define LOCTEXT_NAMESPACE "SketchfabAssetBrowser"

DEFINE_LOG_CATEGORY(LogSketchfabAssetBrowserWindow);

SSketchfabAssetBrowserWindow::~SSketchfabAssetBrowserWindow()
{
	FSketchfabRESTClient::Get()->Shutdown();
}

void SSketchfabAssetBrowserWindow::Construct(const FArguments& InArgs)
{
	//IE C:/Users/user/AppData/Local/UnrealEngine/4.18/SketchfabCache/
	CacheFolder = GetSketchfabCacheDir();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*CacheFolder))
	{
		PlatformFile.CreateDirectory(*CacheFolder);
	}

	Window = InArgs._WidgetWindow;

	bSearchAnimated = false;
	bSearchStaffPicked = true;
	SortByType = SORTBY_MostRecent;
	FaceCount = FACECOUNT_ALL;
	CategoryIndex = -1;

	TSharedPtr<SBox> DetailsViewBox;
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 2)
		[
			SNew(SBorder)
			.Padding(FMargin(3))
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
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
						.IsEnabled(this, &SSketchfabAssetBrowserWindow::OnLoginEnabled)
					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("SSketchfabAssetBrowserWindow_Logout", "Logout"))
						.OnClicked(this, &SSketchfabAssetBrowserWindow::OnLogout)
						.IsEnabled(this, &SSketchfabAssetBrowserWindow::OnLogoutEnabled)
					]
				]
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 2)
		[
			SNew(SBorder)
			.Padding(FMargin(3))
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(2)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Login to your Sketchfab account. Select a model and click Download Selected to download. Double click to view model information. After downloading you can import by click+dragging it into the content browser."))
					.Justification(ETextJustify::Center)
					.AutoWrapText(true)
					.MinDesiredWidth(400.0f)
				]
			]
		]

		//Search Bar
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 2)
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SBorder)
				.Padding(FMargin(3))
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				[
					SNew(SHorizontalBox)

					// Search Text 
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding( 0, 0, 4, 0 )
					[
						SNew(STextBlock)
						.Text(FText::FromString("Search:"))
						.Justification(ETextJustify::Left)
					]

					// Search
					+SHorizontalBox::Slot()
					.Padding(4, 1, 0, 0)
					.FillWidth(1.0f)
					[
						SAssignNew(SearchBoxPtr, SSketchfabAssetSearchBox)
						//.HintText( this, &SContentBrowser::GetSearchAssetsHintText )
						.OnTextChanged( this, &SSketchfabAssetBrowserWindow::OnSearchBoxChanged )
						.OnTextCommitted( this, &SSketchfabAssetBrowserWindow::OnSearchBoxCommitted )
						//.PossibleSuggestions( this, &SContentBrowser::GetAssetSearchSuggestions )
						.DelayChangeNotificationsWhileTyping( true )
						//.Visibility( ( Config != nullptr ? Config->bCanShowAssetSearch : true ) ? EVisibility::Visible : EVisibility::Collapsed )
						//.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ContentBrowserSearchAssets")))
					]

					// Search Button
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(2.0f, 0.0f, 0.0f, 0.0f)
					[
						SNew(SButton)
						.HAlign(HAlign_Right)
						.Text(LOCTEXT("SSketchfabAssetBrowserWindow_SearchButton", "Search"))
						.OnClicked(this, &SSketchfabAssetBrowserWindow::OnSearchPressed)
						//.ButtonStyle(FEditorStyle::Get(), "FlatButton")
						//.ToolTipText(LOCTEXT("SaveSearchButtonTooltip", "Save the current search as a dynamic collection."))
						//.IsEnabled(this, &SContentBrowser::IsSaveSearchButtonEnabled)
						.ContentPadding( FMargin(1, 1) )
					]
				]
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 2)
		[
			SNew(SBorder)
			.Padding(FMargin(3))
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SUniformGridPanel)
				.SlotPadding(2)
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew( SComboButton )
					.HAlign(HAlign_Center)
					.ComboButtonStyle( FEditorStyle::Get(), "GenericFilters.ComboButtonStyle" )
					.ForegroundColor(FLinearColor::White)
					.ContentPadding(0)
					//.ToolTipText( LOCTEXT( "AddFilterToolTip", "Add an asset filter." ) )
					.OnGetMenuContent( this, &SSketchfabAssetBrowserWindow::MakeCategoriesMenu)
					.HasDownArrow( true )
					.ContentPadding( FMargin( 1, 0 ) )
					.ButtonContent()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "GenericFilters.TextStyle")
							.Text(FText::FromString(FString(TEXT("Category: "))))
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2,0,0,0)
						[
							SAssignNew(CategoryText, STextBlock)
							.TextStyle(FEditorStyle::Get(), "GenericFilters.TextStyle")
							.Text(GetCategoryText())
						]
					]
				]

				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SCheckBox)
					.IsChecked(this, &SSketchfabAssetBrowserWindow::IsSearchAnimatedChecked)
					.OnCheckStateChanged(this, &SSketchfabAssetBrowserWindow::OnSearchAnimatedCheckStateChanged)
					[
						SNew(STextBlock)
						.Text( LOCTEXT("SSketchfabAssetBrowserWindow_Search_Animated", "Animated" ) )
					]
				]

				+ SUniformGridPanel::Slot(2, 0)
				[
					SNew(SCheckBox)
					.IsChecked(this, &SSketchfabAssetBrowserWindow::IsSearchStaffPickedChecked)
					.OnCheckStateChanged(this, &SSketchfabAssetBrowserWindow::OnSearchStaffPickedCheckStateChanged)
					[
						SNew(STextBlock)
						.Text( LOCTEXT("SSketchfabAssetBrowserWindow_Search_Staff Picked", "Staff Picked" ) )
					]
				]
				
				+ SUniformGridPanel::Slot(3, 0)
				[
					SNew( SComboButton )
					.HAlign(HAlign_Center)
					.ComboButtonStyle( FEditorStyle::Get(), "GenericFilters.ComboButtonStyle" )
					.ForegroundColor(FLinearColor::White)
					.ContentPadding(0)
					//.ToolTipText( LOCTEXT( "AddFilterToolTip", "Add an asset filter." ) )
					.OnGetMenuContent( this, &SSketchfabAssetBrowserWindow::MakeFaceCountMenu)
					.HasDownArrow( true )
					.ContentPadding( FMargin( 1, 0 ) )
					.ButtonContent()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "GenericFilters.TextStyle")
							.Text(FText::FromString(FString(TEXT("Faces Count: "))))
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2,0,0,0)
						[
							SAssignNew(FaceCountText, STextBlock)
							.TextStyle(FEditorStyle::Get(), "GenericFilters.TextStyle")
							.Text(GetFaceCountText())
						]
					]
				]

				+ SUniformGridPanel::Slot(4, 0)
				[
					SNew( SComboButton )
					.HAlign(HAlign_Center)
					.ComboButtonStyle( FEditorStyle::Get(), "GenericFilters.ComboButtonStyle" )
					.ForegroundColor(FLinearColor::White)
					.ContentPadding(0)
					//.ToolTipText( LOCTEXT( "AddFilterToolTip", "Add an asset filter." ) )
					.OnGetMenuContent( this, &SSketchfabAssetBrowserWindow::MakeSortByMenu)
					.HasDownArrow( true )
					.ContentPadding( FMargin( 1, 0 ) )
					.ButtonContent()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "GenericFilters.TextStyle")
							.Text(FText::FromString(FString(TEXT("Sort By: "))))
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2,0,0,0)
						[
							SAssignNew(SortByText, STextBlock)
							.TextStyle(FEditorStyle::Get(), "GenericFilters.TextStyle")
							.Text(GetSortByText())
						]
					]
				]
			]
		]

		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 2)
		[
			SNew(SBorder)
			.Padding(FMargin(3))
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.Padding(2)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(2)
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("SSketchfabAssetBrowserWindow_Download", "Download Selected"))
						.OnClicked(this, &SSketchfabAssetBrowserWindow::OnDownloadSelected)
					]

					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("SSketchfabAssetBrowserWindow_ClearCache", "Clear Cache"))
						.OnClicked(this, &SSketchfabAssetBrowserWindow::OnClearCache)
					]
				]

				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.Padding(2)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Text(LOCTEXT("SSketchfabAssetBrowserWindow_Next", "Next"))
					.OnClicked(this, &SSketchfabAssetBrowserWindow::OnNext)
				]
			]
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		.Padding(0)
		[
			SNew(SBorder)
			.Padding(FMargin(3))
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				[
					SAssignNew(AssetViewPtr, SSketchfabAssetView)
					.ThumbnailLabel(ESketchfabThumbnailLabel::AssetName)
					.ThumbnailScale(0.4f)
					.OnAssetsActivated(this, &SSketchfabAssetBrowserWindow::OnAssetsActivated)
					.OnGetAssetContextMenu(this, &SSketchfabAssetBrowserWindow::OnGetAssetContextMenu)
					.AllowDragging(true)
				]
			]
		]
	];
		
	GetCategories();
	OnSearchPressed();
}

void SSketchfabAssetBrowserWindow::DownloadModel(const FString &ModelUID)
{
	if (LoggedInUser.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("SSketchfabAssetBrowserWindow_ModelDoubleClick", "Sketchfab_NotLoggedIn", "You must be logged in to download models."));
		if (Window.IsValid())
		{
			Window.Pin()->BringToFront(true);
		}
	}

	if (LoggedInUser.IsEmpty() || Token.IsEmpty() || ModelUID.IsEmpty())
	{
		return;
	}

	FString fileName = CacheFolder + ModelUID + ".zip";
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (PlatformFile.FileExists(*fileName))
	{
		return;
	}

	//TODO: Check to see if its already scheduled to be downloaded, or is already downloading.
	if (ModelsDownloading.Find(ModelUID))
	{
		return;
	}

	ModelsDownloading.Add(ModelUID);

	FSketchfabTaskData TaskData;
	TaskData.Token = Token;
	TaskData.CacheFolder = CacheFolder;
	TaskData.ModelUID = ModelUID;
	TaskData.StateLock = new FCriticalSection();

	TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
	Task->SetState(SRS_GETMODELLINK);
	Task->OnModelLink().BindRaw(this, &SSketchfabAssetBrowserWindow::OnModelLink);
	Task->OnTaskFailed().BindRaw(this, &SSketchfabAssetBrowserWindow::OnDownloadFailed);
	FSketchfabRESTClient::Get()->AddTask(Task);
}

void SSketchfabAssetBrowserWindow::OnAssetsActivated(const TArray<FSketchfabAssetData>& ActivatedAssets, EAssetTypeActivationMethod::Type ActivationMethod)
{
	if (ActivatedAssets.Num() > 0)
	{
		ShowModelWindow(ActivatedAssets[0]);
	}
}

FReply SSketchfabAssetBrowserWindow::OnDownloadSelected()
{
	if (LoggedInUser.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("SSketchfabAssetBrowserWindow_ModelDoubleClick", "Sketchfab_NotLoggedIn", "You must be logged in to download models."));
		if (Window.IsValid())
		{
			Window.Pin()->BringToFront(true);
		}
		return FReply::Handled();
	}

	if (!Token.IsEmpty())
	{
		TArray<FSketchfabAssetData> data = AssetViewPtr->GetSelectedAssets();
		for (int32 i = 0; i < data.Num(); i++)
		{
			const FSketchfabAssetData& AssetData = data[i];
			DownloadModel(AssetData.ModelUID.ToString());
		}
	}
	return FReply::Handled();
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

FReply SSketchfabAssetBrowserWindow::OnSearchPressed()
{
	AssetViewPtr->FlushThumbnails();

	FString url = "https://api.sketchfab.com/v3/search?type=models&downloadable=true";
	if (!QuerySearchText.IsEmpty())
	{
		TArray<FString> Array;
		QuerySearchText.ParseIntoArray(Array, TEXT(" "), true);

		url += "&q=";
		int32 count = Array.Num();
		for (int32 i = 0; i < Array.Num(); i++)
		{
			url += Array[i].ToLower();

			if (i != (count - 1))
			{
				url += "+";
			}
		}
	}

	if (bSearchAnimated)
	{
		url += "&animated=true";
	}

	if (bSearchStaffPicked)
	{
		url += "&staffpicked=true";
	}

	//Sort By
	switch (SortByType)
	{
	case SORTBY_Relevance:
	{
		url += "&sort_by=-pertinence";
	}
	break;
	case SORTBY_MostLiked:
	{
		url += "&sort_by=-likeCount";
	}
	break;
	case SORTBY_MostViewed:
	{
		url += "&sort_by=-viewCount";
	}
	break;
	case SORTBY_MostRecent:
	{
		url += "&sort_by=-publishedAt";
	}
	break;
	}

	//Sort By
	switch (FaceCount)
	{
	case FACECOUNT_ALL:
	{
		//Add Nothing
	}
	break;
	case FACECOUNT_0_10:
	{
		url += "&min_face_count=0&max_face_count=10000";
	}
	break;
	case FACECOUNT_10_50:
	{
		url += "&min_face_count=10000&max_face_count=50000";
	}
	break;
	case FACECOUNT_50_100:
	{
		url += "&min_face_count=50000&max_face_count=100000";
	}
	break;
	case FACECOUNT_100_250:
	{
		url += "&min_face_count=100000&max_face_count=250000";
	}
	break;
	case FACECOUNT_250:
	{
		url += "&min_face_count=250000";
	}
	break;
	}

	if (CategoryIndex >= 0 && CategoryIndex < Categories.Num())
	{
		url += "&categories=";
		url += Categories[CategoryIndex].slug.ToLower();
	}

	Search(url);
	return FReply::Handled();
}

bool SSketchfabAssetBrowserWindow::SetSearchBoxText(const FText& InSearchText)
{
	// Has anything changed? (need to test case as the operators are case-sensitive)
	if (!InSearchText.ToString().Equals(QuerySearchText, ESearchCase::CaseSensitive))
	{
		QuerySearchText = InSearchText.ToString();
		if (InSearchText.IsEmpty())
		{
			AssetViewPtr->FlushThumbnails();
			OnSearchPressed();
		}
		else
		{
			return true;
		}
	}
	return false;
}

void SSketchfabAssetBrowserWindow::OnSearchBoxChanged(const FText& InSearchText)
{
	//SetSearchBoxText(InSearchText);
}

void SSketchfabAssetBrowserWindow::OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo)
{
	if (SetSearchBoxText(InSearchText))
	{
		OnSearchPressed();
	}
}

FReply SSketchfabAssetBrowserWindow::OnLogin()
{
	TSharedPtr<SWindow> loginWindow = OAuthWindowPtr.Pin();
	if (loginWindow.IsValid())
	{
		loginWindow->ShowWindow();
		return FReply::Handled();
	}

	TSharedPtr<SWindow> ParentWindow;

	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}

	TSharedRef<SWindow> OAuthWindow = SNew(SWindow)
		.Title(LOCTEXT("SketchfabAssetBrowser_LoginWindow", "Sketchfab Login Window"))
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

	FWidgetPath WidgetPath;
	TSharedPtr<SWindow> CurrentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared(), WidgetPath);
	FSlateApplication::Get().AddWindowAsNativeChild(OAuthWindow, CurrentWindow.ToSharedRef());

	return FReply::Handled();
}

FReply SSketchfabAssetBrowserWindow::OnLogout()
{
	Token = "";
	LoggedInUser = "";

	TSharedPtr<SWindow> loginWindow = OAuthWindowPtr.Pin();
	if (loginWindow.IsValid())
	{
		loginWindow->ShowWindow();
		return FReply::Handled();
	}

	TSharedPtr<SWindow> ParentWindow;

	if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
	{
		IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
		ParentWindow = MainFrame.GetParentWindow();
	}

	TSharedRef<SWindow> OAuthWindow = SNew(SWindow)
		.Title(LOCTEXT("SketchfabAssetBrowser_LogoutWindow", "Sketchfab Logout Window"))
		.SizingRule(ESizingRule::FixedSize)
		.ClientSize(FVector2D(400, 600));

	OAuthWindowPtr = OAuthWindow;

	FString initialURL = "https://sketchfab.com/logout";

	TSharedPtr<SOAuthWebBrowser> OAuthBrowser;
	OAuthWindow->SetContent
	(
		SAssignNew(OAuthBrowser, SOAuthWebBrowser)
		.ParentWindow(OAuthWindow)
		.InitialURL(initialURL)
		.OnUrlChanged(this, &SSketchfabAssetBrowserWindow::OnUrlChanged)
		.OnCloseWindow(this, &SSketchfabAssetBrowserWindow::LoginBrowserClosed)
	);

	FWidgetPath WidgetPath;
	TSharedPtr<SWindow> CurrentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared(), WidgetPath);
	FSlateApplication::Get().AddWindowAsNativeChild(OAuthWindow, CurrentWindow.ToSharedRef());

	return FReply::Handled();
}

bool SSketchfabAssetBrowserWindow::OnLoginEnabled() const
{
	if (Token.IsEmpty() || LoggedInUser.IsEmpty())
	{
		return true;
	}
	return false;
}

bool SSketchfabAssetBrowserWindow::OnLogoutEnabled() const
{
	return !OnLoginEnabled();
}

bool SSketchfabAssetBrowserWindow::LoginBrowserClosed(const TWeakPtr<IWebBrowserWindow>& WindowPtr)
{
	OAuthWindowPtr = NULL;
	return true;
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
	Search(NextURL);
	return FReply::Handled();
}

FReply SSketchfabAssetBrowserWindow::OnClearCache()
{
	AssetViewPtr->FlushThumbnails();

	FString Folder = GetSketchfabCacheDir();

	FString msg = "Delete all files in the Sketchfab cache folder '" + Folder + "' ?";
	if (FMessageDialog::Open(EAppMsgType::OkCancel, FText::FromString(msg)) == EAppReturnType::Ok)
	{
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (PlatformFile.DirectoryExists(*Folder))
		{
			PlatformFile.DeleteDirectoryRecursively(*Folder);
			PlatformFile.CreateDirectory(*Folder);
		}
	}

	if (Window.IsValid())
	{
		Window.Pin()->BringToFront(true);
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
	if (data.Contains(leftBlock))
	{
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

			OAuthWindowPtr = NULL;

			Token = accesstoken;
			GetUserData();
		}
		return;
	}

	leftBlock = TEXT("https://sketchfab.com/?logged_out=1");
	if (data.Contains(leftBlock))
	{
		TSharedPtr<SWindow> loginWindow = OAuthWindowPtr.Pin();
		if (loginWindow.IsValid())
		{
			loginWindow->DestroyWindowImmediately();
		}
		OAuthWindowPtr = NULL;
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

void SSketchfabAssetBrowserWindow::AddCategoryWidget(FMenuBuilder &MenuBuilder, int32 CategoryIndex)
{
	MenuBuilder.AddWidget(
		SNew(SCheckBox)
		.Style(FCoreStyle::Get(), "RadioButton")
		.IsChecked(this, &SSketchfabAssetBrowserWindow::HandleCategoryIsChecked, CategoryIndex)
		.OnCheckStateChanged(this, &SSketchfabAssetBrowserWindow::HandleCategoryStateChanged, CategoryIndex)
		[
			SNew(STextBlock)
			.Text(GetCategoryText(CategoryIndex))
		], FText::GetEmpty()
	);
}

TSharedRef<SWidget> SSketchfabAssetBrowserWindow::MakeCategoriesMenu()
{
	// create packing mode menu
	FMenuBuilder MenuBuilder(true, NULL);
	AddCategoryWidget(MenuBuilder, -1);

	for (int32 i = 0; i < Categories.Num(); i++)
	{
		AddCategoryWidget(MenuBuilder, i);
	}

	return MenuBuilder.MakeWidget();
}

void SSketchfabAssetBrowserWindow::AddSortByWidget(FMenuBuilder &MenuBuilder, ESortBy sb)
{
	MenuBuilder.AddWidget(
		SNew(SCheckBox)
		.Style(FCoreStyle::Get(), "RadioButton")
		.IsChecked(this, &SSketchfabAssetBrowserWindow::HandleSortByTypeIsChecked, sb)
		.OnCheckStateChanged(this, &SSketchfabAssetBrowserWindow::HandleSortByTypeStateChanged, sb)
		[
			SNew(STextBlock)
			.Text(GetSortByText(sb))
		], FText::GetEmpty()
	);
}

TSharedRef<SWidget> SSketchfabAssetBrowserWindow::MakeSortByMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);
	AddSortByWidget(MenuBuilder, SORTBY_Relevance);
	AddSortByWidget(MenuBuilder, SORTBY_MostLiked);
	AddSortByWidget(MenuBuilder, SORTBY_MostViewed);
	AddSortByWidget(MenuBuilder, SORTBY_MostRecent);
	return MenuBuilder.MakeWidget();
}

void SSketchfabAssetBrowserWindow::AddFaceCountWidget(FMenuBuilder &MenuBuilder, EFaceCount fc)
{
	MenuBuilder.AddWidget(
		SNew(SCheckBox)
		.Style(FCoreStyle::Get(), "RadioButton")
		.IsChecked(this, &SSketchfabAssetBrowserWindow::HandleFaceCountChecked, fc)
		.OnCheckStateChanged(this, &SSketchfabAssetBrowserWindow::HandleFaceCountStateChanged, fc)
		[
			SNew(STextBlock)
			.Text(GetFaceCountText(fc))
		], FText::GetEmpty()
	);
}

TSharedRef<SWidget> SSketchfabAssetBrowserWindow::MakeFaceCountMenu()
{
	FMenuBuilder MenuBuilder(true, NULL);
	AddFaceCountWidget(MenuBuilder, FACECOUNT_ALL);
	AddFaceCountWidget(MenuBuilder, FACECOUNT_0_10);
	AddFaceCountWidget(MenuBuilder, FACECOUNT_10_50);
	AddFaceCountWidget(MenuBuilder, FACECOUNT_50_100);
	AddFaceCountWidget(MenuBuilder, FACECOUNT_100_250);
	AddFaceCountWidget(MenuBuilder, FACECOUNT_250);
	return MenuBuilder.MakeWidget();
}

ECheckBoxState SSketchfabAssetBrowserWindow::IsSearchAnimatedChecked() const
{
	return bSearchAnimated ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SSketchfabAssetBrowserWindow::OnSearchAnimatedCheckStateChanged(ECheckBoxState NewState)
{
	bSearchAnimated = (NewState == ECheckBoxState::Checked);
}

ECheckBoxState SSketchfabAssetBrowserWindow::IsSearchStaffPickedChecked() const
{
	return bSearchStaffPicked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SSketchfabAssetBrowserWindow::OnSearchStaffPickedCheckStateChanged(ECheckBoxState NewState)
{
	bSearchStaffPicked = (NewState == ECheckBoxState::Checked);
}


void SSketchfabAssetBrowserWindow::ShowModelWindow(const FSketchfabAssetData& AssetData)
{
	TSharedRef<SWindow> ModelWindow = SNew(SWindow)
		.Title(LOCTEXT("SketchfabAssetWindow", "Sketchfab Model Info"))
		.SizingRule(ESizingRule::FixedSize)
		.ClientSize(FVector2D(600, 600));

	ModelWindow->SetContent
	(
		SAssignNew(AssetWindow, SSketchfabAssetWindow)
		.WidgetWindow(ModelWindow)
		.AssetData(AssetData)
		.ThumbnailPool(AssetViewPtr->GetThumbnailPool())
		.OnDownloadRequest(this, &SSketchfabAssetBrowserWindow::DownloadModel)
	);

	FWidgetPath WidgetPath;
	TSharedPtr<SWindow> CurrentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared(), WidgetPath);
	FSlateApplication::Get().AddWindowAsNativeChild(ModelWindow, CurrentWindow.ToSharedRef());

	GetModelInfo(AssetData.ModelUID.ToString());
}

//=====================================================
// Direct REST API Calls
//=====================================================
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

void SSketchfabAssetBrowserWindow::GetCategories()
{
	FSketchfabTaskData TaskData;
	TaskData.Token = Token;
	TaskData.CacheFolder = CacheFolder;
	TaskData.StateLock = new FCriticalSection();

	TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
	Task->SetState(SRS_GETCATEGORIES);
	Task->OnCategories().BindRaw(this, &SSketchfabAssetBrowserWindow::OnCategories);
	Task->OnTaskFailed().BindRaw(this, &SSketchfabAssetBrowserWindow::OnTaskFailed);
	FSketchfabRESTClient::Get()->AddTask(Task);
}

void SSketchfabAssetBrowserWindow::GetModelInfo(const FString &ModelUID)
{
	FSketchfabTaskData TaskData;
	TaskData.CacheFolder = GetSketchfabCacheDir();
	TaskData.ModelUID = ModelUID;
	TaskData.StateLock = new FCriticalSection();

	TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
	Task->SetState(SRS_GETMODELINFO);
	Task->OnModelInfo().BindRaw(this, &SSketchfabAssetBrowserWindow::OnGetModelInfo);
	Task->OnTaskFailed().BindRaw(this, &SSketchfabAssetBrowserWindow::OnTaskFailed);
	FSketchfabRESTClient::Get()->AddTask(Task);
}

void SSketchfabAssetBrowserWindow::GetBigThumbnail(const FSketchfabTaskData &data)
{
	FString jpg = data.ThumbnailUID_1024 + ".jpg";
	FString FileName = data.CacheFolder / jpg;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.FileExists(*FileName))
	{
		FSketchfabTaskData TaskData = data;
		TaskData.ThumbnailUID = data.ThumbnailUID_1024;
		TaskData.ThumbnailURL = data.ThumbnailURL_1024;
		TaskData.StateLock = new FCriticalSection();

		TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
		Task->SetState(SRS_GETTHUMBNAIL);
		Task->OnThumbnailDownloaded().BindRaw(this, &SSketchfabAssetBrowserWindow::OnGetBigThumbnail);
		Task->OnTaskFailed().BindRaw(this, &SSketchfabAssetBrowserWindow::OnTaskFailed);
		FSketchfabRESTClient::Get()->AddTask(Task);
	}
	else
	{
		if (AssetWindow.IsValid())
		{
			AssetWindow->SetThumbnail(data);
		}
	}
}

//=====================================================
// Callback API calls
//=====================================================
void SSketchfabAssetBrowserWindow::OnDownloadFailed(const FSketchfabTask& InTask)
{
	ModelsDownloading.Remove(InTask.TaskData.ModelUID);
}

void SSketchfabAssetBrowserWindow::OnTaskFailed(const FSketchfabTask& InTask)
{
}

void SSketchfabAssetBrowserWindow::OnUserData(const FSketchfabTask& InTask)
{
	LoggedInUser = InTask.TaskData.UserName;

	//Download the users thumbnail

	FSketchfabTaskData TaskData = InTask.TaskData;
	TaskData.StateLock = new FCriticalSection();

	TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
	Task->SetState(SRS_GETUSERTHUMB);
	Task->OnUserThumbnail().BindRaw(this, &SSketchfabAssetBrowserWindow::OnUserThumbnailDownloaded);
	Task->OnTaskFailed().BindRaw(this, &SSketchfabAssetBrowserWindow::OnTaskFailed);
	FSketchfabRESTClient::Get()->AddTask(Task);
}

void SSketchfabAssetBrowserWindow::OnUserThumbnailDownloaded(const FSketchfabTask& InTask)
{
	AssetViewPtr->NeedRefresh();
}

void SSketchfabAssetBrowserWindow::OnThumbnailDownloaded(const FSketchfabTask& InTask)
{
	AssetViewPtr->NeedRefresh();
}

void SSketchfabAssetBrowserWindow::OnSearch(const FSketchfabTask& InTask)
{
	for (int32 Index = 0; Index < InTask.SearchData.Num(); Index++)
	{
		TSharedPtr<FSketchfabTaskData> Data = InTask.SearchData[Index];

		AssetViewPtr->ForceCreateNewAsset(Data);

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
}

void SSketchfabAssetBrowserWindow::OnModelLink(const FSketchfabTask& InTask)
{
	FSketchfabTaskData TaskData = InTask.TaskData;
	TaskData.StateLock = new FCriticalSection();

	TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
	Task->SetState(SRS_DOWNLOADMODEL);
	Task->OnModelDownloaded().BindRaw(this, &SSketchfabAssetBrowserWindow::OnModelDownloaded);
	Task->OnModelDownloadProgress().BindRaw(this, &SSketchfabAssetBrowserWindow::OnModelDownloadProgress);
	Task->OnTaskFailed().BindRaw(this, &SSketchfabAssetBrowserWindow::OnTaskFailed);
	FSketchfabRESTClient::Get()->AddTask(Task);
}

void SSketchfabAssetBrowserWindow::OnModelDownloaded(const FSketchfabTask& InTask)
{
	AssetViewPtr->DownloadProgress(InTask.TaskData.ModelUID, 0.0);
	AssetViewPtr->NeedRefresh();
	ModelsDownloading.Remove(InTask.TaskData.ModelUID);
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

void SSketchfabAssetBrowserWindow::OnCategories(const FSketchfabTask& InTask)
{
	Categories = InTask.TaskData.Categories;
}


void SSketchfabAssetBrowserWindow::OnGetModelInfo(const FSketchfabTask& InTask)
{
	if (AssetWindow.IsValid())
	{
		AssetWindow->SetThumbnail(InTask);
		AssetWindow->SetModelInfo(InTask);
	}

	if (!InTask.TaskData.ThumbnailUID_1024.IsEmpty())
	{
		GetBigThumbnail(InTask.TaskData);
	}
}
void SSketchfabAssetBrowserWindow::OnGetBigThumbnail(const FSketchfabTask& InTask)
{
	if (AssetWindow.IsValid())
	{
		AssetWindow->SetThumbnail(InTask);
	}
}



#undef LOCTEXT_NAMESPACE

