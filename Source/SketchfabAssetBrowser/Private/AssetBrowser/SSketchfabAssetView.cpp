#include "SSketchfabAssetView.h"

#include "HAL/FileManager.h"
#include "UObject/UnrealType.h"
#include "Widgets/SOverlay.h"
#include "Engine/GameViewportClient.h"
#include "Factories/Factory.h"
#include "Framework/Commands/UIAction.h"
#include "Textures/SlateIcon.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "SlateOptMacros.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBorder.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SSlider.h"
#include "Framework/Docking/TabManager.h"
#include "EditorStyleSet.h"
#include "EditorReimportHandler.h"
#include "Settings/ContentBrowserSettings.h"
#include "Engine/Blueprint.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "FrontendFilterBase.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "ContentBrowserModule.h"
#include "ObjectTools.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Layout/SSplitter.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/FileHelper.h"
#include "Internationalization/BreakIterator.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "UObject/ConstructorHelpers.h"
#include "ImageLoader.h"
#include "SketchfabTask.h"

#define LOCTEXT_NAMESPACE "SSketchfabAssetView"

#define MAX_THUMBNAIL_SIZE 4096

FString GetSketchfabCacheDir()
{
	return FPaths::EngineUserDir() + TEXT("SketchfabCache/");
}

FString GetSketchfabAssetZipPath(const FSketchfabAssetData &AssetData)
{
	FString CacheFolder = GetSketchfabCacheDir();
	FString zipFile = CacheFolder / AssetData.ModelUID.ToString() + TEXT(".zip");
	return zipFile;
}

bool HasSketchAssetZipFile(const FSketchfabAssetData &AssetData)
{
	FString zipFile = GetSketchfabAssetZipPath(AssetData);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	return PlatformFile.FileExists(*zipFile);
}


void SSketchfabAssetView::Construct(const FArguments& InArgs)
{
	FillScale = 1.0f;

	// Get desktop metrics
	FDisplayMetrics DisplayMetrics;
	FSlateApplication::Get().GetDisplayMetrics(DisplayMetrics);

	const FVector2D DisplaySize(
		DisplayMetrics.PrimaryDisplayWorkAreaRect.Right - DisplayMetrics.PrimaryDisplayWorkAreaRect.Left,
		DisplayMetrics.PrimaryDisplayWorkAreaRect.Bottom - DisplayMetrics.PrimaryDisplayWorkAreaRect.Top);

	const float ThumbnailScaleRangeScalar = (DisplaySize.Y / 1080);

	// Create a thumbnail pool for rendering thumbnails
	AssetThumbnailPool = MakeShareable(new FSketchfabAssetThumbnailPool(1024, true /*InArgs._AreRealTimeThumbnailsAllowed*/));
	NumOffscreenThumbnails = 64;
	TileViewThumbnailResolution = 256;
	TileViewThumbnailSize = 128;
	TileViewThumbnailPadding = 5;

	TileViewNameHeight = 36;

	ThumbnailScaleSliderValue = InArgs._ThumbnailScale;

	if (!ThumbnailScaleSliderValue.IsBound())
	{
		ThumbnailScaleSliderValue = FMath::Clamp<float>(ThumbnailScaleSliderValue.Get(), 0.0f, 1.0f);
	}

	MinThumbnailScale = 0.2f * ThumbnailScaleRangeScalar;
	MaxThumbnailScale = 2.0f * ThumbnailScaleRangeScalar;

	bBulkSelecting = false;

	CurrentThumbnailSize = TileViewThumbnailSize;

	ThumbnailLabel = InArgs._ThumbnailLabel;
	OnAssetSelected = InArgs._OnAssetSelected;
	OnAssetsActivated = InArgs._OnAssetsActivated;
	OnGetAssetContextMenu = InArgs._OnGetAssetContextMenu;
	SelectionMode = InArgs._SelectionMode;
	OnGetCustomAssetToolTip = InArgs._OnGetCustomAssetToolTip;
	OnVisualizeAssetToolTip = InArgs._OnVisualizeAssetToolTip;
	OnAssetToolTipClosing = InArgs._OnAssetToolTipClosing;
	bShowBottomToolbar = InArgs._ShowBottomToolbar;
	bAllowDragging = InArgs._AllowDragging;
	bFillEmptySpaceInTileView = InArgs._FillEmptySpaceInTileView;

	TSharedRef<SVerticalBox> VerticalBox = SNew(SVerticalBox);

	ChildSlot
	[
		VerticalBox
	];

	// Assets area
	VerticalBox->AddSlot()
	.FillHeight(1.f)
	[
		SNew( SVerticalBox )

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew( SBox )
			.Visibility_Lambda([this] { return bIsWorking ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
			.HeightOverride( 2 )
			[
				SNew( SProgressBar )
				//.Percent( this, &SAssetView::GetIsWorkingProgressBarState )
				.Style( FEditorStyle::Get(), "WorkingBar" )
				.BorderPadding( FVector2D(0,0) )
			]
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SOverlay)

			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				// Container for the view types
				SAssignNew(ViewContainer, SBorder)
				.Padding(0)
				.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			]

			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Center)
			.Padding(FMargin(0, 14, 0, 0))
			[
				// A warning to display when there are no assets to show
				SNew( STextBlock )
				.Justification( ETextJustify::Center )
				//.Text( this, &SAssetView::GetAssetShowWarningText )
				//.Visibility( this, &SAssetView::IsAssetShowWarningTextVisible )
				.AutoWrapText( true )
			]

			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Bottom)
			.Padding(FMargin(24, 0, 24, 0))
				/*
			[
				// Asset discovery indicator
				AssetDiscoveryIndicator
			]
			*/
		]
	];

	CreateCurrentView();
}

