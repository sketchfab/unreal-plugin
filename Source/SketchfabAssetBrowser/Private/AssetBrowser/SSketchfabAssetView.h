// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "Input/Reply.h"
#include "Layout/Visibility.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Styling/SlateColor.h"
#include "Widgets/SWidget.h"
#include "Widgets/SCompoundWidget.h"
#include "SketchfabAssetData.h"
#include "ARFilter.h"
#include "SketchfabAssetThumbnail.h"
#include "IContentBrowserSingleton.h"
#include "Animation/CurveSequence.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STileView.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "SketchfabTask.h"
#include "Editor/EditorStyle/Public/EditorStyleSet.h"

struct FAssetViewItemHelper;

/** Called when the user double clicks, presses enter, or presses space on an asset */
DECLARE_DELEGATE_TwoParams(FOnSketchAssetsActivated, const TArray<FSketchfabAssetData>& /*ActivatedAssets*/, EAssetTypeActivationMethod::Type /*ActivationMethod*/);

/** Called when an asset is selected in the asset view */
DECLARE_DELEGATE_OneParam(FOnSketchfabAssetSelected, const FSketchfabAssetData& /*AssetData*/);

/** Called to request the menu when right clicking on an asset */
DECLARE_DELEGATE_RetVal_OneParam(TSharedPtr<SWidget>, FOnGetSketchfabAssetContextMenu, const TArray<FSketchfabAssetData>& /*SelectedAssets*/);

/** Called to request a custom asset item tooltip */
DECLARE_DELEGATE_RetVal_OneParam(TSharedRef<SToolTip>, FOnGetCustomSketchfabAssetToolTip, FSketchfabAssetData& /*AssetData*/);

/** Called when an asset item visualizes its tooltip */
DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnVisualizeSketchfabAssetToolTip, const TSharedPtr<SWidget>& /*ToolTipContent*/, FSketchfabAssetData& /*AssetData*/);

/** Called when an asset item's tooltip is closing */
DECLARE_DELEGATE(FOnSketchfabAssetToolTipClosing);

namespace EAssetItemType
{
	enum Type
	{
		Normal,
		Folder,
		Creation,
		Duplication
	};
}

/** Base class for items displayed in the asset view */
struct FAssetViewItem
{
	FAssetViewItem()
		: bRenameWhenScrolledIntoview(false)
	{
	}

	virtual ~FAssetViewItem() {}

	/** Get the type of this asset item */
	virtual EAssetItemType::Type GetType() const = 0;

	/** Get whether this is a temporary item */
	virtual bool IsTemporaryItem() const = 0;

	/** Broadcasts whenever a rename is requested */
	FSimpleDelegate RenamedRequestEvent;

	/** An event to fire when the asset data for this item changes */
	DECLARE_MULTICAST_DELEGATE(FOnAssetDataChanged);
	FOnAssetDataChanged OnAssetDataChanged;

	/** True if this item will enter inline renaming on the next scroll into view */
	bool bRenameWhenScrolledIntoview;
};

/** Item that represents an asset */
struct FAssetViewAsset : public FAssetViewItem
{
	/** The asset registry data associated with this item */
	FSketchfabAssetData Data;

	/** Map of values for custom columns */
	TMap<FName, FString> CustomColumnData;

	explicit FAssetViewAsset(const FSketchfabAssetData& AssetData)
		: Data(AssetData)
	{}

	void SetAssetData(const FSketchfabAssetData& NewData)
	{
		Data = NewData;
		OnAssetDataChanged.Broadcast();
	}

	// FAssetViewItem interface
	virtual EAssetItemType::Type GetType() const override
	{
		return EAssetItemType::Normal;
	}

	virtual bool IsTemporaryItem() const override
	{
		return false;
	}
};

