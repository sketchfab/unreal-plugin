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
					.Text(FText::FromString("Login to your Sketchfab account. Double click on a model to download it. Import by dragging it into the content browser."))
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

					// Filter
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0, 0, 4, 0)
					[
						SNew( SComboButton )
						.ComboButtonStyle( FEditorStyle::Get(), "GenericFilters.ComboButtonStyle" )
						.ForegroundColor(FLinearColor::White)
						.ContentPadding(0)
						.ToolTipText( LOCTEXT( "AddFilterToolTip", "Add an asset filter." ) )
						.OnGetMenuContent( this, &SSketchfabAssetBrowserWindow::MakeFilterMenu )
						.HasDownArrow( true )
						.ContentPadding( FMargin( 1, 0 ) )
						//.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("ContentBrowserFiltersCombo")))
						//.Visibility( ( Config != nullptr ? Config->bCanShowFilters : true ) ? EVisibility::Visible : EVisibility::Collapsed )
						.ButtonContent()
						[
							SNew(SHorizontalBox)

							+ SHorizontalBox::Slot()
							.AutoWidth()
							[
								SNew(STextBlock)
								.TextStyle(FEditorStyle::Get(), "GenericFilters.TextStyle")
								.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.9"))
								.Text(FText::FromString(FString(TEXT("\xf0b0"))) /*fa-filter*/)
							]

							+ SHorizontalBox::Slot()
							.AutoWidth()
							.Padding(2,0,0,0)
							[
								SNew(STextBlock)
								.TextStyle(FEditorStyle::Get(), "GenericFilters.TextStyle")
								.Text(LOCTEXT("Filters", "Filters"))
							]
						]
					]

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
				SNew(SHorizontalBox)
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
		
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	TSharedPtr<IDetailsView> DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	DetailsViewBox->SetContent(DetailsView.ToSharedRef());


	bSearchAnimated = false;
	bSearchStaffPicked = false;

	GetCategories();
	Search();
}

void SSketchfabAssetBrowserWindow::OnAssetsActivated(const TArray<FSketchfabAssetData>& ActivatedAssets, EAssetTypeActivationMethod::Type ActivationMethod)
{
	if (LoggedInUser.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("SSketchfabAssetBrowserWindow_ModelDoubleClick", "Sketchfab_NotLoggedIn", "You must be logged in to download models."));
		if (Window.IsValid())
		{
			Window.Pin()->BringToFront(true);
		}
		return;
	}

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

FReply SSketchfabAssetBrowserWindow::OnSearchPressed()
{
	AssetViewPtr->FlushThumbnails();

	FString url = "https://api.sketchfab.com/v3/search?type=models&downloadable=true";
	if (!TagSearchText.IsEmpty())
	{
		TArray<FString> Array;
		TagSearchText.ParseIntoArray(Array, TEXT(" "), true);

		/*
		url += "&tags=";
		int32 count = Array.Num();
		for (int32 i = 0; i < Array.Num(); i++)
		{
			url += Array[i].ToLower();

			if (i != (count - 1))
			{
				url += "%2C";
			}
		}
		*/

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
	if (true)
	{
		url += "&sort_by=-publishedAt";
	}

	int32 categoryCount = Categories.Num();
	bool firstCategory = false;
	for (int32 i = 0; i < categoryCount; i++)
	{
		if (Categories[i]->active)
		{
			if (!firstCategory)
			{
				url += "&categories=";
				firstCategory = true;
			}
			else
			{
				url += "%2C";
			}
			url += Categories[i]->slug.ToLower();

			break; //You can only have one at a time.
		}
	}

	Search(url);
	return FReply::Handled();
}

bool SSketchfabAssetBrowserWindow::SetSearchBoxText(const FText& InSearchText)
{
	// Has anything changed? (need to test case as the operators are case-sensitive)
	if (!InSearchText.ToString().Equals(TagSearchText, ESearchCase::CaseSensitive))
	{
		TagSearchText = InSearchText.ToString();
		if (InSearchText.IsEmpty())
		{
			AssetViewPtr->FlushThumbnails();
			Search();
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

TSharedRef<SWidget> SSketchfabAssetBrowserWindow::CreateCheckBox(const FText& CheckBoxText, bool* CheckBoxChoice)
{
	return SNew(SCheckBox)
		//.Style(FCoreStyle::Get(), "RadioButton")
		.IsChecked(this, &SSketchfabAssetBrowserWindow::HandleCategoryCheckBoxIsChecked, CheckBoxChoice)
		.OnCheckStateChanged(this, &SSketchfabAssetBrowserWindow::HandleCategoryCheckBoxCheckedStateChanged, CheckBoxChoice)
		[
			SNew(STextBlock)
			.Text(CheckBoxText)
		];
}

// Callback for changing the checked state of a check box.
void SSketchfabAssetBrowserWindow::HandleCategoryCheckBoxCheckedStateChanged(ECheckBoxState NewState, bool* CheckBoxThatChanged)
{
	*CheckBoxThatChanged = (NewState == ECheckBoxState::Checked);
}

// Callback for determining whether a check box is checked.
ECheckBoxState SSketchfabAssetBrowserWindow::HandleCategoryCheckBoxIsChecked(bool* CheckBox) const
{
	return (*CheckBox)
		? ECheckBoxState::Checked
		: ECheckBoxState::Unchecked;
}


void SSketchfabAssetBrowserWindow::MakeCategoriesMenu(FMenuBuilder& MenuBuilder)
{
	// create packing mode menu
	for (int32 i = 0; i < Categories.Num(); i++)
	{
		MenuBuilder.AddWidget(CreateCheckBox(FText::FromString(Categories[i]->name), &Categories[i]->active), FText::GetEmpty());
	}
}

TSharedRef<SWidget> SSketchfabAssetBrowserWindow::MakeFilterMenu()
{
	// create packing mode menu
	FMenuBuilder MenuBuilder(true, NULL);
	{
		MenuBuilder.AddWidget(

			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SCheckBox)
				.IsChecked(this, &SSketchfabAssetBrowserWindow::IsSearchAnimatedChecked)
				.OnCheckStateChanged(this, &SSketchfabAssetBrowserWindow::OnSearchAnimatedCheckStateChanged)
				[
					SNew(STextBlock)
					.Text( LOCTEXT("SSketchfabAssetBrowserWindow_Search_Animated", "Animated" ) )
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SCheckBox)
				.IsChecked(this, &SSketchfabAssetBrowserWindow::IsSearchStaffPickedChecked)
				.OnCheckStateChanged(this, &SSketchfabAssetBrowserWindow::OnSearchStaffPickedCheckStateChanged)
				[
					SNew(STextBlock)
					.Text( LOCTEXT("SSketchfabAssetBrowserWindow_Search_Staff Picked", "Staff Picked" ) )
				]
			]
			, FText::GetEmpty());


		MenuBuilder.AddSubMenu(
			FText::FromString("Categories"), 
			FText::FromString("Choose A Category"), 
			FNewMenuDelegate::CreateRaw(this, &SSketchfabAssetBrowserWindow::MakeCategoriesMenu),
			false,
			FSlateIcon()
		);
	}

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

//=====================================================
// Callback API calls
//=====================================================
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
	Categories = InTask.Categories;
}

#undef LOCTEXT_NAMESPACE

