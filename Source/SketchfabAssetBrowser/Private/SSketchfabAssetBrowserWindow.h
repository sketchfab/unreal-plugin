// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "ISketchfabAssetBrowser.h"
#include "SSketchfabAssetView.h"

#include "Runtime/Online/HTTP/Public/Http.h"
#include "SketchfabTask.h"
#include "SSketchfabAssetSearchBox.h"
#include "SSketchfabAssetWindow.h"
#include "SOAuthWebBrowser.h"
#include "SComboBox.h"
#include "ContentBrowserModule.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSketchfabAssetBrowserWindow, Log, All);

class SSketchfabAssetBrowserWindow : public SCompoundWidget
{
public:
	~SSketchfabAssetBrowserWindow();

	SLATE_BEGIN_ARGS(SSketchfabAssetBrowserWindow)
	{}

	SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)

	SLATE_END_ARGS()

public:
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

public:
	void Construct(const FArguments& InArgs);

	//Main Window
	FReply OnLogin();
	FReply OnLogout();

	bool OnLoginEnabled() const;
	bool OnLogoutEnabled() const;

	bool hasMoreResults() const;

	FReply OnCancel();
	FReply OnNext();
	FReply OnDownloadSelected();
	FReply OnClearCache();
	FReply CheckLatestPluginVersion();
	FReply GetLatestPluginVersion();
	FReply OnUpgradeToPro();

	//Search Box
	FReply OnSearchPressed();
	bool SetSearchBoxText(const FText& InSearchText);
	void OnSearchBoxChanged(const FText& InSearchText);
	void OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo);

	//Login
	FText GetLoginText() const;
	FText GetLoginButtonText() const;
	FText GetMyModelText() const;

	//Browser Window
	void OnUrlChanged(const FText &url);

	//AssetView
	void OnAssetsActivated(const TArray<FSketchfabAssetData>& ActivatedAssets, EAssetTypeActivationMethod::Type ActivationMethod);
	void OnAssetsSelected(const FSketchfabAssetData &SelectedAsset);
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FSketchfabAssetData>& SelectedAssets);

public:
	void Search();
	void GetUserData();
	void GetCategories();
	void GetModelLicence(const FString &ModelUID);

private:
	void Search(const FString &url);
	void DownloadModel(const FString &ModelUID, const FDateTime &ModelPublishedAt);
	void ShowModelWindow(const FSketchfabAssetData& AssetData);
	void DoLoginLogout(const FString &url);
	void GetModelSize(const FString &ModelUID);

	FString GetModelZipFileName(const FString &ModelUID);

private:
	//Search Options
	enum ESortBy {
		SORTBY_Relevance,
		SORTBY_MostLiked,
		SORTBY_MostViewed,
		SORTBY_MostRecent,
		SORTBY_UNDEFINED,
	};

	enum EFaceCount {
		FACECOUNT_ALL,
		FACECOUNT_0_10,
		FACECOUNT_10_50,
		FACECOUNT_50_100,
		FACECOUNT_100_250,
		FACECOUNT_250,
		FACECOUNT_UNDEFINED,
	};

	void setDefaultParams(bool myModels);
	bool IsSearchMyModelsAvailable() const;
	EVisibility ShouldDisplayUpgradeToPro() const;
	ECheckBoxState IsSearchMyModelsChecked() const;
	void OnSearchMyModelsCheckStateChanged(ECheckBoxState NewState);

	ECheckBoxState IsSearchAnimatedChecked() const;
	void OnSearchAnimatedCheckStateChanged(ECheckBoxState NewState);

	ECheckBoxState IsSearchStaffPickedChecked() const;
	void OnSearchStaffPickedCheckStateChanged(ECheckBoxState NewState);

	EVisibility GetNewVersionButtonVisibility() const;
	FText GetCurrentVersionText() const;

	bool bSearchMyModels;
	bool bSearchAnimated;
	bool bSearchStaffPicked;

	// Sort By
	TSharedRef<SWidget> GenerateSortByComboItem(TSharedPtr<FString> InItem);
	void HandleSortByComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	FText GetSortByComboText() const;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> SortByComboBox;
	TArray<TSharedPtr<FString>> SortByComboList;
	int32 SortByIndex;
	FString CurrentSortByString;

	// Category
	TSharedRef<SWidget> GenerateCategoryComboItem(TSharedPtr<FString> InItem);
	void HandleCategoryComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	FText GetCategoryComboText() const;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> CategoriesComboBox;
	TArray<FSketchfabCategory> Categories;
	TArray<TSharedPtr<FString>> CategoryComboList;
	int32 CategoryIndex;
	FString CurrentCategoryString;

	// Face Count
	TSharedRef<SWidget> GenerateFaceCountComboItem(TSharedPtr<FString> InItem);
	void HandleFaceCountComboChanged(TSharedPtr<FString> Item, ESelectInfo::Type SelectInfo);
	FText GetFaceCountComboText() const;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> FaceCountComboBox;
	TArray<TSharedPtr<FString>> FaceCountComboList;
	int32 FaceCountIndex;
	FString CurrentFaceCountString;


	bool ShouldDownloadFile(const FString &FileName, const FDateTime &ModelPublishedAt);

