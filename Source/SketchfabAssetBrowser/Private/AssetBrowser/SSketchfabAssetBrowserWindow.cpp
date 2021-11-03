// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SSketchfabAssetBrowserWindow.h"
#include "EditorAssetLibrary.h"
//#include <EditorScriptingUtilities/Public/EditorAssetLibrary.h>

#define LOCTEXT_NAMESPACE "SketchfabAssetBrowser"

DEFINE_LOG_CATEGORY(LogSketchfabAssetBrowserWindow);

void SSketchfabAssetBrowserWindow::UpdateSearchDomainList() {

}

void SSketchfabAssetBrowserWindow::GenerateSearchDomainList() {
	SearchDomainIndex = (int32)SEARCHDOMAIN_Default;

	if (SearchDomainComboList.Num() > 0)
		SearchDomainComboList.Empty();

	if (UsesOrgProfile) {
		SearchDomainIndex = 0;
		
		CurrentSearchDomainString = TEXT("All organization");
		SearchDomainComboList.Add(MakeShared<FString>(CurrentSearchDomainString));

		for (int32 i = 0; i < Orgs[OrgIndex].Projects.Num() ; i++)
		{
			SearchDomainComboList.Add(MakeShared<FString>(Orgs[OrgIndex].Projects[i].name));
		}
	}
	else {
		CurrentSearchDomainString = TEXT("All site");
		for (int32 i = 0; i < (int32)SEARCHDOMAIN_UNDEFINED; i++)
		{
			switch (i)
			{
			case SEARCHDOMAIN_Default:
			{
				SearchDomainComboList.Add(MakeShared<FString>(TEXT("All site")));
			}
			break;
			case SEARCHDOMAIN_Own:
			{
				SearchDomainComboList.Add(MakeShared<FString>(TEXT("Own Models (PRO)")));
			}
			break;
			case SEARCHDOMAIN_Store:
			{
				SearchDomainComboList.Add(MakeShared<FString>(TEXT("Store purchases")));
			}
			break;
			}
		}
	}
	
	if(SearchDomainComboBox.IsValid())
		SearchDomainComboBox->RefreshOptions();
}