void SSketchfabAssetView::CreateCurrentView()
{
	TileView.Reset();

	TSharedRef<SWidget> NewView = SNullWidget::NullWidget;
	TileView = CreateTileView();
	NewView = CreateShadowOverlay(TileView.ToSharedRef());

	ViewContainer->SetContent(NewView);
}


TSharedRef<SAssetTileView> SSketchfabAssetView::CreateTileView()
{
	return SNew(SAssetTileView)
		.SelectionMode(SelectionMode)
		.ListItemsSource(&FilteredAssetItems)
		.OnGenerateTile(this, &SSketchfabAssetView::MakeTileViewWidget)
 		.OnItemScrolledIntoView(this, &SSketchfabAssetView::ItemScrolledIntoView)
 		.OnContextMenuOpening(this, &SSketchfabAssetView::OnGetContextMenuContent)
 		.OnMouseButtonDoubleClick(this, &SSketchfabAssetView::OnListMouseButtonDoubleClick)
 		.OnSelectionChanged(this, &SSketchfabAssetView::AssetSelectionChanged)
		.ItemHeight(this, &SSketchfabAssetView::GetTileViewItemHeight)
		.ItemWidth(this, &SSketchfabAssetView::GetTileViewItemWidth);
}

TSharedRef<SWidget> SSketchfabAssetView::CreateShadowOverlay(TSharedRef<STableViewBase> Table)
{
	return SNew(SScrollBorder, Table)
		[
			Table
		];
}

float SSketchfabAssetView::GetThumbnailScale() const
{
	return ThumbnailScaleSliderValue.Get();
}

float SSketchfabAssetView::GetTileViewItemHeight() const
{
	return TileViewNameHeight + GetTileViewItemBaseHeight() * FillScale;
}

float SSketchfabAssetView::GetTileViewItemBaseHeight() const
{
	return (TileViewThumbnailSize + TileViewThumbnailPadding * 2) * FMath::Lerp(MinThumbnailScale, MaxThumbnailScale, GetThumbnailScale());
}

float SSketchfabAssetView::GetTileViewItemWidth() const
{
	return GetTileViewItemBaseWidth() * FillScale;
}

float SSketchfabAssetView::GetTileViewItemBaseWidth() const
{
	return (TileViewThumbnailSize + TileViewThumbnailPadding * 2) * FMath::Lerp(MinThumbnailScale, MaxThumbnailScale, GetThumbnailScale());
}

TSharedRef<ITableRow> SSketchfabAssetView::MakeTileViewWidget(TSharedPtr<FAssetViewItem> AssetItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	if (!ensure(AssetItem.IsValid()))
	{
		return SNew(STableRow<TSharedPtr<FAssetViewAsset>>, OwnerTable);
	}

	VisibleItems.Add(AssetItem);
	bPendingUpdateThumbnails = true;


	TSharedPtr<FAssetViewAsset> AssetItemAsAsset = StaticCastSharedPtr<FAssetViewAsset>(AssetItem);
	TSharedPtr<FSketchfabAssetThumbnail>* AssetThumbnailPtr = RelevantThumbnails.Find(AssetItemAsAsset);
	TSharedPtr<FSketchfabAssetThumbnail> AssetThumbnail;
	if (AssetThumbnailPtr)
	{
		AssetThumbnail = *AssetThumbnailPtr;
	}
	else
	{
		const float ThumbnailResolution = TileViewThumbnailResolution;
		AssetThumbnail = MakeShareable(new FSketchfabAssetThumbnail(AssetItemAsAsset->Data, AssetItemAsAsset->Data.ThumbnailWidth, AssetItemAsAsset->Data.ThumbnailHeight, AssetThumbnailPool));
		RelevantThumbnails.Add(AssetItemAsAsset, AssetThumbnail);
		AssetThumbnail->GetViewportRenderTargetTexture(); // Access the texture once to trigger it to render
	}

	TSharedPtr< STableRow<TSharedPtr<FAssetViewItem>> > TableRowWidget;
	SAssignNew(TableRowWidget, STableRow<TSharedPtr<FAssetViewItem>>, OwnerTable)
		.Cursor(bAllowDragging ? EMouseCursor::GrabHand : EMouseCursor::Default)
		.OnDragDetected(this, &SSketchfabAssetView::OnDraggingAssetItem);

	TSharedRef<SSKAssetTileItem> Item =
		SNew(SSKAssetTileItem)
		.AssetThumbnail(AssetThumbnail)
		.AssetItem(AssetItem)
		.ThumbnailPadding(TileViewThumbnailPadding)
		.ItemWidth(this, &SSketchfabAssetView::GetTileViewItemWidth)
		//.OnRenameBegin(this, &SAssetView::AssetRenameBegin)
		//.OnRenameCommit(this, &SAssetView::AssetRenameCommit)
		//.OnVerifyRenameCommit(this, &SAssetView::AssetVerifyRenameCommit)
		//.OnItemDestroyed(this, &SAssetView::AssetItemWidgetDestroyed)
		//.ShouldAllowToolTip(this, &SSketchfabAssetView::ShouldAllowToolTips)
		//.HighlightText(HighlightedText)
		//.ThumbnailEditMode(this, &SAssetView::IsThumbnailEditMode)
		.ThumbnailLabel(ThumbnailLabel)
		//.ThumbnailHintColorAndOpacity(this, &SAssetView::GetThumbnailHintColorAndOpacity)
		//.AllowThumbnailHintLabel(AllowThumbnailHintLabel)
		.IsSelected(FIsSelected::CreateSP(TableRowWidget.Get(), &STableRow<TSharedPtr<FAssetViewItem>>::IsSelectedExclusively));
		//.OnGetCustomAssetToolTip(OnGetCustomAssetToolTip)
		//.OnVisualizeAssetToolTip(OnVisualizeAssetToolTip)
		//.OnAssetToolTipClosing(OnAssetToolTipClosing);
	TableRowWidget->SetContent(Item);

	return TableRowWidget.ToSharedRef();
}

