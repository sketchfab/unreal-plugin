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
#include "AssetToolsModule.h"
#include "IWebBrowserCookieManager.h"
#include "WebBrowserModule.h"

#include "IPluginManager.h"

#define LOCTEXT_NAMESPACE "SketchfabAssetBrowser"

DEFINE_LOG_CATEGORY(LogSketchfabAssetBrowserWindow);



class SAcceptLicenseWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAcceptLicenseWindow)
	{}

	SLATE_ARGUMENT(FSketchfabAssetData, AssetData)

	SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
		SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs)
	{
		AssetData = InArgs._AssetData;
		Window = InArgs._WidgetWindow;
		bShouldImport = false;

		ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0, 0, 0, 2)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(2,2,2,10)
				[
					SNew(STextBlock)
					.Text(FText::FromString("You must accept the following license before using this model"))
					.Justification(ETextJustify::Center)
					.AutoWrapText(true)
					.MinDesiredWidth(400.0f)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(2)
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.05f, 1.f))
					.Padding(FMargin(2))
					[
						SNew(SBox)
						.MinDesiredHeight(200.0f)
						.MinDesiredWidth(400.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0, 0, 0, 4)
							[
								SNew(STextBlock)
								.Text(FText::FromString("License Information"))
								.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0, 0, 0, 2)
							[
								SNew(STextBlock)
								.AutoWrapText(true)
								.Text(FText::FromString(AssetData.LicenceType))
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0, 0, 0, 2)
							[
								SNew(STextBlock)
								.AutoWrapText(true)
								.Text(FText::FromString(AssetData.LicenceInfo))
							]
						]
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(2)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(2)
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Text(LOCTEXT("SAcceptLicenseWindow_Accept", "Accept"))
					.OnClicked(this, &SAcceptLicenseWindow::OnImport)
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.Text(LOCTEXT("SAcceptLicenseWindow_Cancel", "Cancel"))
					.OnClicked(this, &SAcceptLicenseWindow::OnCancel)
				]
			]
		];
	}

	virtual bool SupportsKeyboardFocus() const override { return true; }

	FReply OnImport()
	{
		bShouldImport = true;
		if (Window.IsValid())
		{
			Window.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}


	FReply OnCancel()
	{
		bShouldImport = false;
		if (Window.IsValid())
		{
			Window.Pin()->RequestDestroyWindow();
		}
		return FReply::Handled();
	}

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
	{
		if (InKeyEvent.GetKey() == EKeys::Escape)
		{
			return OnCancel();
		}

		return FReply::Unhandled();
	}

	bool ShouldImport() const
	{
		return bShouldImport;
	}

private:
	TWeakPtr< SWindow > Window;
	FSketchfabAssetData AssetData;
	bool bShouldImport;
};

//===========================================================================

SSketchfabAssetBrowserWindow::~SSketchfabAssetBrowserWindow()
{
	FSketchfabRESTClient::Get()->Shutdown();
}