void SSketchfabAssetBrowserWindow::Construct(const FArguments& InArgs)
{
	initWindow();
	Window = InArgs._WidgetWindow;

	bSearchAnimated = false;
	bSearchStaffPicked = true;

	CategoryIndex = 0;
	CurrentCategoryString = TEXT("All");

	GenerateSearchDomainList();

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

	TSharedRef<SVerticalBox> LoginNode = CreateLoginArea(1);
	TSharedRef<SVerticalBox> FooterNode = CreateFooterArea(1);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 2)
		[
			LoginNode
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

					// Search domain
					+SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(2.0f, 0.0f, 2.0f, 0.0f)
					[
						SAssignNew(SearchDomainComboBox, SComboBox<TSharedPtr<FString>>)
						.OptionsSource(&SearchDomainComboList)
						.OnGenerateWidget(this, &SSketchfabAssetBrowserWindow::GenerateSearchDomainComboItem)
						.OnSelectionChanged(this, &SSketchfabAssetBrowserWindow::HandleSearchDomainComboChanged)
						[
							SNew(STextBlock)
							.Text(this, &SSketchfabAssetBrowserWindow::GetSearchDomainComboText)
						]
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
			.Visibility(this, &SSketchfabAssetBrowserWindow::ShouldDisplayFilters)
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
						.IsEnabled(this, &SSketchfabAssetBrowserWindow::hasMoreResults)
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
					//.OnGetAssetContextMenu(this, &SSketchfabAssetBrowserWindow::OnGetAssetContextMenu)
					.AllowDragging(true)
				]
			]
		]

		// UpgradeToPro
		+ SVerticalBox::Slot()
		.Padding(0, 2, 0, 2)
		.AutoHeight()
		[
			SNew(SBorder)
			.Visibility(this, &SSketchfabAssetBrowserWindow::ShouldDisplayUpgradeToPro)
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
						.Text(LOCTEXT("SSketchfabAssetBrowserWindow_UpgradeProTxt", "Access your personal library of 3D models"))
						.Justification(ETextJustify::Center)
					]
					+ SUniformGridPanel::Slot(0, 1)
					.HAlign(HAlign_Center)
					[
					SNew(SButton)
					.Text(LOCTEXT("SSketchfabAssetBrowserWindow_UpgradeProBtn", "Upgrade to PRO"))
					.OnClicked(this, &SSketchfabAssetBrowserWindow::OnUpgradeToPro)
					]
				]
			]
		]

		// No results found
		+ SVerticalBox::Slot()
		.Padding(0, 2, 0, 2)
		.AutoHeight()
		[
			SNew(SBorder)
			.Visibility(this, &SSketchfabAssetBrowserWindow::ShouldDisplayEmptyResults)
			.Padding(FMargin(3))
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[

					SNew(SUniformGridPanel)
					.SlotPadding(2)
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(STextBlock)
						.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Light.ttf"), 16))
						.Text(LOCTEXT("SSketchfabAssetBrowserWindow_EmptyResultsTxt", "No results found"))
						.Justification(ETextJustify::Center)
					]
				]
			]
		]

		// No store purchases, visit the store
		+ SVerticalBox::Slot()
		.Padding(0, 2, 0, 2)
		.AutoHeight()
		[
			SNew(SBorder)
			.Visibility(this, &SSketchfabAssetBrowserWindow::ShouldDisplayVisitStore)
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
						.Text(LOCTEXT("SSketchfabAssetBrowserWindow_VisitStoreTxt", "No results found in your purchases"))
						.Justification(ETextJustify::Center)
					]
					+ SUniformGridPanel::Slot(0, 1)
					.HAlign(HAlign_Center)
					[
					SNew(SButton)
					.Text(LOCTEXT("SSketchfabAssetBrowserWindow_VisitStoreBtn", "Visit the Store"))
					.OnClicked(this, &SSketchfabAssetBrowserWindow::OnVisitStore)
					]
				]
			]
		]

		
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 0, 0, 2)
		[
			FooterNode
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

/*
void SSketchfabAssetBrowserWindow::OnUseOrgProfileCheckStateChanged(ECheckBoxState NewState) {
	UsesOrgProfile = (NewState == ECheckBoxState::Checked);
	GenerateSearchDomainList();
	OnSearchPressed();
}
*/

void SSketchfabAssetBrowserWindow::OnOrgChanged() {
	GenerateSearchDomainList();
	OnSearchPressed();
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

	// If the model was downloaded, then we don't ask for the user to log in
	FString fileName = GetModelZipFileName(ModelUID);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	bool download = ShouldDownloadFile(fileName, ModelPublishedAt);
	if (!download)
		return;

	if (LoggedInUserDisplayName.IsEmpty() || Token.IsEmpty() || ModelUID.IsEmpty())
	{
		return;
	}

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
	TaskData.OrgUID = UsesOrgProfile ? Orgs[OrgIndex].uid : "";
	TaskData.OrgModel = UsesOrgProfile;
	uint64* size = ModelSizesMap.Find(ModelUID);
	TaskData.ModelSize = *size;
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

EVisibility SSketchfabAssetBrowserWindow::ShouldDisplayClearCache() const 
{ 
	return EVisibility::Visible; 
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

/*
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
*/

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

	FString url = "https://api.sketchfab.com/v3";

	// Adapt the base url according to the search domain
	if (UsesOrgProfile) {
		url += "/orgs/" + Orgs[OrgIndex].uid + "/models?isArchivesReady=true";
		if (SearchDomainIndex != 0) { // Specific project
			url += "&projects=" + Orgs[OrgIndex].Projects[SearchDomainIndex - 1].uid;
		}
	}
	else {
		switch (SearchDomainIndex)
		{
			case SEARCHDOMAIN_Default:
			{
				url += "/search?type=models&downloadable=true";
				break;
			}
			case SEARCHDOMAIN_Own:
			{
				url += "/me/search?type=models&downloadable=true";
				break;
			}
			case SEARCHDOMAIN_Store:
			{
				url += "/me/models/purchases?";
				break;
			}
		}
	}



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

	if (!UsesOrgProfile) {
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
			//Don't display point clouds
			url += "&min_face_count=1";
		}
		break;
		case FACECOUNT_0_10:
		{
			url += "&min_face_count=1&max_face_count=10000";
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

FReply SSketchfabAssetBrowserWindow::OnVisitStore()
{
	{
		OpenUrlInBrowser("https://sketchfab.com/store?utm_source=unreal-plugin&utm_medium=plugin&utm_campaign=store-cta");
		return FReply::Handled();
	}
}

bool SSketchfabAssetBrowserWindow::hasMoreResults() const
{
	return !NextURL.IsEmpty();
}

FReply SSketchfabAssetBrowserWindow::OnNext()
{
	Search(NextURL);
	return FReply::Handled();
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

EVisibility SSketchfabAssetBrowserWindow::ShouldDisplayFilters() const
{
	return (SearchDomainIndex == SEARCHDOMAIN_Store && !UsesOrgProfile) ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility SSketchfabAssetBrowserWindow::ShouldDisplayVisitStore() const
{
	return (SearchDomainIndex == SEARCHDOMAIN_Store && NResults == 0 && !UsesOrgProfile) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SSketchfabAssetBrowserWindow::ShouldDisplayEmptyResults() const
{
	return (SearchDomainIndex != SEARCHDOMAIN_Store && NResults == 0) ? EVisibility::Visible : EVisibility::Collapsed;
}

void SSketchfabAssetBrowserWindow::setDefaultParams()
{
	bSearchStaffPicked = true;
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

	TSharedPtr<SWindow> CurrentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
	FSlateApplication::Get().AddWindowAsNativeChild(ModelWindow, CurrentWindow.ToSharedRef());

	GetModelInfo(AssetData.ModelUID.ToString());
}

// Search domain
TSharedRef<SWidget> SSketchfabAssetBrowserWindow::GenerateSearchDomainComboItem(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock)
		.Text(FText::FromString(*InItem))
		.Margin(FMargin(4, 2));
}

void SSketchfabAssetBrowserWindow::HandleSearchDomainComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo)
{
	for (int32 i = 0; i < SearchDomainComboList.Num(); i++)
	{
		if (Item == SearchDomainComboList[i])
		{
			CurrentSearchDomainString = *Item.Get();
			SearchDomainIndex = i;
			OnSearchPressed();
		}
	}
}

FText SSketchfabAssetBrowserWindow::GetSearchDomainComboText() const
{
	return FText::FromString(CurrentSearchDomainString);
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
	TaskData.Token = Token;
	TaskData.OrgUID = UsesOrgProfile ? Orgs[OrgIndex].uid : "";
	TaskData.OrgModel = UsesOrgProfile;
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
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	for (int32 Index = 0; Index < InTask.SearchData.Num(); Index++)
	{
		TSharedPtr<FSketchfabTaskData> Data = InTask.SearchData[Index];

		AssetViewPtr->ForceCreateNewAsset(Data);

		FString jpg = Data->ThumbnailUID + ".jpg";
		FString FileName = Data->CacheFolder / jpg;

		// Add the model size to a map in order to keep the reference for reference while download is in progress
		ModelSizesMap.Add(Data->ModelUID, Data->ModelSize);

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
	NResults = InTask.SearchData.Num();
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

		// If we don't have the license info, the license was already accepted, let the user proceed.
		FString LicenseText;
		if (!DragDropOp->DraggedAssets[0].LicenceInfo.IsEmpty())
		{
			LicenseText = "By importing this model, you agree to the terms of the following license:\n"
				+ DragDropOp->DraggedAssets[0].LicenceType + "\n" + DragDropOp->DraggedAssets[0].LicenceInfo;
		}

		ISketchfabAssetBrowserModule& gltfModule = FModuleManager::Get().LoadModuleChecked<ISketchfabAssetBrowserModule>("SketchfabAssetBrowser");
		if (PayLoad.PackagePaths.Num() > 0)
		{
			gltfModule.CurrentModelName = FName(ObjectTools::SanitizeObjectName(DragDropOp->DraggedAssets[0].ModelName.ToString())).ToString();
			gltfModule.LicenseInfo = LicenseText;

			TArray<FString> Assets;
			Assets.Add(DragDropOp->DraggedAssetPaths[0]);

			FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
			TArray<UObject*> AddedFiles = AssetToolsModule.Get().ImportAssets(Assets, PayLoad.PackagePaths[0].ToString(), nullptr, true);
		}

		// Reset those info to in case of following direct gltf import
		gltfModule.LicenseInfo = "";
		gltfModule.CurrentModelName = "";

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