void SSketchfabAssetView::OnListMouseButtonDoubleClick(TSharedPtr<FAssetViewItem> AssetItem)
{
	if (!ensure(AssetItem.IsValid()))
	{
		return;
	}

	TArray<FSketchfabAssetData> ActivatedAssets;
	ActivatedAssets.Add(StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data);
	OnAssetsActivated.ExecuteIfBound(ActivatedAssets, EAssetTypeActivationMethod::DoubleClicked);
}

FReply SSketchfabAssetView::OnDraggingAssetItem(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bAllowDragging)
	{
		TSharedRef<FSketchfabDragDropOperation> Operation = MakeShareable(new FSketchfabDragDropOperation);
		Operation->AssetThumbnailPool = AssetThumbnailPool;

		// Work out which assets to drag
		{
			TArray<FSketchfabAssetData> AssetDataList = GetSelectedAssets();
			for (const FSketchfabAssetData& AssetData : AssetDataList)
			{
				// Skip invalid assets and redirectors
				if (AssetData.IsValid() && HasSketchAssetZipFile(AssetData))
				{
					FSketchfabAssetData DataCopy = AssetData;
					LicenceDataInfo *data = LicenceData.Find(AssetData.ModelUID.ToString());
					if (data)
					{
						DataCopy.LicenceInfo = data->LicenceInfo;
						DataCopy.LicenceType = data->LicenceType;
					}

					Operation->DraggedAssets.Add(DataCopy);
					Operation->DraggedAssetPaths.Add(GetSketchfabAssetZipPath(DataCopy));
				}
			}
		}

		// Use the custom drag handler?
		if (Operation->DraggedAssets.Num() > 0 && FEditorDelegates::OnAssetDragStarted.IsBound())
		{
			return FReply::Handled();
		}

		if ((Operation->DraggedAssets.Num() > 0 || Operation->DraggedAssetPaths.Num() > 0) && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			Operation->Construct();
			return FReply::Handled().BeginDragDrop(Operation);
		}
	}

	return FReply::Unhandled();
}

void SSketchfabAssetView::AssetSelectionChanged(TSharedPtr< struct FAssetViewItem > AssetItem, ESelectInfo::Type SelectInfo)
{
	if (!bBulkSelecting)
	{
		if (AssetItem.IsValid() && AssetItem->GetType() != EAssetItemType::Folder)
		{
			OnAssetSelected.ExecuteIfBound(StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data);
		}
		else
		{
			OnAssetSelected.ExecuteIfBound(FSketchfabAssetData());
		}
	}
}

void SSketchfabAssetView::ItemScrolledIntoView(TSharedPtr<struct FAssetViewItem> AssetItem, const TSharedPtr<ITableRow>& Widget)
{
	if (AssetItem->bRenameWhenScrolledIntoview)
	{
		// Make sure we have window focus to avoid the inline text editor from canceling itself if we try to click on it
		// This can happen if creating an asset opens an intermediary window which steals our focus,
		// eg, the blueprint and slate widget style class windows (TTP# 314240)
		TSharedPtr<SWindow> OwnerWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
		if (OwnerWindow.IsValid())
		{
			OwnerWindow->BringToFront();
		}

		//AwaitingRename = AssetItem;
	}
}

TSharedPtr<SWidget> SSketchfabAssetView::OnGetContextMenuContent()
{
	if (CanOpenContextMenu())
	{
		return OnGetAssetContextMenu.Execute(GetSelectedAssets());
	}

	return NULL;
}

bool SSketchfabAssetView::CanOpenContextMenu() const
{
	if (!OnGetAssetContextMenu.IsBound())
	{
		// You can only a summon a context menu if one is set up
		return false;
	}

	TArray<FSketchfabAssetData> SelectedAssets = GetSelectedAssets();

	// Detect if at least one temporary item was selected. If there were no valid assets selected and a temporary one was, then deny the context menu.
	TArray<TSharedPtr<FAssetViewItem>> SelectedItems = GetSelectedItems();
	bool bAtLeastOneTemporaryItemFound = false;
	for (auto ItemIt = SelectedItems.CreateConstIterator(); ItemIt; ++ItemIt)
	{
		const TSharedPtr<FAssetViewItem>& Item = *ItemIt;
		if (Item->IsTemporaryItem())
		{
			bAtLeastOneTemporaryItemFound = true;
		}
	}

	// If there were no valid assets found, but some invalid assets were found, deny the context menu
	if (SelectedAssets.Num() == 0 && bAtLeastOneTemporaryItemFound)
	{
		return false;
	}

	return true;
}