void SSketchfabAssetBrowserWindow::Construct(const FArguments& InArgs)
{
	//IE C:/Users/user/AppData/Local/UnrealEngine/4.18/SketchfabCache/
	CacheFolder = GetSketchfabCacheDir();
	CheckLatestPluginVersion();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*CacheFolder))
	{
		PlatformFile.CreateDirectory(*CacheFolder);
	}

	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("Sketchfab");
	FPluginDescriptor descriptor = Plugin->GetDescriptor();
	CurrentPluginVersion = descriptor.VersionName;

	Window = InArgs._WidgetWindow;
	bSearchMyModels = false;
	bSearchAnimated = false;
	bSearchStaffPicked = true;

	CategoryIndex = 0;
	CurrentCategoryString = TEXT("All");

	SortByIndex = (int32)SORTBY_MostRecent;
	CurrentSortByString = TEXT("Most Recent");
	for (int32 i = 0; i < (int32)SORTBY_UNDEFINED; i++)
	{
		switch (i)
		{
		case SORTBY_Relevance:
		{
			SortByComboList.Add(MakeShared<FString>(TEXT("Relevance")));
		}
		break;
		case SORTBY_MostLiked:
		{
			SortByComboList.Add(MakeShared<FString>(TEXT("Most Liked")));
		}
		break;
		case SORTBY_MostViewed:
		{
			SortByComboList.Add(MakeShared<FString>(TEXT("Most Viewed")));
		}
		break;
		case SORTBY_MostRecent:
		{
			SortByComboList.Add(MakeShared<FString>(TEXT("Most Recent")));
		}
		break;
		}
	}

	FaceCountIndex = (int32)FACECOUNT_ALL;
	CurrentFaceCountString = TEXT("All");
	for (int32 i = 0; i < (int32)FACECOUNT_UNDEFINED; i++)
	{
		switch (i)
		{
		case FACECOUNT_ALL:
		{
			FaceCountComboList.Add(MakeShared<FString>(TEXT("All")));
		}
		break;
		case FACECOUNT_0_10:
		{
			FaceCountComboList.Add(MakeShared<FString>(TEXT("Up to 10k")));
		}
		break;
		case FACECOUNT_10_50:
		{
			FaceCountComboList.Add(MakeShared<FString>(TEXT("10k to 50k")));
		}
		break;
		case FACECOUNT_50_100:
		{
			FaceCountComboList.Add(MakeShared<FString>(TEXT("50k to 100k")));
		}
		break;
		case FACECOUNT_100_250:
		{
			FaceCountComboList.Add(MakeShared<FString>(TEXT("100k to 250k")));
		}
		break;
		case FACECOUNT_250:
		{
			FaceCountComboList.Add(MakeShared<FString>(TEXT("250k+")));
		}
		break;
		}
	}



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
				.HAlign(HAlign_Right)
				.Padding(2)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(2)
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.Margin(FMargin(0.0f, 5.0f, 0.0f, 0.0f))
						.Text(this, &SSketchfabAssetBrowserWindow::GetLoginText)
						.MinDesiredWidth(200.0f)
					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(this, &SSketchfabAssetBrowserWindow::GetLoginButtonText)
						.OnClicked(this, &SSketchfabAssetBrowserWindow::OnLogin)
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
					.Text(FText::FromString("1. Login to your Sketchfab account.\n2. Select a model and click Download Selected to download (double click to view model information)\n3. After downloading you can import by click+dragging it into the content browser."))
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
					.Padding(2.0f, 0.0f, 2.0f, 0.0f)
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

					// My models checkbox
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(12.0f, 0.0f, 4.0f, 0.0f)
					[
						SNew(SCheckBox)
						.IsChecked(this, &SSketchfabAssetBrowserWindow::IsSearchMyModelsChecked)
						.IsEnabled(this, &SSketchfabAssetBrowserWindow::IsSearchMyModelsAvailable)
						.OnCheckStateChanged(this, &SSketchfabAssetBrowserWindow::OnSearchMyModelsCheckStateChanged)
						[
							SNew(STextBlock)
							.Text(this, &SSketchfabAssetBrowserWindow::GetMyModelText)
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

				// Categories
				+SHorizontalBox::Slot()
				.MaxWidth(250.0f)
				.Padding(12.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(2.0f, 6.0f)
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CategoryComboMenu", "Categories:"))
					]
					+ SHorizontalBox::Slot()
					.Padding(2.0f, 2.0f)
					.FillWidth(1.0f)
					.HAlign(HAlign_Fill)
					[
						SAssignNew(CategoriesComboBox, SComboBox<TSharedPtr<FString>>)
						.OptionsSource(&CategoryComboList)
						.OnGenerateWidget(this, &SSketchfabAssetBrowserWindow::GenerateCategoryComboItem)
						.OnSelectionChanged(this, &SSketchfabAssetBrowserWindow::HandleCategoryComboChanged)
						[
							SNew(STextBlock)
							.Text(this, &SSketchfabAssetBrowserWindow::GetCategoryComboText)
						]
					]
				]

				// Animated
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(12.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SCheckBox)
					.IsChecked(this, &SSketchfabAssetBrowserWindow::IsSearchAnimatedChecked)
					.OnCheckStateChanged(this, &SSketchfabAssetBrowserWindow::OnSearchAnimatedCheckStateChanged)
					[
						SNew(STextBlock)
						.Text( LOCTEXT("SSketchfabAssetBrowserWindow_Search_Animated", "Animated" ) )
					]
				]

				// Staffpicked
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(12.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SCheckBox)
					.IsChecked(this, &SSketchfabAssetBrowserWindow::IsSearchStaffPickedChecked)
					.OnCheckStateChanged(this, &SSketchfabAssetBrowserWindow::OnSearchStaffPickedCheckStateChanged)
					[
						SNew(STextBlock)
						.Text( LOCTEXT("SSketchfabAssetBrowserWindow_Search_Staff Picked", "Staff Picked" ) )
					]
				]

				// Face count
				+SHorizontalBox::Slot()
				.MaxWidth(180.0f)
				.Padding(12.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.Padding(2.0f, 6.0f)
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("FaceCountComboMenu", "Face Count:"))
					]
					+ SHorizontalBox::Slot()
					.Padding(2.0f, 2.0f)
					.FillWidth(1.0f)
					.HAlign(HAlign_Fill)
					[
						SAssignNew(FaceCountComboBox, SComboBox<TSharedPtr<FString>>)
						.OptionsSource(&FaceCountComboList)
						.OnGenerateWidget(this, &SSketchfabAssetBrowserWindow::GenerateFaceCountComboItem)
						.OnSelectionChanged(this, &SSketchfabAssetBrowserWindow::HandleFaceCountComboChanged)
						[
							SNew(STextBlock)
							.Text(this, &SSketchfabAssetBrowserWindow::GetFaceCountComboText)
						]
					]
				]
				// Sort by
				+SHorizontalBox::Slot()
				.Padding(12.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SSpacer)
				]

				// Sort by
				+SHorizontalBox::Slot()
				.Padding(12.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Right)
					.Padding(0, 0, 0, 2)
					[
						SNew(SHorizontalBox)
						// Sort by
						+ SHorizontalBox::Slot()
						.Padding(2.0f, 6.0f)
						.AutoWidth()
						.HAlign(HAlign_Right)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("SortByComboMenu", "Sort By:"))
						]
						+ SHorizontalBox::Slot()
						.Padding(2.0f, 2.0f)
						.MaxWidth(140.0f)
						.HAlign(HAlign_Right)
						[
							SAssignNew(SortByComboBox, SComboBox<TSharedPtr<FString>>)
							.OptionsSource(&SortByComboList)
							.OnGenerateWidget(this, &SSketchfabAssetBrowserWindow::GenerateSortByComboItem)
							.OnSelectionChanged(this, &SSketchfabAssetBrowserWindow::HandleSortByComboChanged)
							[
								SNew(STextBlock)
								.Text(this, &SSketchfabAssetBrowserWindow::GetSortByComboText)
							]
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
				]

				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.Padding(2)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(2)
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(LOCTEXT("SSketchfabAssetBrowserWindow_Next", "Load more"))
						.OnClicked(this, &SSketchfabAssetBrowserWindow::OnNext)
					]
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
					.OnAssetSelected(this, &SSketchfabAssetBrowserWindow::OnAssetsSelected)
					.OnAssetsActivated(this, &SSketchfabAssetBrowserWindow::OnAssetsActivated)
					.OnGetAssetContextMenu(this, &SSketchfabAssetBrowserWindow::OnGetAssetContextMenu)
					.AllowDragging(true)
				]
			]
		]

		+ SVerticalBox::Slot()
		.Padding(0, 2, 0, 2)
		.AutoHeight()
		[
			SNew(SBorder)
			.Padding(FMargin(3))
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				//.Padding(2)
				[

					SNew(SUniformGridPanel)
					.SlotPadding(2)
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(STextBlock)
						.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"), 16))
						//.ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.9f)) // send color and alpha R G B A
						.Visibility(this, &SSketchfabAssetBrowserWindow::ShouldDisplayUpgradeToPro)
						.Text(LOCTEXT("SSketchfabAssetBrowserWindow_UpgradeProTxt", "Gain full API access to your personal library of 3D models"))
						.Justification(ETextJustify::Center)
					]
					+ SUniformGridPanel::Slot(0, 1)
					.HAlign(HAlign_Center)
					[
					SNew(SButton)
					.Visibility(this, &SSketchfabAssetBrowserWindow::ShouldDisplayUpgradeToPro)
					.Text(LOCTEXT("SSketchfabAssetBrowserWindow_UpgradeProBtn", "Upgrade to PRO"))
					.OnClicked(this, &SSketchfabAssetBrowserWindow::OnUpgradeToPro)
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
						.Text(LOCTEXT("SSketchfabAssetBrowserWindow_ClearCache", "Clear Cache"))
						.OnClicked(this, &SSketchfabAssetBrowserWindow::OnClearCache)
					]
				]

				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.Padding(2)
				[
					SNew(SUniformGridPanel)
					.SlotPadding(2)
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Visibility(this, &SSketchfabAssetBrowserWindow::GetNewVersionButtonVisibility)
						.Text(FText::FromString("Get new Version"))
						.OnClicked(this, &SSketchfabAssetBrowserWindow::GetLatestPluginVersion)
					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(STextBlock)
						.Justification(ETextJustify::Center)
						.Margin(FMargin(0.0, 4.0, 0.0, 2.0))
						.Text(this, &SSketchfabAssetBrowserWindow::GetCurrentVersionText)
					]
				]
			]
		]
	];


	//Create our own drag and drop handler for the Content Browser so we can drag and drop our assets into the main Content Browser
	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetViewDragAndDropExtender>& ExtenderList = ContentBrowserModule.GetAssetViewDragAndDropExtenders();
	FAssetViewDragAndDropExtender DragAndDrop(
		FAssetViewDragAndDropExtender::FOnDropDelegate::CreateRaw(this, &SSketchfabAssetBrowserWindow::OnContentBrowserDrop),
		FAssetViewDragAndDropExtender::FOnDragOverDelegate::CreateRaw(this, &SSketchfabAssetBrowserWindow::OnContentBrowserDragOver),
		FAssetViewDragAndDropExtender::FOnDragLeaveDelegate::CreateRaw(this, &SSketchfabAssetBrowserWindow::OnContentBrowserDragLeave)
	);
	ExtenderList.Add(DragAndDrop);

	GetCategories();
	OnSearchPressed();
}

