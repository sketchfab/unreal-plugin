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
#include "SSplitter.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Misc/FileHelper.h"
#include "BreakIterator.h"
#include "SInlineEditableTextBlock.h"
#include "ConstructorHelpers.h"
#include "ImageLoader.h"

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

			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			.Padding(FMargin(8, 0))
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ErrorReporting.EmptyBox"))
				/*
				.BorderBackgroundColor(this, &SAssetView::GetQuickJumpColor)
				.Visibility(this, &SAssetView::IsQuickJumpVisible)
				[
					SNew(STextBlock)
					.Text(this, &SAssetView::GetQuickJumpTerm)
				]
				*/
			]
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


	if (AssetItem->GetType() == EAssetItemType::Folder)
	{
		TSharedPtr< STableRow<TSharedPtr<FAssetViewItem>> > TableRowWidget;
		/*
		SAssignNew(TableRowWidget, STableRow<TSharedPtr<FAssetViewItem>>, OwnerTable)
			.Style(FEditorStyle::Get(), "ContentBrowser.AssetListView.TableRow")
			.Cursor(bAllowDragging ? EMouseCursor::GrabHand : EMouseCursor::Default)
			.OnDragDetected(this, &SAssetView::OnDraggingAssetItem);

		TSharedRef<SAssetTileItem> Item =
			SNew(SAssetTileItem)
			.AssetItem(AssetItem)
			.ItemWidth(this, &SAssetView::GetTileViewItemWidth)
			.OnRenameBegin(this, &SAssetView::AssetRenameBegin)
			.OnRenameCommit(this, &SAssetView::AssetRenameCommit)
			.OnVerifyRenameCommit(this, &SAssetView::AssetVerifyRenameCommit)
			.OnItemDestroyed(this, &SAssetView::AssetItemWidgetDestroyed)
			.ShouldAllowToolTip(this, &SAssetView::ShouldAllowToolTips)
			.HighlightText(HighlightedText)
			.IsSelected(FIsSelected::CreateSP(TableRowWidget.Get(), &STableRow<TSharedPtr<FAssetViewItem>>::IsSelectedExclusively))
			.OnAssetsOrPathsDragDropped(this, &SAssetView::OnAssetsOrPathsDragDropped)
			.OnFilesDragDropped(this, &SAssetView::OnFilesDragDropped);

		TableRowWidget->SetContent(Item);
		*/

		return TableRowWidget.ToSharedRef();
	}
	else 
	{
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
			AssetThumbnail = MakeShareable(new FSketchfabAssetThumbnail(AssetItemAsAsset->Data, ThumbnailResolution, ThumbnailResolution, AssetThumbnailPool));
			RelevantThumbnails.Add(AssetItemAsAsset, AssetThumbnail);
			AssetThumbnail->GetViewportRenderTargetTexture(); // Access the texture once to trigger it to render
		}

		TSharedPtr< STableRow<TSharedPtr<FAssetViewItem>> > TableRowWidget;
		SAssignNew(TableRowWidget, STableRow<TSharedPtr<FAssetViewItem>>, OwnerTable)
			.Style(FEditorStyle::Get(), "ContentBrowser.AssetListView.TableRow")
			.Cursor(bAllowDragging ? EMouseCursor::GrabHand : EMouseCursor::Default)
			.OnDragDetected(this, &SSketchfabAssetView::OnDraggingAssetItem);

		TSharedRef<SAssetTileItem> Item =
			SNew(SAssetTileItem)
			.AssetThumbnail(AssetThumbnail)
			.AssetItem(AssetItem)
			.ThumbnailPadding(TileViewThumbnailPadding)
			.ItemWidth(this, &SSketchfabAssetView::GetTileViewItemWidth)
			//.OnRenameBegin(this, &SAssetView::AssetRenameBegin)
			//.OnRenameCommit(this, &SAssetView::AssetRenameCommit)
			//.OnVerifyRenameCommit(this, &SAssetView::AssetVerifyRenameCommit)
			//.OnItemDestroyed(this, &SAssetView::AssetItemWidgetDestroyed)
			//.ShouldAllowToolTip(this, &SAssetView::ShouldAllowToolTips)
			//.HighlightText(HighlightedText)
			//.ThumbnailEditMode(this, &SAssetView::IsThumbnailEditMode)
			.ThumbnailLabel(ThumbnailLabel)
			//.ThumbnailHintColorAndOpacity(this, &SAssetView::GetThumbnailHintColorAndOpacity)
			//.AllowThumbnailHintLabel(AllowThumbnailHintLabel)
			.IsSelected(FIsSelected::CreateSP(TableRowWidget.Get(), &STableRow<TSharedPtr<FAssetViewItem>>::IsSelectedExclusively))
			//.OnGetCustomAssetToolTip(OnGetCustomAssetToolTip)
			//.OnVisualizeAssetToolTip(OnVisualizeAssetToolTip)
			//.OnAssetToolTipClosing(OnAssetToolTipClosing);
			;

		TableRowWidget->SetContent(Item);

		return TableRowWidget.ToSharedRef();
	}
}