/** A base class for all asset view items */
class SSKAssetViewItem : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSKAssetViewItem)
	{}

	/** Data for the asset this item represents */
	SLATE_ARGUMENT(TSharedPtr<FAssetViewItem>, AssetItem)

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs)
	{
		AssetItem = InArgs._AssetItem;
		bDraggedOver = false;
	}

	/** Handles committing a name change */
	virtual void OnAssetDataChanged() { }

	/** Returns the width at which the name label will wrap the name */
	virtual float GetNameTextWrapWidth() const { return 0.0f; }

	const FSlateBrush* GetBorderImage() const
	{
		return bDraggedOver ? FEditorStyle::GetBrush("Menu.Background") : FEditorStyle::GetBrush("NoBorder");
	}

	bool IsFolder()
	{
		return false;
	}

	const FSlateBrush* GetSCCStateImage() const
	{
		//return ThumbnailEditMode.Get() ? FEditorStyle::GetNoBrush() : SCCStateBrush;
		return FEditorStyle::GetNoBrush();
	}

	const FSlateBrush* GetDirtyImage() const
	{
		//return IsDirty() ? AssetDirtyBrush : NULL;
		return NULL;
	}

	FText GetNameText() const
	{
		if (AssetItem.IsValid())
		{
			if (AssetItem->GetType() != EAssetItemType::Folder)
			{
				return FText::FromName(StaticCastSharedPtr<FAssetViewAsset>(AssetItem)->Data.ModelName);
			}
			else
			{
				//return StaticCastSharedPtr<FAssetViewFolder>(AssetItem)->FolderName;
			}
		}

		return FText();
	}


	FString GetModelPath() const
	{
		if (AssetItem.IsValid())
		{
			TSharedPtr<FAssetViewAsset> Asset = StaticCastSharedPtr<FAssetViewAsset>(AssetItem);
			const FName &AssetUID = Asset->Data.ModelUID;
			const FName &PackagePath = Asset->Data.ContentFolder;

			FString path = PackagePath.ToString() / AssetUID.ToString();
			path += ".zip";

			return path;
		}

		return FString();
	}

	FString GetThumbPath() const
	{
		if (AssetItem.IsValid())
		{
			TSharedPtr<FAssetViewAsset> Asset = StaticCastSharedPtr<FAssetViewAsset>(AssetItem);
			const FName &ThumbUID = Asset->Data.ThumbUID;
			const FName &PackagePath = Asset->Data.ContentFolder;

			FString path = PackagePath.ToString() / ThumbUID.ToString();
			path += ".jpg";

			return path;
		}

		return FString();
	}

	void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{
		const float PrevSizeX = LastGeometry.Size.X;

		LastGeometry = AllottedGeometry;

		// Set cached wrap text width based on new "LastGeometry" value.
		// We set this only when changed because binding a delegate to text wrapping attributes is expensive
		if (PrevSizeX != AllottedGeometry.Size.X && InlineRenameWidget.IsValid())
		{
			InlineRenameWidget->SetWrapTextAt(GetNameTextWrapWidth());
		}

		/*
		UpdatePackageDirtyState();

		UpdateSourceControlState((float)InDeltaTime);
		*/
	}

protected:
	bool bDraggedOver;

	/** The data for this item */
	TSharedPtr<FAssetViewItem> AssetItem;

	TSharedPtr< STextBlock > InlineRenameWidget;

	/** The geometry last frame. Used when telling popup messages where to appear. */
	FGeometry LastGeometry;
};

/** An item in the asset tile view */
class SSKAssetTileItem : public SSKAssetViewItem
{
	friend struct FAssetViewItemHelper;

public:
	SLATE_BEGIN_ARGS(SSKAssetTileItem)
	{
	}
		/** The handle to the thumbnail this item should render */
	SLATE_ARGUMENT(TSharedPtr<FSketchfabAssetThumbnail>, AssetThumbnail)

		/** Data for the asset this item represents */
		SLATE_ARGUMENT(TSharedPtr<FAssetViewItem>, AssetItem)

		/** How much padding to allow around the thumbnail */
		SLATE_ARGUMENT(float, ThumbnailPadding)

		/** The contents of the label displayed on the thumbnail */
		SLATE_ARGUMENT(ESketchfabThumbnailLabel::Type, ThumbnailLabel)

		/** The width of the item */
		SLATE_ATTRIBUTE(float, ItemWidth)

		/** Whether the item is selected in the view */
		SLATE_ARGUMENT(FIsSelected, IsSelected)


	SLATE_END_ARGS()

	/** Destructor */
	~SSKAssetTileItem();

	void Construct(const FArguments& InArgs);