FString SSketchfabAssetBrowserWindow::GetModelZipFileName(const FString &ModelUID)
{
	return CacheFolder + ModelUID + ".zip";
}

void SSketchfabAssetBrowserWindow::DownloadModel(const FString &ModelUID, const FDateTime &ModelPublishedAt)
{
	if (LoggedInUserDisplayName.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("SSketchfabAssetBrowserWindow_ModelDoubleClick", "Sketchfab_NotLoggedIn", "You must be logged in to download models."));
		if (Window.IsValid())
		{
			Window.Pin()->BringToFront(true);
		}
	}

	if (LoggedInUserDisplayName.IsEmpty() || Token.IsEmpty() || ModelUID.IsEmpty())
	{
		return;
	}

	FString fileName = GetModelZipFileName(ModelUID);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	bool download = ShouldDownloadFile(fileName, ModelPublishedAt);
	if (!download)
		return;

	//TODO: Check to see if its already scheduled to be downloaded, or is already downloading.
	if (ModelsDownloading.Find(ModelUID))
	{
		return;
	}

	ModelsDownloading.Add(ModelUID);

	AssetViewPtr->DownloadProgress(ModelUID, 1e-8);
	AssetViewPtr->NeedRefresh();

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

void SSketchfabAssetBrowserWindow::OnAssetsSelected(const FSketchfabAssetData &SelectedAsset)
{
}