void SSketchfabAssetView::OnListMouseButtonDoubleClick(TSharedPtr<FAssetViewItem> AssetItem)
{
	if (!ensure(AssetItem.IsValid()))
	{
		return;
	}
	/*
	if (IsThumbnailEditMode())
	{
		// You can not activate assets when in thumbnail edit mode because double clicking may happen inadvertently while adjusting thumbnails.
		return;
	}

	if (AssetItem->GetType() == EAssetItemType::Folder)
	{
		OnPathSelected.ExecuteIfBound(StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->FolderPath);
		return;
	}
	*/

	/*
	if (AssetItem->IsTemporaryItem())
	{
		// You may not activate temporary items, they are just for display.
		return;
	}
	*/

	TArray<FSketchfabAssetData> ActivatedAssets;
	ActivatedAssets.Add(StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data);
	OnAssetsActivated.ExecuteIfBound(ActivatedAssets, EAssetTypeActivationMethod::DoubleClicked);
}

FReply SSketchfabAssetView::OnDraggingAssetItem(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bAllowDragging)
	{
		TArray<FSketchfabAssetData> DraggedAssets;
		TArray<FString> DraggedAssetPaths;

		// Work out which assets to drag
		{
			TArray<FSketchfabAssetData> AssetDataList = GetSelectedAssets();
			for (const FSketchfabAssetData& AssetData : AssetDataList)
			{
				// Skip invalid assets and redirectors
				if (AssetData.IsValid() && HasSketchAssetZipFile(AssetData))
				{
					DraggedAssets.Add(AssetData);
					DraggedAssetPaths.Add(GetSketchfabAssetZipPath(AssetData));
				}
			}
		}

		// Use the custom drag handler?
		if (DraggedAssets.Num() > 0 && FEditorDelegates::OnAssetDragStarted.IsBound())
		{
			//FEditorDelegates::OnAssetDragStarted.Broadcast(DraggedAssets, nullptr);
			return FReply::Handled();
		}

		// Use the standard drag handler?
		if ((DraggedAssets.Num() > 0 || DraggedAssetPaths.Num() > 0) && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{			
			return FReply::Handled().BeginDragDrop(FExternalDragOperation::NewFiles(DraggedAssetPaths));
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


	/*
	if (IsThumbnailEditMode())
	{
		// You can not summon a context menu for assets when in thumbnail edit mode because right clicking may happen inadvertently while adjusting thumbnails.
		return false;
	}
	*/

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

	/*
	if (SelectedAssets.Num() == 0 && SourcesData.HasCollections())
	{
		// Don't allow a context menu when we're viewing a collection and have no assets selected
		return false;
	}
	*/

	// Build a list of selected object paths
	/*
	TArray<FString> ObjectPaths;
	for (auto AssetIt = SelectedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
	{
		ObjectPaths.Add(AssetIt->ObjectPath.ToString());
	}
	*/

	bool bLoadSuccessful = true;

	/*
	if (bPreloadAssetsForContextMenu)
	{
		TArray<UObject*> LoadedObjects;
		const bool bAllowedToPrompt = false;
		bLoadSuccessful = ContentBrowserUtils::LoadAssetsIfNeeded(ObjectPaths, LoadedObjects, bAllowedToPrompt);
	}
	*/

	// Do not show the context menu if the load failed
	return bLoadSuccessful;
}

/*
void SSketchfabAssetView::CreateNewAsset(const FString& DefaultAssetName, const FString& PackagePath, UClass* AssetClass, UFactory* Factory, const FString& ModelAssetUID, const FString& ThumbAssetUID)
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

void SSketchfabAssetView::ForceCreateNewAsset(const FString& ModelNameStr, const FString& ContentFolderStr, const FString& ModelAssetUID, const FString& ThumbAssetUID)
{
	FName ContentFolder = FName(*ContentFolderStr);
	FName ModelName = FName(*ModelNameStr);
	FName ObjectUIDName = FName(*ModelAssetUID);
	FName ThumbAssetUIDName = FName(*ThumbAssetUID);

	FSketchfabAssetData NewAssetData(ContentFolder, ModelName, ObjectUIDName, ThumbAssetUIDName);

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

void SSketchfabAssetView::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	/*
	CalculateFillScale(AllottedGeometry);

	CurrentTime = InCurrentTime;

	// If there were any assets that were recently added via the asset registry, process them now
	ProcessRecentlyAddedAssets();

	// If there were any assets loaded since last frame that we are currently displaying thumbnails for, push them on the render stack now.
	ProcessRecentlyLoadedOrChangedAssets();

	CalculateThumbnailHintColorAndOpacity();
	*/

	if (bPendingUpdateThumbnails)
	{
		bPendingUpdateThumbnails = false;
		UpdateThumbnails();
	}

	if (bNeedsRefresh)
	{
		bNeedsRefresh = false;
		UpdateAssetList();
		UpdateThumbnails();

		//This is a hack for now. Just refresh all the widgets.
		TileView->RebuildList();
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
		if (!ensure(CurrentThumbnailSize > 0 && CurrentThumbnailSize <= MAX_THUMBNAIL_SIZE))
		{
			// Thumbnail size must be in a sane range
			CurrentThumbnailSize = 64;
		}

		// The thumbnail newly relevant, create a new thumbnail
		const float ThumbnailResolution = CurrentThumbnailSize * MaxThumbnailScale;
		TSharedPtr<FSketchfabAssetThumbnail> NewThumbnail = MakeShareable(new FSketchfabAssetThumbnail(Item->Data, ThumbnailResolution, ThumbnailResolution, AssetThumbnailPool));
		NewRelevantThumbnails.Add(Item, NewThumbnail);
		NewThumbnail->GetViewportRenderTargetTexture(); // Access the texture once to trigger it to render

		return NewThumbnail;
	}
}



///////////////////////////////
// SAssetTileItem
///////////////////////////////

SAssetTileItem::~SAssetTileItem()
{
	//FCoreUObjectDelegates::OnAssetLoaded.RemoveAll(this);
}

void SAssetTileItem::Construct( const FArguments& InArgs )
{
	SAssetViewItem::Construct(SAssetViewItem::FArguments()
		.AssetItem(InArgs._AssetItem));
	/*
		.OnRenameBegin(InArgs._OnRenameBegin)
		.OnRenameCommit(InArgs._OnRenameCommit)
		.OnVerifyRenameCommit(InArgs._OnVerifyRenameCommit)
		.OnItemDestroyed(InArgs._OnItemDestroyed)
		.ShouldAllowToolTip(InArgs._ShouldAllowToolTip)
		.ThumbnailEditMode(InArgs._ThumbnailEditMode)
		.HighlightText(InArgs._HighlightText)
		.OnAssetsOrPathsDragDropped(InArgs._OnAssetsOrPathsDragDropped)
		.OnFilesDragDropped(InArgs._OnFilesDragDropped)
		.OnGetCustomAssetToolTip(InArgs._OnGetCustomAssetToolTip)
		.OnVisualizeAssetToolTip(InArgs._OnVisualizeAssetToolTip)
		.OnAssetToolTipClosing( InArgs._OnAssetToolTipClosing )
		);
		*/

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
		.BorderImage(this, &SAssetViewItem::GetBorderImage)
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
				.WidthOverride(this, &SAssetTileItem::GetThumbnailBoxSize)
				.HeightOverride( this, &SAssetTileItem::GetThumbnailBoxSize )
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
 				SAssignNew(InlineRenameWidget, SInlineEditableTextBlock)
 					//.Font( this, &SAssetTileItem::GetThumbnailFont )
 					.Text( GetNameText() )
 					//.OnBeginTextEdit(this, &SAssetTileItem::HandleBeginNameChange)
 					//.OnTextCommitted(this, &SAssetTileItem::HandleNameCommitted)
 					//.OnVerifyTextChanged(this, &SAssetTileItem::HandleVerifyNameChanged)
 					//.HighlightText(InArgs._HighlightText)
 					.IsSelected(InArgs._IsSelected)
 					.IsReadOnly(this, &SAssetTileItem::IsNameReadOnly)
 					.Justification(ETextJustify::Center)
 					.LineBreakPolicy(FBreakIterator::CreateLineBreakIterator())
 			]
		]
	];

	if(AssetItem.IsValid())
	{
		AssetItem->RenamedRequestEvent.BindSP( InlineRenameWidget.Get(), &SInlineEditableTextBlock::EnterEditingMode );
	}
}