	/** Handles committing a name change */
	virtual void OnAssetDataChanged() override;

private:
	/** SSKAssetViewItem interface */
	virtual float GetNameTextWrapWidth() const override { return LastGeometry.GetLocalSize().X - 2.f; }

	/** Returns the size of the thumbnail box widget */
	FOptionalSize GetThumbnailBoxSize() const;

	/** Returns the font to use for the thumbnail label */
	FSlateFontInfo GetThumbnailFont() const;

	/** Returns the size of the source control state box widget */
	FOptionalSize GetSCCImageSize() const;

	bool IsNameReadOnly() const
	{
		return true;
	}

private:
	/** The handle to the thumbnail that this item is rendering */
	TSharedPtr<FSketchfabAssetThumbnail> AssetThumbnail;

	/** The width of the item. Used to enforce a square thumbnail. */
	TAttribute<float> ItemWidth;

	/** The padding allotted for the thumbnail */
	float ThumbnailPadding;
};

/** Item that represents an asset that is being created */
/*
struct FAssetViewCreation : public FAssetViewAsset, public FGCObject
{
	FAssetViewCreation(const FSketchfabAssetData& AssetData)
		: FAssetViewAsset(AssetData)
	{}

	// FAssetViewItem interface
	virtual EAssetItemType::Type GetType() const override
	{
		return EAssetItemType::Creation;
	}

	virtual bool IsTemporaryItem() const override
	{
		return true;
	}

	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override
	{
		Collector.AddReferencedObject(AssetClass);
		Collector.AddReferencedObject(Factory);
	}
};
*/

// Additional data
struct LicenceDataInfo
{
	FString LicenceType;
	FString LicenceInfo;
};

class SAssetTileView;
class SSketchfabAssetView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSketchfabAssetView)
		: _ThumbnailLabel(ESketchfabThumbnailLabel::AssetName)
		, _ThumbnailScale(0.1f)
		, _ShowBottomToolbar(true)
		, _SelectionMode(ESelectionMode::Multi)
		, _AllowDragging(true)
		, _FillEmptySpaceInTileView(true)
	{}
		/** What the label on the asset thumbnails should be */
		SLATE_ARGUMENT(ESketchfabThumbnailLabel::Type, ThumbnailLabel)

		/** The thumbnail scale. [0-1] where 0.5 is no scale */
		SLATE_ATTRIBUTE(float, ThumbnailScale)

		/** Called to when an asset is selected */
		SLATE_EVENT(FOnSketchfabAssetSelected, OnAssetSelected)

		/** Called when the user double clicks, presses enter, or presses space on an asset */
		SLATE_EVENT(FOnSketchAssetsActivated, OnAssetsActivated)

		/** Called when an asset is right clicked */
		SLATE_EVENT(FOnGetSketchfabAssetContextMenu, OnGetAssetContextMenu)

		/** Called to get a custom asset item tool tip (if necessary) */
		SLATE_EVENT(FOnGetCustomSketchfabAssetToolTip, OnGetCustomAssetToolTip)

		/** Called when an asset item is about to show a tooltip */
		SLATE_EVENT(FOnVisualizeSketchfabAssetToolTip, OnVisualizeAssetToolTip)

		/** Called when an asset item's tooltip is closing */
		SLATE_EVENT(FOnSketchfabAssetToolTipClosing, OnAssetToolTipClosing)

		/** Should the toolbar indicating number of selected assets, mode switch buttons, etc... be shown? */
		SLATE_ARGUMENT(bool, ShowBottomToolbar)

		/** The selection mode the asset view should use */
		SLATE_ARGUMENT(ESelectionMode::Type, SelectionMode)

		/** Whether to allow dragging of items */
		SLATE_ARGUMENT(bool, AllowDragging)

		/** Whether this asset view should allow the thumbnails to consume empty space after the user scale is applied */
		SLATE_ARGUMENT(bool, FillEmptySpaceInTileView)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);
	//void CreateNewAsset(TSharedPtr<FSketchfabTaskData> Data);
	void ForceCreateNewAsset(TSharedPtr<FSketchfabTaskData> Data);
	void NeedRefresh();
	void DownloadProgress(const FString& ModelUID, float progress);
	void FlushThumbnails();

	void SetLicence(const FString& ModelUID, const FString &LicenceType, const FString &LicenceInfo);
	bool HasLicence(const FString& ModelUID);

	/** Returns all the asset data objects in items currently selected in the view */
	TArray<FSketchfabAssetData> GetSelectedAssets() const;

	TSharedPtr<FSketchfabAssetThumbnailPool> GetThumbnailPool() { return AssetThumbnailPool; }