FReply SSketchfabAssetBrowserWindow::OnDownloadSelected()
{
	if (LoggedInUserDisplayName.IsEmpty())
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
			DownloadModel(AssetData.ModelUID.ToString(), AssetData.ModelPublishedAt);
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

EVisibility SSketchfabAssetBrowserWindow::GetNewVersionButtonVisibility() const
{
	if (!LatestPluginVersion.IsEmpty() && LatestPluginVersion != CurrentPluginVersion)
	{
		return EVisibility::Visible;
	}
	else
	{
		return EVisibility::Hidden;
	}
}

FText SSketchfabAssetBrowserWindow::GetCurrentVersionText() const
{
	FString versionState;
	if(LatestPluginVersion.IsEmpty())
	{
		versionState = "";
	}
	else if (CurrentPluginVersion == LatestPluginVersion)
	{
		versionState = "(up to date)";
	}
	else
	{
		versionState = "(outdated)";
	}

	return FText::FromString("Sketchfab plugin version " + CurrentPluginVersion + " " + versionState);
}

FText SSketchfabAssetBrowserWindow::GetLoginText() const
{
	if (!LoggedInUserDisplayName.IsEmpty())
	{
		FString text = "Logged in as " + LoggedInUserDisplayName + " " + LoggedInUserAccountType;
        return FText::FromString(text);
	}
	else
	{
		return LOCTEXT("SSketchfabAssetBrowserWindow_GetLoginText", "You're not logged in");
	}
}

FText SSketchfabAssetBrowserWindow::GetLoginButtonText() const
{
	if (!LoggedInUserDisplayName.IsEmpty())
	{
		return LOCTEXT("SSketchfabAssetBrowserWindow_OnLogin", "Logout");
	}
	else
	{
		return LOCTEXT("SSketchfabAssetBrowserWindow_OnLogin", "Login");
	}
}

FText SSketchfabAssetBrowserWindow::GetMyModelText() const
{
	if(IsLoggedUserPro)
	{
		return LOCTEXT("SSketchfabAssetBrowserWindow_Search_MyModels", "My models" );
	}
	else
	{
		return LOCTEXT("SSketchfabAssetBrowserWindow_Search_MyModels", "My models (PRO only)" );
	}
}

FReply SSketchfabAssetBrowserWindow::OnSearchPressed()
{
	AssetViewPtr->FlushThumbnails();

	FString url = bSearchMyModels ? "https://api.sketchfab.com/v3/me/search?type=models&downloadable=true" : "https://api.sketchfab.com/v3/search?type=models&downloadable=true";

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
	switch (SortByIndex)
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
	switch (FaceCountIndex)
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

	if (CategoryIndex > 0 && CategoryIndex < Categories.Num())
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
	if (!LoggedInUserDisplayName.IsEmpty())
	{
		Token = "";
		LoggedInUserName = "";
		LoggedInUserDisplayName = "";

		TSharedPtr<IWebBrowserCookieManager> cookieMan = IWebBrowserModule::Get().GetSingleton()->GetCookieManager();
		cookieMan->DeleteCookies("sketchfab.com");

		//FString url = "https://sketchfab.com/logout";
		//DoLoginLogout(url);
		return FReply::Handled();
	}
	else
	{
		FString url = "https://sketchfab.com/oauth2/authorize/?state=123456789&response_type=token&client_id=lZklEgZh9G5LcFqiCUUXTzXyNIZaO7iX35iXODqO";
		DoLoginLogout(url);
		return FReply::Handled();
	}

}

FReply SSketchfabAssetBrowserWindow::OnUpgradeToPro()
{
	{
		FString url = "https://sketchfab.com/plans?utm_source=unreal-plugin&utm_medium=plugin&utm_campaign=download-api-pro-cta";
		FPlatformMisc::OsExecute(TEXT("open"), *url);
		return FReply::Handled();
	}
}

void SSketchfabAssetBrowserWindow::DoLoginLogout(const FString &url)
{
	if (OAuthWindowPtr.IsValid())
	{
		OAuthWindowPtr->BringToFront();
	}
	else
	{
		TSharedPtr<SWindow> ParentWindow;

		if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
		{
			IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
			ParentWindow = MainFrame.GetParentWindow();
		}

		OAuthWindowPtr = SNew(SWindow)
			.Title(LOCTEXT("SketchfabAssetBrowser_LoginWindow", "Sketchfab Login Window"))
			.SizingRule(ESizingRule::FixedSize)
			.ClientSize(FVector2D(400, 600));

		OAuthWindowPtr->SetOnWindowClosed(FOnWindowClosed::CreateRaw(this, &SSketchfabAssetBrowserWindow::OnOAuthWindowClosed));

		TSharedPtr<SOAuthWebBrowser> OAuthBrowser;
		OAuthWindowPtr->SetContent
		(
			SAssignNew(OAuthBrowser, SOAuthWebBrowser)
			.ParentWindow(OAuthWindowPtr)
			.InitialURL(url)
			.OnUrlChanged(this, &SSketchfabAssetBrowserWindow::OnUrlChanged)
		);

		FWidgetPath WidgetPath;
		TSharedPtr<SWindow> CurrentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared(), WidgetPath);
		FSlateApplication::Get().AddWindowAsNativeChild(OAuthWindowPtr.ToSharedRef(), CurrentWindow.ToSharedRef());
	}

}