private:
	FString Token;

	//SketchfabRESTClient callbacks
	void OnCheckLatestVersion(const FSketchfabTask& InTask);
	void OnSearch(const FSketchfabTask& InTask);
	void OnThumbnailDownloaded(const FSketchfabTask& InTask);
	void OnModelLink(const FSketchfabTask& InTask);
	void OnModelDownloaded(const FSketchfabTask& InTask);
	void OnModelDownloadProgress(const FSketchfabTask& InTask);
	void OnUserData(const FSketchfabTask& InTask);
	void OnUserThumbnailDownloaded(const FSketchfabTask& InTask);
	void OnCategories(const FSketchfabTask& InTask);
	void OnGetModelSize(const FSketchfabTask& InTask);

	void OnTaskFailed(const FSketchfabTask& InTask);
	void OnDownloadFailed(const FSketchfabTask& InTask);

	//Call for the Asset Window
	void GetModelInfo(const FString &ModelUID);
	void GetBigThumbnail(const FSketchfabTaskData &data);

	//Callbacks for the Asset Window
	void OnGetModelInfo(const FSketchfabTask& InTask);
	void OnGetBigThumbnail(const FSketchfabTask& InTask);

	// The Asset Window is telling the main window to download a file
	void OnDownloadRequest(const FString &ModelUID, const FString &ModelPublishedAt);

	void OnOAuthWindowClosed(const TSharedRef<SWindow>& InWindow);

private:
	bool OnContentBrowserDrop(const FAssetViewDragAndDropExtender::FPayload &PayLoad);
	bool OnContentBrowserDragOver(const FAssetViewDragAndDropExtender::FPayload &PayLoad);
	bool OnContentBrowserDragLeave(const FAssetViewDragAndDropExtender::FPayload &PayLoad);

private:
	TWeakPtr<SWindow> Window;

	TSharedPtr<SWindow> OAuthWindowPtr;
	TSharedPtr<SSketchfabAssetWindow> AssetWindow;

	TSharedPtr<STextBlock> FaceCountText;

	TSharedPtr<SSketchfabAssetView> AssetViewPtr;

	/** The text box used to search for assets */
	TSharedPtr<SSketchfabAssetSearchBox> SearchBoxPtr;

	FString CurrentPluginVersion;

	FString LatestPluginVersion;

	FString QuerySearchText;

	FString CacheFolder;

	FString NextURL;

	FString LoggedInUserDisplayName;

	FString LoggedInUserName;

	FString LoggedInUserAccountType;

	bool IsLoggedUserPro;

	TSet<FString> ModelsDownloading;
};