private:
	void CreateCurrentView();
	TSharedRef<SAssetTileView> CreateTileView();
	TSharedRef<SWidget> CreateShadowOverlay(TSharedRef<STableViewBase> Table);

	float GetTileViewItemHeight() const;
	float GetTileViewItemBaseHeight() const;
	float GetTileViewItemWidth() const;
	float GetTileViewItemBaseWidth() const;
	float GetThumbnailScale() const;

	TSharedRef<ITableRow> MakeTileViewWidget(TSharedPtr<FAssetViewItem> AssetItem, const TSharedRef<STableViewBase>& OwnerTable);

	TSharedPtr<FSketchfabAssetThumbnail> AddItemToNewThumbnailRelevancyMap(const TSharedPtr<FAssetViewAsset>& Item, TMap< TSharedPtr<FAssetViewAsset>, TSharedPtr<FSketchfabAssetThumbnail> >& NewRelevantThumbnails);


	//void DeferredCreateNewAsset();
	void UpdateThumbnails();
	void UpdateAssetList();

private:
	/** Handler for double clicking an item */
	void OnListMouseButtonDoubleClick(TSharedPtr<FAssetViewItem> AssetItem);

	/** Handle dragging an asset */
	FReply OnDraggingAssetItem(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);

	/** Handler for tree view selection changes */
	void AssetSelectionChanged(TSharedPtr< struct FAssetViewItem > AssetItem, ESelectInfo::Type SelectInfo);

	/** Handler for when an item has scrolled into view after having been requested to do so */
	void ItemScrolledIntoView(TSharedPtr<struct FAssetViewItem> AssetItem, const TSharedPtr<ITableRow>& Widget);

	/** Handler for context menus */
	TSharedPtr<SWidget> OnGetContextMenuContent();

	/** Handler called when an asset context menu is about to open */
	bool CanOpenContextMenu() const;

private:
	/** Returns all the items currently selected in the view */
	TArray<TSharedPtr<FAssetViewItem>> GetSelectedItems() const;

private:
	// SWidget inherited
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	/** A struct to hold data for the deferred creation of assets */
	struct FCreateDeferredAssetData
	{
		/** The name of the asset */
		FString DefaultAssetName;

		/** The path where the asset will be created */
		FString PackagePath;

		/** The class of the asset to be created */
		UClass* AssetClass;

		/** The factory to use */
		UFactory* Factory;

		FString ModelAssetUID;
		FString ThumbAssetUID;
	};

	/** Asset pending deferred creation */
	TSharedPtr<FCreateDeferredAssetData> DeferredAssetToCreate;

	/** A struct to hold data for the deferred creation of a folder */
	struct FCreateDeferredFolderData
	{
		/** The name of the folder to create */
		FString FolderName;

		/** The path of the folder to create */
		FString FolderPath;
	};

	/** Folder pending deferred creation */
	TSharedPtr<FCreateDeferredFolderData> DeferredFolderToCreate;