/*
void SSketchfabAssetView::CreateNewAsset(TSharedPtr<FSketchfabTaskData> Data)
{
	// we should only be creating one deferred asset per tick
	check(!DeferredAssetToCreate.IsValid());

	// Make sure we are showing the location of the new asset (we may have created it in a folder)
	//OnPathSelected.Execute(PackagePath);

	// Defer asset creation until next tick, so we get a chance to refresh the view
	DeferredAssetToCreate = MakeShareable(new FCreateDeferredAssetData());
	DeferredAssetToCreate->DefaultAssetName = DefaultAssetName;
	DeferredAssetToCreate->PackagePath = PackagePath;
	DeferredAssetToCreate->AssetClass = AssetClass;
	DeferredAssetToCreate->Factory = Factory;
	DeferredAssetToCreate->ModelAssetUID = ModelAssetUID;
	DeferredAssetToCreate->ThumbAssetUID = ThumbAssetUID;
}

void SSketchfabAssetView::DeferredCreateNewAsset()
{
	if (DeferredAssetToCreate.IsValid())
	{
		FString PackageNameStr = DeferredAssetToCreate->PackagePath + "/" + DeferredAssetToCreate->DefaultAssetName;
		FName PackageName = FName(*PackageNameStr);
		FName PackagePathFName = FName(*DeferredAssetToCreate->PackagePath);
		FName AssetName = FName(*DeferredAssetToCreate->DefaultAssetName);
		//FName AssetClassName = DeferredAssetToCreate->AssetClass->GetFName();
		FName AssetClassName = "SketchAsset";
		FName ObjectUIDName = FName(*DeferredAssetToCreate->ModelAssetUID);
		FName ThumbAssetUIDName = FName(*DeferredAssetToCreate->ThumbAssetUID);

		FSketchfabAssetData NewAssetData(PackageName, PackagePathFName, AssetName, AssetClassName, ObjectUIDName, ThumbAssetUIDName);
		TSharedPtr<FAssetViewItem> NewItem = MakeShareable(new FAssetViewAsset(NewAssetData));

		NewItem->bRenameWhenScrolledIntoview = true;
		FilteredAssetItems.Insert(NewItem, 0);
		//SortManager.SortList(FilteredAssetItems, MajorityAssetType, CustomColumns);

		//SetSelection(NewItem);
		//RequestScrollIntoView(NewItem);

		//FEditorDelegates::OnNewAssetCreated.Broadcast(DeferredAssetToCreate->Factory);

		DeferredAssetToCreate.Reset();
	}
}
*/


void SSketchfabAssetView::DownloadProgress(const FString& ModelUID, float progress)
{
	DownloadProgressData.Add(ModelUID, progress);
}

bool SSketchfabAssetView::HasLicence(const FString& ModelUID)
{
	LicenceDataInfo *data = LicenceData.Find(ModelUID);
	if (data && !data->LicenceType.IsEmpty())
	{
		return true;
	}
	return false;
}

void SSketchfabAssetView::SetLicence(const FString& ModelUID, const FString &LicenceType, const FString &LicenceInfo)
{
	LicenceDataInfo *data = LicenceData.Find(ModelUID);
	if (data)
	{
		data->LicenceType = LicenceType;
		data->LicenceInfo = LicenceInfo;
	}
	else
	{
		LicenceDataInfo data2;
		data2.LicenceType = LicenceType;
		data2.LicenceInfo = LicenceInfo;
		LicenceData.Add(ModelUID, data2);
	}
}


void SSketchfabAssetView::ForceCreateNewAsset(TSharedPtr<FSketchfabTaskData> Data)
{
	FSketchfabAssetData NewAssetData;
	NewAssetData.ContentFolder = FName(*Data->CacheFolder);
	NewAssetData.ModelName = FName(*Data->ModelName);
	NewAssetData.ModelUID = FName(*Data->ModelUID);
	NewAssetData.ThumbUID = FName(*Data->ThumbnailUID);
	NewAssetData.ThumbnailWidth = Data->ThumbnailWidth;
	NewAssetData.ThumbnailHeight = Data->ThumbnailHeight;
	NewAssetData.AuthorName = FName(*Data->ModelAuthor);
	NewAssetData.ModelPublishedAt = Data->ModelPublishedAt;
	NewAssetData.LicenceInfo = Data->LicenceInfo;
	NewAssetData.LicenceType = Data->LicenceType;
	NewAssetData.ModelSize = Data->ModelSize;

	TSharedPtr<FAssetViewItem> NewItem = MakeShareable(new FAssetViewAsset(NewAssetData));

	NewItem->bRenameWhenScrolledIntoview = true;
	FilteredAssetItems.Add(NewItem);
}

void SSketchfabAssetView::UpdateAssetList()
{
	TileView->RequestListRefresh();
}

void SSketchfabAssetView::NeedRefresh()
{
	bPendingUpdateThumbnails = true;
	bNeedsRefresh = true;
}

void SSketchfabAssetView::FlushThumbnails()
{
	bFlushThumbnails = true;
}