void SSketchfabAssetBrowserWindow::OnOAuthWindowClosed(const TSharedRef<SWindow>& InWindow)
{
	OAuthWindowPtr = NULL;
}

FReply SSketchfabAssetBrowserWindow::OnLogout()
{
	Token = "";
	LoggedInUserName = "";
	LoggedInUserDisplayName = "";

	TSharedPtr<IWebBrowserCookieManager> cookieMan = IWebBrowserModule::Get().GetSingleton()->GetCookieManager();
	cookieMan->DeleteCookies("sketchfab.com");

	//FString url = "https://sketchfab.com/logout";
	//DoLoginLogout(url);
	return FReply::Handled();
}

bool SSketchfabAssetBrowserWindow::OnLoginEnabled() const
{
	if (Token.IsEmpty() || LoggedInUserDisplayName.IsEmpty())
	{
		return true;
	}
	return false;
}

bool SSketchfabAssetBrowserWindow::OnLogoutEnabled() const
{
	return !OnLoginEnabled();
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
	FString Folder = GetSketchfabCacheDir();

	FString msg = "Delete all files in the Sketchfab cache folder '" + Folder + "' ?";
	if (FMessageDialog::Open(EAppMsgType::OkCancel, FText::FromString(msg)) == EAppReturnType::Ok)
	{
		AssetViewPtr->FlushThumbnails();

		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (PlatformFile.DirectoryExists(*Folder))
		{
			PlatformFile.DeleteDirectoryRecursively(*Folder);
			PlatformFile.CreateDirectory(*Folder);
		}
		OnSearchPressed();
	}

	if (Window.IsValid())
	{
		Window.Pin()->BringToFront(true);
	}

	return FReply::Handled();
}

