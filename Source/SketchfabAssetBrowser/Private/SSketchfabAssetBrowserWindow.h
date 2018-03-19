// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once 

#include "ISketchfabAssetBrowser.h"
#include "SSketchfabAssetView.h"

#include "Runtime/Online/HTTP/Public/Http.h"
#include "SketchfabTask.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSketchfabAssetBrowserWindow, Log, All);

class SSketchfabAssetBrowserWindow : public SCompoundWidget
{
public:
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
	FReply OnCancel();

	//Browser Window
	void OnUrlChanged(const FText &url);

	//AssetView 
	void OnAssetsActivated(const TArray<FSketchfabAssetData>& ActivatedAssets, EAssetTypeActivationMethod::Type ActivationMethod);
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FSketchfabAssetData>& SelectedAssets);

public:
	void CreateNewAsset(const FString& DefaultAssetName, const FString& PackagePath, UClass* AssetClass, UFactory* Factory, const FString& ModelAssetUID, const FString& ThumbAssetUID);
	void ForceCreateNewAsset(const FString& DefaultAssetName, const FString& PackagePath, const FString& ModelAssetUID, const FString& ThumbAssetUID);
	//void NewAssetRequested(const FString& SelectedPath, TWeakObjectPtr<UClass> FactoryClass);

public:
	/* The actual HTTP call */
	void GetModels(const FString &url);
	void GetThumbnail(const FString &url, const FString &uid);
	void GetUserData();

	void GetModelLink(const FString &uid);
	void GetModel(const FString &url, const FString &uid);

private:
	FString Token;
	FHttpModule* Http;

	/*Assign this function to call when the GET request processes sucessfully*/
	void OnModelsReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnThumbnailReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnUserDataReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnModelLinkReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	void OnModelReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void OnModelProgress(FHttpRequestPtr HttpRequest, int32 BytesSent, int32 BytesReceived);

	void AddAuthorization(TSharedRef<IHttpRequest> Request);

private:
	//SketchfabRESTClient callbacks
	void OnModelList(const FSketchfabTask& InSwarmTask);
	void OnDownloadThumbnail(const FSketchfabTask& InSwarmTask);
	void OnSketchfabTaskFailed(const FSketchfabTask& InSwarmTask);

private:
	TWeakPtr<SWindow> OAuthWindowPtr;

	TWeakPtr< SWindow > Window;

	/** The asset view widget */
	TSharedPtr<SSketchfabAssetView> AssetViewPtr;


	/** List of pending Http requests model files being downloaded */
	TMap<FHttpRequestPtr, FString> ModelRequests;

	//UUID of thumbnail and asset data to create then thumbnail downloaded
	TMap<FString, FSketchfabAssetData> AssetsToCreate;


	TQueue<TWeakPtr<class IHttpRequest>> AllRequests;

	FString CacheFolder;
};