void SSketchfabAssetView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if (bPendingUpdateThumbnails)
	{
		bPendingUpdateThumbnails = false;
		UpdateThumbnails();
	}

	if (bNeedsRefresh)
	{
		bNeedsRefresh = false;
		//UpdateAssetList();

		//This is a hack for now. Just refresh all the widgets.
		TileView->RebuildList();
	}


	if (bFlushThumbnails)
	{
		FilteredAssetItems.Empty();
		bFlushThumbnails = false;
		bNeedsRefresh = true;
	}


	/*
	if (bSlowFullListRefreshRequested)
	{
		RefreshSourceItems();
		bSlowFullListRefreshRequested = false;
		bQuickFrontendListRefreshRequested = true;
	}

	if (QueriedAssetItems.Num() > 0)
	{
		check(OnShouldFilterAsset.IsBound());
		double TickStartTime = FPlatformTime::Seconds();

		// Mark the first amortize time
		if (AmortizeStartTime == 0)
		{
			AmortizeStartTime = FPlatformTime::Seconds();
			bIsWorking = true;
		}

		ProcessQueriedItems(TickStartTime);

		if (QueriedAssetItems.Num() == 0)
		{
			TotalAmortizeTime += FPlatformTime::Seconds() - AmortizeStartTime;
			AmortizeStartTime = 0;
			bIsWorking = false;
		}
		else
		{
			// Need to finish processing queried items before rest of function is safe
			return;
		}
	}

	if (bQuickFrontendListRefreshRequested)
	{
		ResetQuickJump();

		RefreshFilteredItems();
		RefreshFolders();
		// Don't sync to selection if we are just going to do it below
		SortList(!PendingSyncItems.Num());

		bQuickFrontendListRefreshRequested = false;
	}

	if (PendingSyncItems.Num() > 0)
	{
		if (bPendingSortFilteredItems)
		{
			// Don't sync to selection because we are just going to do it below
			SortList(false);
		}

		bBulkSelecting = true;
		ClearSelection();
		bool bFoundScrollIntoViewTarget = false;

		for (auto ItemIt = FilteredAssetItems.CreateConstIterator(); ItemIt; ++ItemIt)
		{
			const auto& Item = *ItemIt;
			if (Item.IsValid())
			{
				if (Item->GetType() == EAssetItemType::Folder)
				{
					const TSharedPtr<FAssetViewFolder>& ItemAsFolder = StaticCastSharedPtr<FAssetViewFolder>(Item);
					if (PendingSyncItems.SelectedFolders.Contains(ItemAsFolder->FolderPath))
					{
						SetItemSelection(*ItemIt, true, ESelectInfo::OnNavigation);

						// Scroll the first item in the list that can be shown into view
						if (!bFoundScrollIntoViewTarget)
						{
							RequestScrollIntoView(Item);
							bFoundScrollIntoViewTarget = true;
						}
					}
				}
				else
				{
					const TSharedPtr<FAssetViewAsset>& ItemAsAsset = StaticCastSharedPtr<FAssetViewAsset>(Item);
					if (PendingSyncItems.SelectedAssets.Contains(ItemAsAsset->Data.ObjectPath))
					{
						SetItemSelection(*ItemIt, true, ESelectInfo::OnNavigation);

						// Scroll the first item in the list that can be shown into view
						if (!bFoundScrollIntoViewTarget)
						{
							RequestScrollIntoView(Item);
							bFoundScrollIntoViewTarget = true;
						}
					}
				}
			}
		}

		bBulkSelecting = false;

		if (bShouldNotifyNextAssetSync && !bUserSearching)
		{
			AssetSelectionChanged(TSharedPtr<FAssetViewAsset>(), ESelectInfo::Direct);
		}

		// Default to always notifying
		bShouldNotifyNextAssetSync = true;

		PendingSyncItems.Reset();

		if (bAllowFocusOnSync && bPendingFocusOnSync)
		{
			FocusList();
		}
	}

	if (IsHovered())
	{
		// This prevents us from sorting the view immediately after the cursor leaves it
		LastSortTime = CurrentTime;
	}
	else if (bPendingSortFilteredItems && InCurrentTime > LastSortTime + SortDelaySeconds)
	{
		SortList();
	}
	*/
	// create any assets & folders we need to now
	//DeferredCreateNewAsset();
	//DeferredCreateNewFolder();

	/*
	// Do quick-jump last as the Tick function might have canceled it
	if (QuickJumpData.bHasChangedSinceLastTick)
	{
		QuickJumpData.bHasChangedSinceLastTick = false;

		const bool bWasJumping = QuickJumpData.bIsJumping;
		QuickJumpData.bIsJumping = true;

		QuickJumpData.LastJumpTime = InCurrentTime;
		QuickJumpData.bHasValidMatch = PerformQuickJump(bWasJumping);
	}
	else if (QuickJumpData.bIsJumping && InCurrentTime > QuickJumpData.LastJumpTime + JumpDelaySeconds)
	{
		ResetQuickJump();
	}

	TSharedPtr<FAssetViewItem> AssetAwaitingRename = AwaitingRename.Pin();
	if (AssetAwaitingRename.IsValid())
	{
		TSharedPtr<SWindow> OwnerWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
		if (!OwnerWindow.IsValid())
		{
			AssetAwaitingRename->bRenameWhenScrolledIntoview = false;
			AwaitingRename = nullptr;
		}
		else if (OwnerWindow->HasAnyUserFocusOrFocusedDescendants())
		{
			AssetAwaitingRename->RenamedRequestEvent.ExecuteIfBound();
			AssetAwaitingRename->bRenameWhenScrolledIntoview = false;
			AwaitingRename = nullptr;
		}
	}
	*/
}