FReply SSketchfabAssetBrowserWindow::CheckLatestPluginVersion()
{
	FSketchfabTaskData TaskData;
	TaskData.StateLock = new FCriticalSection();

	TSharedPtr<FSketchfabTask> Task = MakeShareable(new FSketchfabTask(TaskData));
	Task->SetState(SRS_CHECK_LATEST_VERSION);
	Task->OnCheckLatestVersion().BindRaw(this, &SSketchfabAssetBrowserWindow::OnCheckLatestVersion);
	Task->OnTaskFailed().BindRaw(this, &SSketchfabAssetBrowserWindow::OnTaskFailed);
	Task->CheckLatestPluginVersion();
	FSketchfabRESTClient::Get()->AddTask(Task);

	return FReply::Handled();
}

FReply SSketchfabAssetBrowserWindow::GetLatestPluginVersion()
{
	FString Command = "https://github.com/sketchfab/Sketchfab-Unreal/releases/latest";
	FPlatformMisc::OsExecute(TEXT("open"), *Command);
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
			if (OAuthWindowPtr.IsValid())
			{
				OAuthWindowPtr->RequestDestroyWindow();
			}

			Token = accesstoken;
			GetUserData();
		}
		return;
	}

	leftBlock = TEXT("https://sketchfab.com/?logged_out=1");
	if (data.Contains(leftBlock))
	{
		if (OAuthWindowPtr.IsValid())
		{
			OAuthWindowPtr->RequestDestroyWindow();
		}
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

bool SSketchfabAssetBrowserWindow::IsSearchMyModelsAvailable() const
{
	return LoggedInUserName != "";
}

EVisibility SSketchfabAssetBrowserWindow::ShouldDisplayUpgradeToPro() const
{
	return bSearchMyModels && LoggedInUserName != "" && !IsLoggedUserPro ? EVisibility::Visible : EVisibility::Collapsed;
}

ECheckBoxState SSketchfabAssetBrowserWindow::IsSearchMyModelsChecked() const
{
	return bSearchMyModels ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SSketchfabAssetBrowserWindow::setDefaultParams(bool myModels)
{
	bSearchStaffPicked = !myModels;
	bSearchAnimated = false;

	// Restore Sort By
	CurrentSortByString = *SortByComboList[0].Get();
	SortByIndex = (int32)SORTBY_MostRecent;

	// Restore Category
	CurrentCategoryString = *CategoryComboList[0].Get();
	CategoryIndex = 0;

	// Restore Face Count
	CurrentFaceCountString = *FaceCountComboList[0].Get();
	FaceCountIndex = (int32)FACECOUNT_ALL;
}

void SSketchfabAssetBrowserWindow::OnSearchMyModelsCheckStateChanged(ECheckBoxState NewState)
{
	bSearchMyModels = (NewState == ECheckBoxState::Checked);
	setDefaultParams(bSearchMyModels);
	OnSearchPressed();
}

ECheckBoxState SSketchfabAssetBrowserWindow::IsSearchAnimatedChecked() const
{
	return bSearchAnimated ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SSketchfabAssetBrowserWindow::OnSearchAnimatedCheckStateChanged(ECheckBoxState NewState)
{
	bSearchAnimated = (NewState == ECheckBoxState::Checked);
	OnSearchPressed();
}

ECheckBoxState SSketchfabAssetBrowserWindow::IsSearchStaffPickedChecked() const
{
	return bSearchStaffPicked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SSketchfabAssetBrowserWindow::OnSearchStaffPickedCheckStateChanged(ECheckBoxState NewState)
{
	bSearchStaffPicked = (NewState == ECheckBoxState::Checked);
	OnSearchPressed();
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


// Category
TSharedRef<SWidget> SSketchfabAssetBrowserWindow::GenerateCategoryComboItem(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock)
		.Text(FText::FromString(*InItem))
		.Margin(FMargin(4, 2));
}

void SSketchfabAssetBrowserWindow::HandleCategoryComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	for (int32 i = 0; i < CategoryComboList.Num(); i++)
	{
		if (Item == CategoryComboList[i])
		{
			CurrentCategoryString = *Item.Get();
			CategoryIndex = i;
			OnSearchPressed();
		}
	}
}

FText SSketchfabAssetBrowserWindow::GetCategoryComboText() const
{
	return FText::FromString(CurrentCategoryString);
}


// Sort By
TSharedRef<SWidget> SSketchfabAssetBrowserWindow::GenerateSortByComboItem(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock)
		.Text(FText::FromString(*InItem))
		.Margin(FMargin(4, 2));
}

void SSketchfabAssetBrowserWindow::HandleSortByComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	for (int32 i = 0; i < SortByComboList.Num(); i++)
	{
		if (Item == SortByComboList[i])
		{
			CurrentSortByString = *Item.Get();
			SortByIndex = i;
			OnSearchPressed();
		}
	}
}

FText SSketchfabAssetBrowserWindow::GetSortByComboText() const
{
	return FText::FromString(CurrentSortByString);
}


// FaceCount
TSharedRef<SWidget> SSketchfabAssetBrowserWindow::GenerateFaceCountComboItem(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock)
		.Text(FText::FromString(*InItem))
		.Margin(FMargin(4, 2));
}