private:
	TSharedPtr<class SAssetTileView> TileView;
	TSharedPtr<SBorder> ViewContainer;

	/** The asset items being displayed in the view and the filtered list */
	TArray<FSketchfabAssetData> QueriedAssetItems;
	TArray<FSketchfabAssetData> AssetItems;
	TArray<TSharedPtr<FAssetViewItem>> FilteredAssetItems;

	/** Pool for maintaining and rendering thumbnails */
	TSharedPtr<FSketchfabAssetThumbnailPool> AssetThumbnailPool;

	/** The number of thumbnails to keep for asset items that are not currently visible. Half of the thumbnails will be before the earliest item and half will be after the latest. */
	int32 NumOffscreenThumbnails;

	/** The current size of relevant thumbnails */
	int32 CurrentThumbnailSize;

	/** Flag to defer thumbnail updates until the next frame */
	bool bPendingUpdateThumbnails;

	/** Should the toolbar indicating number of selected assets, mode switch buttons, etc... be shown? */
	bool bShowBottomToolbar;

	/** The selection mode the asset view should use */
	ESelectionMode::Type SelectionMode;

	/** Whether to allow dragging of items */
	bool bAllowDragging;

	/** Whether this asset view should allow the thumbnails to consume empty space after the user scale is applied */
	bool bFillEmptySpaceInTileView;

	/** The size of thumbnails */
	int32 TileViewThumbnailResolution;
	int32 TileViewThumbnailSize;
	int32 TileViewThumbnailPadding;
	int32 TileViewNameHeight;

	/** The current value for the thumbnail scale from the thumbnail slider */
	TAttribute< float > ThumbnailScaleSliderValue;

	/** The max and min thumbnail scales as a fraction of the rendered size */
	float MinThumbnailScale;
	float MaxThumbnailScale;

	/** The amount to scale each thumbnail so that the empty space is filled. */
	float FillScale;

	/** A map of FAssetViewAsset to the thumbnail that represents it. Only items that are currently visible or within half of the FilteredAssetItems array index distance described by NumOffscreenThumbnails are in this list */
	TMap< TSharedPtr<FAssetViewAsset>, TSharedPtr<class FSketchfabAssetThumbnail> > RelevantThumbnails;

	/** The set of FAssetItems that currently have widgets displaying them. */
	TArray< TSharedPtr<FAssetViewItem> > VisibleItems;

	/** Whether the asset view is currently working on something and should display a cue to the user */
	bool bIsWorking;

	/** What the label on the thumbnails should be */
	ESketchfabThumbnailLabel::Type ThumbnailLabel;

	/** Called when an asset was selected in the list */
	FOnSketchfabAssetSelected OnAssetSelected;

	/** Called when the user double clicks, presses enter, or presses space on an asset */
	FOnSketchAssetsActivated OnAssetsActivated;

	/** Called when the user right clicks on an asset in the view */
	FOnGetSketchfabAssetContextMenu OnGetAssetContextMenu;

	/** Called to get a custom asset item tooltip (If necessary) */
	FOnGetCustomSketchfabAssetToolTip OnGetCustomAssetToolTip;

	/** Called when a custom asset item is about to show a tooltip */
	FOnVisualizeSketchfabAssetToolTip OnVisualizeAssetToolTip;

	/** Called when a custom asset item's tooltip is closing */
	FOnSketchfabAssetToolTipClosing OnAssetToolTipClosing;

	bool bBulkSelecting;
	bool bNeedsRefresh;
	bool bFlushThumbnails;

	/** Licensing information from the ModelInfo call */
	TMap<FString, LicenceDataInfo> LicenceData;

	/** Progress information for ModelUID download. Gets set on the RelevantThumbnails */
	TMap<FString, float> DownloadProgressData;

};

class SAssetTileView : public STileView<TSharedPtr<FAssetViewItem>>
{
public:
	virtual bool SupportsKeyboardFocus() const override { return true; }
	/*
	virtual FReply OnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FNavigationReply OnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	*/
};


struct FAssetViewItemHelper
{
public:
	static TSharedRef<SWidget> CreateTileItemContents(SSKAssetTileItem* const InTileItem, const TSharedRef<SWidget>& InThumbnail, FName& OutItemShadowBorder);

private:
	template <typename T>
	static TSharedRef<SWidget> CreateListTileItemContents(T* const InTileOrListItem, const TSharedRef<SWidget>& InThumbnail, FName& OutItemShadowBorder);
};


//TODO:
//Copy code from FFlipbookKeyFrameDragDropOp
class FSketchfabDragDropOperation : public FDragDropOperation
{
public:
	DRAG_DROP_OPERATOR_TYPE(FSketchfabDragDropOperation, FDragDropOperation)

	FSketchfabDragDropOperation();
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const override;
	virtual void OnDragged(const class FDragDropEvent& DragDropEvent);
	virtual void Construct() override;

public:
	TSharedPtr<FSketchfabAssetThumbnailPool> AssetThumbnailPool;
	TArray<FSketchfabAssetData> DraggedAssets;
	TArray<FString> DraggedAssetPaths;
};



FString GetSketchfabCacheDir();
FString GetSketchfabAssetZipPath(const FSketchfabAssetData &AssetData);
bool HasSketchAssetZipFile(const FSketchfabAssetData &AssetData);