void SSketchfabAssetView::UpdateThumbnails()
{
	int32 MinItemIdx = INDEX_NONE;
	int32 MaxItemIdx = INDEX_NONE;
	int32 MinVisibleItemIdx = INDEX_NONE;
	int32 MaxVisibleItemIdx = INDEX_NONE;

	const int32 HalfNumOffscreenThumbnails = NumOffscreenThumbnails * 0.5;
	for (auto ItemIt = VisibleItems.CreateConstIterator(); ItemIt; ++ItemIt)
	{
		int32 ItemIdx = FilteredAssetItems.Find(*ItemIt);
		if (ItemIdx != INDEX_NONE)
		{
			const int32 ItemIdxLow = FMath::Max<int32>(0, ItemIdx - HalfNumOffscreenThumbnails);
			const int32 ItemIdxHigh = FMath::Min<int32>(FilteredAssetItems.Num() - 1, ItemIdx + HalfNumOffscreenThumbnails);
			if (MinItemIdx == INDEX_NONE || ItemIdxLow < MinItemIdx)
			{
				MinItemIdx = ItemIdxLow;
			}
			if (MaxItemIdx == INDEX_NONE || ItemIdxHigh > MaxItemIdx)
			{
				MaxItemIdx = ItemIdxHigh;
			}
			if (MinVisibleItemIdx == INDEX_NONE || ItemIdx < MinVisibleItemIdx)
			{
				MinVisibleItemIdx = ItemIdx;
			}
			if (MaxVisibleItemIdx == INDEX_NONE || ItemIdx > MaxVisibleItemIdx)
			{
				MaxVisibleItemIdx = ItemIdx;
			}
		}
	}

	if (MinItemIdx != INDEX_NONE && MaxItemIdx != INDEX_NONE && MinVisibleItemIdx != INDEX_NONE && MaxVisibleItemIdx != INDEX_NONE)
	{
		// We have a new min and a new max, compare it to the old min and max so we can create new thumbnails
		// when appropriate and remove old thumbnails that are far away from the view area.
		TMap< TSharedPtr<FAssetViewAsset>, TSharedPtr<FSketchfabAssetThumbnail> > NewRelevantThumbnails;

		// Operate on offscreen items that are furthest away from the visible items first since the thumbnail pool processes render requests in a LIFO order.
		while (MinItemIdx < MinVisibleItemIdx || MaxItemIdx > MaxVisibleItemIdx)
		{
			const int32 LowEndDistance = MinVisibleItemIdx - MinItemIdx;
			const int32 HighEndDistance = MaxItemIdx - MaxVisibleItemIdx;

			if (HighEndDistance > LowEndDistance)
			{
				if (FilteredAssetItems.IsValidIndex(MaxItemIdx) && FilteredAssetItems[MaxItemIdx]->GetType() != EAssetItemType::Folder)
				{
					AddItemToNewThumbnailRelevancyMap(StaticCastSharedPtr<FAssetViewAsset>(FilteredAssetItems[MaxItemIdx]), NewRelevantThumbnails);
				}
				MaxItemIdx--;
			}
			else
			{
				if (FilteredAssetItems.IsValidIndex(MinItemIdx) && FilteredAssetItems[MinItemIdx]->GetType() != EAssetItemType::Folder)
				{
					AddItemToNewThumbnailRelevancyMap(StaticCastSharedPtr<FAssetViewAsset>(FilteredAssetItems[MinItemIdx]), NewRelevantThumbnails);
				}
				MinItemIdx++;
			}
		}

		// Now operate on VISIBLE items then prioritize them so they are rendered first
		TArray< TSharedPtr<FSketchfabAssetThumbnail> > ThumbnailsToPrioritize;
		for (int32 ItemIdx = MinVisibleItemIdx; ItemIdx <= MaxVisibleItemIdx; ++ItemIdx)
		{
			if (FilteredAssetItems.IsValidIndex(ItemIdx) && FilteredAssetItems[ItemIdx]->GetType() != EAssetItemType::Folder)
			{
				TSharedPtr<FSketchfabAssetThumbnail> Thumbnail = AddItemToNewThumbnailRelevancyMap(StaticCastSharedPtr<FAssetViewAsset>(FilteredAssetItems[ItemIdx]), NewRelevantThumbnails);
				if (Thumbnail.IsValid())
				{
					ThumbnailsToPrioritize.Add(Thumbnail);
				}
			}
		}

		// Now prioritize all thumbnails there were in the visible range
		if (ThumbnailsToPrioritize.Num() > 0)
		{
			AssetThumbnailPool->PrioritizeThumbnails(ThumbnailsToPrioritize, CurrentThumbnailSize, CurrentThumbnailSize);
		}

		// Assign the new map of relevant thumbnails. This will remove any entries that were no longer relevant.
		RelevantThumbnails = NewRelevantThumbnails;

		//Update any progress information
		//This is a temporary hack for now to update the progress information for thumbnails when downloading.
		for (auto ItemIt = RelevantThumbnails.CreateIterator(); ItemIt; ++ItemIt)
		{
			auto& Item = *ItemIt;
			TSharedPtr<FSketchfabAssetThumbnail> Thumbnail = StaticCastSharedPtr<FSketchfabAssetThumbnail>(Item.Value);
			float * progress = DownloadProgressData.Find(Thumbnail->GetAssetData().ModelUID.ToString());
			if (progress)
			{
				if ((*progress) < 1.0)
				{
					Thumbnail->SetProgress((*progress));
				}
			}
		}
	}
}