void SAssetTileItem::OnAssetDataChanged()
{
	//SAssetViewItem::OnAssetDataChanged();
}

FOptionalSize SAssetTileItem::GetThumbnailBoxSize() const
{
	return FOptionalSize(ItemWidth.Get());
}

FOptionalSize SAssetTileItem::GetSCCImageSize() const
{
	return GetThumbnailBoxSize().Get() * 0.2;
}

FSlateFontInfo SAssetTileItem::GetThumbnailFont() const
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

TSharedRef<SWidget> FAssetViewItemHelper::CreateTileItemContents(SAssetTileItem* const InTileItem, const TSharedRef<SWidget>& InThumbnail, FName& OutItemShadowBorder)
{
	return CreateListTileItemContents(InTileItem, InThumbnail, OutItemShadowBorder);
}

template <typename T>
TSharedRef<SWidget> FAssetViewItemHelper::CreateListTileItemContents(T* const InTileOrListItem, const TSharedRef<SWidget>& InThumbnail, FName& OutItemShadowBorder)
{
	TSharedRef<SOverlay> ItemContentsOverlay = SNew(SOverlay);

	/*
	if (InTileOrListItem->IsFolder())
	{
		OutItemShadowBorder = FName("NoBorder");

		TSharedPtr<FAssetViewFolder> AssetFolderItem = StaticCastSharedPtr<FAssetViewFolder>(InTileOrListItem->AssetItem);

		ECollectionShareType::Type CollectionFolderShareType = ECollectionShareType::CST_All;
		if (AssetFolderItem->bCollectionFolder)
		{
			ContentBrowserUtils::IsCollectionPath(AssetFolderItem->FolderPath, nullptr, &CollectionFolderShareType);
		}

		const FSlateBrush* FolderBaseImage = AssetFolderItem->bDeveloperFolder
			? FEditorStyle::GetBrush("ContentBrowser.ListViewDeveloperFolderIcon.Base")
			: FEditorStyle::GetBrush("ContentBrowser.ListViewFolderIcon.Base");

		const FSlateBrush* FolderTintImage = AssetFolderItem->bDeveloperFolder
			? FEditorStyle::GetBrush("ContentBrowser.ListViewDeveloperFolderIcon.Mask")
			: FEditorStyle::GetBrush("ContentBrowser.ListViewFolderIcon.Mask");

		// Folder base
		ItemContentsOverlay->AddSlot()
			[
				SNew(SImage)
				.Image(FolderBaseImage)
			.ColorAndOpacity(InTileOrListItem, &T::GetAssetColor)
			];

		if (AssetFolderItem->bCollectionFolder)
		{
			FLinearColor IconColor = FLinearColor::White;
			switch (CollectionFolderShareType)
			{
			case ECollectionShareType::CST_Local:
				IconColor = FColor(196, 15, 24);
				break;
			case ECollectionShareType::CST_Private:
				IconColor = FColor(192, 196, 0);
				break;
			case ECollectionShareType::CST_Shared:
				IconColor = FColor(0, 136, 0);
				break;
			default:
				break;
			}

			auto GetCollectionIconBoxSize = [InTileOrListItem]() -> FOptionalSize
			{
				return FOptionalSize(InTileOrListItem->GetThumbnailBoxSize().Get() * 0.3f);
			};

			auto GetCollectionIconBrush = [=]() -> const FSlateBrush*
			{
				const TCHAR* IconSizeSuffix = (GetCollectionIconBoxSize().Get() <= 16.0f) ? TEXT(".Small") : TEXT(".Large");
			return FEditorStyle::GetBrush(ECollectionShareType::GetIconStyleName(CollectionFolderShareType, IconSizeSuffix));
			};

			// Collection share type
			ItemContentsOverlay->AddSlot()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SBox)
					.WidthOverride_Lambda(GetCollectionIconBoxSize)
				.HeightOverride_Lambda(GetCollectionIconBoxSize)
				[
					SNew(SImage)
					.Image_Lambda(GetCollectionIconBrush)
				.ColorAndOpacity(IconColor)
				]
				];
		}

		// Folder tint
		ItemContentsOverlay->AddSlot()
			[
				SNew(SImage)
				.Image(FolderTintImage)
			];
	}
	else
	*/
	{
		OutItemShadowBorder = FName("ContentBrowser.ThumbnailShadow");

		// The actual thumbnail
		ItemContentsOverlay->AddSlot()
			[
				InThumbnail
			];


		// Source control state
		/*
		ItemContentsOverlay->AddSlot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Top)
			[
				SNew(SBox)
				.WidthOverride(InTileOrListItem, &T::GetSCCImageSize)
			.HeightOverride(InTileOrListItem, &T::GetSCCImageSize)
			[
				SNew(SImage)
				.Image(InTileOrListItem, &T::GetSCCStateImage)
			]
			];

		// Dirty state
		ItemContentsOverlay->AddSlot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Bottom)
			[
				SNew(SImage)
				.Image(InTileOrListItem, &T::GetDirtyImage)
			];
		*/

		// Tools for thumbnail edit mode
		/*
		ItemContentsOverlay->AddSlot()
			[
				SNew(SThumbnailEditModeTools, InTileOrListItem->AssetThumbnail)
				.SmallView(!InTileOrListItem->CanDisplayPrimitiveTools())
			.Visibility(InTileOrListItem, &T::GetThumbnailEditModeUIVisibility)
			];
			*/
	}

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

#undef LOCTEXT_NAMESPACE