void SSketchfabAssetBrowserWindow::HandleFaceCountComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	for (int32 i = 0; i < FaceCountComboList.Num(); i++)
	{
		if (Item == FaceCountComboList[i])
		{
			CurrentFaceCountString = *Item.Get();
			FaceCountIndex = i;
			OnSearchPressed();
		}
	}
}

FText SSketchfabAssetBrowserWindow::GetFaceCountComboText() const
{
	return FText::FromString(CurrentFaceCountString);
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

	bool download = ShouldDownloadFile(FileName, data.ModelPublishedAt);
	if (download)
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
	LoggedInUserName = InTask.TaskData.UserSession.UserName;
	LoggedInUserDisplayName = InTask.TaskData.UserSession.UserDisplayName;
	if(InTask.TaskData.UserSession.UserPlan == ACCOUNT_PRO)
	{
		LoggedInUserAccountType = "(PRO)";
		IsLoggedUserPro = true;
	}
	else if(InTask.TaskData.UserSession.UserPlan == ACCOUNT_PREMIUM)
	{
		LoggedInUserAccountType = "(PREMIUM)";
		IsLoggedUserPro = true;
	}
	else if(InTask.TaskData.UserSession.UserPlan == ACCOUNT_BUSINESS)
	{
		LoggedInUserAccountType = "(BIZ)";
		IsLoggedUserPro = true;
	}
	else if(InTask.TaskData.UserSession.UserPlan == ACCOUNT_ENTERPRISE)
	{
		LoggedInUserAccountType = "(ENT)";
		IsLoggedUserPro = true;
	}
	else
	{
		LoggedInUserAccountType = "(FREE)";
		IsLoggedUserPro = false;
	}
}

void SSketchfabAssetBrowserWindow::OnUserThumbnailDownloaded(const FSketchfabTask& InTask)
{
	AssetViewPtr->NeedRefresh();
}

void SSketchfabAssetBrowserWindow::OnThumbnailDownloaded(const FSketchfabTask& InTask)
{
	AssetViewPtr->NeedRefresh();
}

void SSketchfabAssetBrowserWindow::OnCheckLatestVersion(const FSketchfabTask& InTask)
{
	LatestPluginVersion = InTask.TaskData.LatestPluginVersion;
}

void SSketchfabAssetBrowserWindow::OnSearch(const FSketchfabTask& InTask)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	for (int32 Index = 0; Index < InTask.SearchData.Num(); Index++)
	{
		TSharedPtr<FSketchfabTaskData> Data = InTask.SearchData[Index];

		AssetViewPtr->ForceCreateNewAsset(Data);

		FString jpg = Data->ThumbnailUID + ".jpg";
		FString FileName = Data->CacheFolder / jpg;
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		bool downloadThumbnail = ShouldDownloadFile(FileName, Data->ModelPublishedAt);
		if (downloadThumbnail)
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
	Categories.Empty();
	FSketchfabCategory allCat;
	allCat.name = TEXT("All");
	Categories.Add(allCat);

	CategoryComboList.Empty();
	CategoryComboList.Add(MakeShared<FString>(TEXT("All")));

	for (auto &a : InTask.TaskData.Categories)
	{
		Categories.Add(a);
		CategoryComboList.Add(MakeShared<FString>(a.name));
	}

	CategoriesComboBox->RefreshOptions();
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

	//This is a bit of a hack to get the license data into the drag and drop window.
	AssetViewPtr->SetLicence(InTask.TaskData.ModelUID, InTask.TaskData.LicenceType, InTask.TaskData.LicenceInfo);
}