TSharedPtr<FSketchfabAssetThumbnail> SSketchfabAssetView::AddItemToNewThumbnailRelevancyMap(const TSharedPtr<FAssetViewAsset>& Item, TMap< TSharedPtr<FAssetViewAsset>, TSharedPtr<FSketchfabAssetThumbnail> >& NewRelevantThumbnails)
{
	const TSharedPtr<FSketchfabAssetThumbnail>* Thumbnail = RelevantThumbnails.Find(Item);
	if (Thumbnail)
	{
		// The thumbnail is still relevant, add it to the new list
		NewRelevantThumbnails.Add(Item, *Thumbnail);

		return *Thumbnail;
	}
	else
	{
		/*
		//For now I am ignoring the scale of the thumbnail since it is currently not used for the asset browser
		//In future if this is to be used the the width and height of the loaded thumbnail image will need to be
		//calculated correctly and code changed in the thumbnailpool loading method.
		if (!ensure(CurrentThumbnailSize > 0 && CurrentThumbnailSize <= MAX_THUMBNAIL_SIZE))
		{
			// Thumbnail size must be in a sane range
			CurrentThumbnailSize = 64;
		}

		// The thumbnail newly relevant, create a new thumbnail
		const float ThumbnailResolution = CurrentThumbnailSize * MaxThumbnailScale;
		*/

		TSharedPtr<FSketchfabAssetThumbnail> NewThumbnail = MakeShareable(new FSketchfabAssetThumbnail(Item->Data, Item->Data.ThumbnailWidth, Item->Data.ThumbnailHeight, AssetThumbnailPool));
		NewRelevantThumbnails.Add(Item, NewThumbnail);
		NewThumbnail->GetViewportRenderTargetTexture(); // Access the texture once to trigger it to render

		return NewThumbnail;
	}
}



///////////////////////////////
// SSKAssetTileItem
///////////////////////////////

SSKAssetTileItem::~SSKAssetTileItem()
{
	//FCoreUObjectDelegates::OnAssetLoaded.RemoveAll(this);
}

void SSKAssetTileItem::Construct( const FArguments& InArgs )
{
	SSKAssetViewItem::Construct(SSKAssetViewItem::FArguments()
		.AssetItem(InArgs._AssetItem)
		/*
		.OnRenameBegin(InArgs._OnRenameBegin)
		.OnRenameCommit(InArgs._OnRenameCommit)
		.OnVerifyRenameCommit(InArgs._OnVerifyRenameCommit)
		.OnItemDestroyed(InArgs._OnItemDestroyed)
		.ThumbnailEditMode(InArgs._ThumbnailEditMode)
		.HighlightText(InArgs._HighlightText)
		.OnAssetsOrPathsDragDropped(InArgs._OnAssetsOrPathsDragDropped)
		.OnFilesDragDropped(InArgs._OnFilesDragDropped)
		*/
		//.ShouldAllowToolTip(InArgs._ShouldAllowToolTip)
		//.OnGetCustomAssetToolTip(InArgs._OnGetCustomAssetToolTip)
		//.OnVisualizeAssetToolTip(InArgs._OnVisualizeAssetToolTip)
		//.OnAssetToolTipClosing( InArgs._OnAssetToolTipClosing )
		);

	AssetThumbnail = InArgs._AssetThumbnail;
	ItemWidth = InArgs._ItemWidth;
	ThumbnailPadding = IsFolder() ? InArgs._ThumbnailPadding + 5.0f : InArgs._ThumbnailPadding;

	TSharedPtr<SWidget> Thumbnail;

	if (AssetItem.IsValid() && AssetThumbnail.IsValid())
	{
		FSketchfabAssetThumbnailConfig ThumbnailConfig;
		ThumbnailConfig.bAllowFadeIn = true;
		//ThumbnailConfig.bAllowHintText = InArgs._AllowThumbnailHintLabel;
		ThumbnailConfig.bForceGenericThumbnail = (AssetItem->GetType() == EAssetItemType::Creation);
		ThumbnailConfig.bAllowAssetSpecificThumbnailOverlay = (AssetItem->GetType() != EAssetItemType::Creation);
		ThumbnailConfig.ThumbnailLabel = InArgs._ThumbnailLabel;
		//ThumbnailConfig.HighlightedText = InArgs._HighlightText;
		//ThumbnailConfig.HintColorAndOpacity = InArgs._ThumbnailHintColorAndOpacity;

		if (HasSketchAssetZipFile(AssetThumbnail->GetAssetData()))
		{
			ThumbnailConfig.AssetTypeColorOverride = FColor(70, 138, 209);
		}

		Thumbnail = AssetThumbnail->MakeThumbnailWidget(ThumbnailConfig);
	}
	else
	{
		Thumbnail = SNew(SImage).Image(FEditorStyle::GetDefaultBrush());
	}

	FName ItemShadowBorderName;
	TSharedRef<SWidget> ItemContents = FAssetViewItemHelper::CreateTileItemContents(this, Thumbnail.ToSharedRef(), ItemShadowBorderName);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(this, &SSKAssetViewItem::GetBorderImage)
		.Padding(0)
		.AddMetaData<FTagMetaData>(FTagMetaData(AssetItem->GetType() == EAssetItemType::Normal ? StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data.ModelUID : NAME_None))
		[
			SNew(SVerticalBox)

			// Thumbnail
			+SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				// The remainder of the space is reserved for the name.
				SNew(SBox)
				.Padding(ThumbnailPadding - 4.f)
				.WidthOverride(this, &SSKAssetTileItem::GetThumbnailBoxSize)
				.HeightOverride( this, &SSKAssetTileItem::GetThumbnailBoxSize )
				[
					// Drop shadow border
					SNew(SBorder)
					.Padding(4.f)
					.BorderImage(FEditorStyle::GetBrush(ItemShadowBorderName))
					[
						ItemContents
					]
				]
			]

 			+SVerticalBox::Slot()
 			.Padding(FMargin(1.f, 0))
 			.HAlign(HAlign_Center)
 			.VAlign(VAlign_Center)
 			.FillHeight(1.f)
 			[
 				SAssignNew(InlineRenameWidget, STextBlock)
 					.Text( GetNameText() )
 					.Justification(ETextJustify::Center)
					.WrapTextAt(134)
 			]
		]
	];
}

void SSKAssetTileItem::OnAssetDataChanged()
{
}

FOptionalSize SSKAssetTileItem::GetThumbnailBoxSize() const
{
	return FOptionalSize(ItemWidth.Get());
}

FOptionalSize SSKAssetTileItem::GetSCCImageSize() const
{
	return GetThumbnailBoxSize().Get() * 0.2;
}

FSlateFontInfo SSKAssetTileItem::GetThumbnailFont() const
{
	FOptionalSize ThumbSize = GetThumbnailBoxSize();
	if (ThumbSize.IsSet())
	{
		float Size = ThumbSize.Get();
		if (Size < 50)
		{
			const static FName SmallFontName("ContentBrowser.AssetTileViewNameFontVerySmall");
			return FEditorStyle::GetFontStyle(SmallFontName);
		}
		else if (Size < 85)
		{
			const static FName SmallFontName("ContentBrowser.AssetTileViewNameFontSmall");
			return FEditorStyle::GetFontStyle(SmallFontName);
		}
	}

	const static FName RegularFont("ContentBrowser.AssetTileViewNameFont");
	return FEditorStyle::GetFontStyle(RegularFont);
}


///////////////////////////////
// FAssetViewModeUtils
///////////////////////////////

TSharedRef<SWidget> FAssetViewItemHelper::CreateTileItemContents(SSKAssetTileItem* const InTileItem, const TSharedRef<SWidget>& InThumbnail, FName& OutItemShadowBorder)
{
	return CreateListTileItemContents(InTileItem, InThumbnail, OutItemShadowBorder);
}

template <typename T>
TSharedRef<SWidget> FAssetViewItemHelper::CreateListTileItemContents(T* const InTileOrListItem, const TSharedRef<SWidget>& InThumbnail, FName& OutItemShadowBorder)
{
	TSharedRef<SOverlay> ItemContentsOverlay = SNew(SOverlay);

	OutItemShadowBorder = FName("ContentBrowser.ThumbnailShadow");

	// The actual thumbnail
	ItemContentsOverlay->AddSlot()
	[
		InThumbnail
	];

	return ItemContentsOverlay;
}

TArray<TSharedPtr<FAssetViewItem>> SSketchfabAssetView::GetSelectedItems() const
{
	return TileView->GetSelectedItems();
}

TArray<FSketchfabAssetData> SSketchfabAssetView::GetSelectedAssets() const
{
	TArray<TSharedPtr<FAssetViewItem>> SelectedItems = GetSelectedItems();
	TArray<FSketchfabAssetData> SelectedAssets;
	for (auto ItemIt = SelectedItems.CreateConstIterator(); ItemIt; ++ItemIt)
	{
		const TSharedPtr<FAssetViewItem>& Item = *ItemIt;

		// Only report non-temporary & non-folder items
		if (Item.IsValid() && !Item->IsTemporaryItem() && Item->GetType() != EAssetItemType::Folder)
		{
			SelectedAssets.Add(StaticCastSharedPtr<FAssetViewAsset>(Item)->Data);
		}
	}

	return SelectedAssets;
}

//==========================================================================

FSketchfabDragDropOperation::FSketchfabDragDropOperation()
{
}

TSharedPtr<SWidget> FSketchfabDragDropOperation::GetDefaultDecorator() const
{
	if (AssetThumbnailPool.IsValid() && DraggedAssets.Num() > 0)
	{
		FDeferredCleanupSlateBrush* Texture = NULL;

		const FSketchfabAssetData &AssetData = DraggedAssets[0];
		AssetThumbnailPool->AccessTexture(AssetData, AssetData.ThumbnailWidth, AssetData.ThumbnailHeight, &Texture);

		// The viewport for the rendered thumbnail, if it exists
		if (Texture)
		{
			TSharedRef<SBorder> RootNode = SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.1f, 0.1f, 0.1f, 1.f))
			.Padding(4)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.MaxHeight(127)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					[
						SNew(SBox)
						.HeightOverride(127)
						.WidthOverride(127)
						[
							SNew(SScaleBox)
							.StretchDirection(EStretchDirection::Both)
							.Stretch(EStretch::ScaleToFill)
							[
								SNew(SImage).Image(Texture->GetSlateBrush())
							]
						]
					]
				]
			];

			return RootNode;
		}
	}
	return SNew(SImage).Image(FEditorStyle::GetDefaultBrush());
}

void FSketchfabDragDropOperation::OnDragged(const class FDragDropEvent& DragDropEvent)
{
	if (CursorDecoratorWindow.IsValid())
	{
		CursorDecoratorWindow->MoveWindowTo(DragDropEvent.GetScreenSpacePosition());
	}
}

void FSketchfabDragDropOperation::Construct()
{
	MouseCursor = EMouseCursor::GrabHandClosed;
	FDragDropOperation::Construct();
}

#undef LOCTEXT_NAMESPACE