void SSketchfabAssetBrowserWindow::OnGetBigThumbnail(const FSketchfabTask& InTask)
{
	if (AssetWindow.IsValid())
	{
		AssetWindow->SetThumbnail(InTask);
	}
}

bool SSketchfabAssetBrowserWindow::ShouldDownloadFile(const FString &FileName, const FDateTime &ModelPublishedAt)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	bool download = false;
	bool fileExists = PlatformFile.FileExists(*FileName);
	if (fileExists)
	{
		FDateTime modifiedTime = PlatformFile.GetTimeStamp(*FileName);
		if (ModelPublishedAt > modifiedTime)
		{
			download = true;
		}
	}
	else
	{
		download = true;
	}
	return download;
}

// Content Browser Delegate Callbacks
bool SSketchfabAssetBrowserWindow::OnContentBrowserDrop(const FAssetViewDragAndDropExtender::FPayload &PayLoad)
{
	if (PayLoad.DragDropOp.IsValid() && PayLoad.DragDropOp->IsOfType<FSketchfabDragDropOperation>())
	{
		TSharedPtr<FSketchfabDragDropOperation> DragDropOp = StaticCastSharedPtr<FSketchfabDragDropOperation>(PayLoad.DragDropOp);

		DragDropOp->SetDecoratorVisibility(false);

		//Skip if we haven't yet got the licence information
		if (DragDropOp->DraggedAssets[0].LicenceInfo.IsEmpty())
		{
			return true;
		}

		TSharedPtr<SWindow> ParentWindow;

		if (FModuleManager::Get().IsModuleLoaded("MainFrame"))
		{
			IMainFrameModule& MainFrame = FModuleManager::LoadModuleChecked<IMainFrameModule>("MainFrame");
			ParentWindow = MainFrame.GetParentWindow();
		}

		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(LOCTEXT("SketchfabAcceptLicenseWindow", "License Acceptance"))
			.SizingRule(ESizingRule::Autosized);


		TSharedPtr<SAcceptLicenseWindow> AcceptLicenceWindow;
		Window->SetContent
		(
			SAssignNew(AcceptLicenceWindow, SAcceptLicenseWindow)
			.AssetData(DragDropOp->DraggedAssets[0])
			.WidgetWindow(Window)
		);

		FSlateApplication::Get().AddModalWindow(Window, ParentWindow, false);

		if (AcceptLicenceWindow->ShouldImport())
		{
			TArray<FString> Assets;
			Assets.Add(DragDropOp->DraggedAssetPaths[0]);

			FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
			TArray<UObject*> AddedFiles = AssetToolsModule.Get().ImportAssets(Assets, PayLoad.PackagePaths[0].ToString());

			//Now Rename the files
			if (AddedFiles.Num() == 1)
			{
				FString Name = DragDropOp->DraggedAssets[0].ModelName.ToString();
				AddedFiles[0]->Rename(*Name);
			}
		}

		return true;
	}

	return false;
}

bool SSketchfabAssetBrowserWindow::OnContentBrowserDragOver(const FAssetViewDragAndDropExtender::FPayload &PayLoad)
{
	if (PayLoad.DragDropOp.IsValid() && PayLoad.DragDropOp->IsOfType<FSketchfabDragDropOperation>())
	{
		TSharedPtr<FSketchfabDragDropOperation> DragDropOp = StaticCastSharedPtr<FSketchfabDragDropOperation>(PayLoad.DragDropOp);
		DragDropOp->SetCursorOverride(EMouseCursor::Hand);
		return true;
	}

	return false;
}

bool SSketchfabAssetBrowserWindow::OnContentBrowserDragLeave(const FAssetViewDragAndDropExtender::FPayload &PayLoad)
{
	if (PayLoad.DragDropOp.IsValid() && PayLoad.DragDropOp->IsOfType<FSketchfabDragDropOperation>())
	{
		TSharedPtr<FSketchfabDragDropOperation> DragDropOp = StaticCastSharedPtr<FSketchfabDragDropOperation>(PayLoad.DragDropOp);
		DragDropOp->SetCursorOverride(EMouseCursor::Default);
		return true;
	}

	return false;
}


#undef LOCTEXT_NAMESPACE

