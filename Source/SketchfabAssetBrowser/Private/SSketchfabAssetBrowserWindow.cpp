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

#define LOCTEXT_NAMESPACE "SketchfabAssetBrowser"

DEFINE_LOG_CATEGORY(LogSketchfabAssetBrowserWindow);

int32 g_pageCount = 0;

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
			.Text(LOCTEXT("SSketchfabAssetBrowserWindow_Login", "Login"))
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

	//When the object is constructed, Get the HTTP module
	Http = &FHttpModule::Get();

	GetModels("https://api.sketchfab.com/v3/search?type=models&downloadable=true&staffpicked=true&sort_by=-publishedAt");
}

void SSketchfabAssetBrowserWindow::OnAssetsActivated(const TArray<FSketchfabAssetData>& ActivatedAssets, EAssetTypeActivationMethod::Type ActivationMethod)
{
	const FText LoadingTemplate = LOCTEXT("LoadingAssetName", "Loading {0}...");
	const FText DefaultText = ActivatedAssets.Num() == 1 ? FText::Format(LoadingTemplate, FText::FromName(ActivatedAssets[0].ObjectUID)) : LOCTEXT("DownloadingModels", "Downloading Models...");
	FScopedSlowTask SlowTask(100, DefaultText);

	for (auto AssetIt = ActivatedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
	{
		const FSketchfabAssetData& AssetData = *AssetIt;
		GetModelLink(AssetData.ObjectUID.ToString());
	}

	// Iterate over all activated assets to map them to AssetTypeActions.
	// This way individual asset type actions will get a batched list of assets to operate on
	/*
	for (auto AssetIt = ActivatedAssets.CreateConstIterator(); AssetIt; ++AssetIt)
	{
		const FSketchfabAssetData& AssetData = *AssetIt;
		if (!AssetData.IsAssetLoaded() && FEditorFileUtils::IsMapPackageAsset(AssetData.ObjectPath.ToString()))
		{
			SlowTask.MakeDialog();
		}

		SlowTask.EnterProgressFrame(75.f / ActivatedAssets.Num(), FText::Format(LoadingTemplate, FText::FromName(AssetData.AssetName)));

		UObject* Asset = (*AssetIt).GetAsset();

		if (Asset != NULL)
		{
			FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
			TWeakPtr<IAssetTypeActions> AssetTypeActions = AssetToolsModule.Get().GetAssetTypeActionsForClass(Asset->GetClass());
			if (AssetTypeActions.IsValid())
			{
				// Add this asset to the list associated with the asset type action object
				TArray<UObject*>& ObjList = TypeActionsToObjects.FindOrAdd(AssetTypeActions.Pin().ToSharedRef());
				ObjList.AddUnique(Asset);
			}
			else
			{
				ObjectsWithoutTypeActions.AddUnique(Asset);
			}
		}
	}

	// Now that we have created our map, activate all the lists of objects for each asset type action.
	for (auto TypeActionsIt = TypeActionsToObjects.CreateConstIterator(); TypeActionsIt; ++TypeActionsIt)
	{
		SlowTask.EnterProgressFrame(25.f / TypeActionsToObjects.Num());

		const TSharedRef<IAssetTypeActions>& TypeActions = TypeActionsIt.Key();
		const TArray<UObject*>& ObjList = TypeActionsIt.Value();

		TypeActions->AssetsActivated(ObjList, ActivationMethod);
	}

	// Finally, open a simple asset editor for all assets which do not have asset type actions if activating with enter or double click
	if (ActivationMethod == EAssetTypeActivationMethod::DoubleClicked || ActivationMethod == EAssetTypeActivationMethod::Opened)
	{
		ContentBrowserUtils::OpenEditorForAsset(ObjectsWithoutTypeActions);
	}
	*/
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

void SSketchfabAssetBrowserWindow::CreateNewAsset(const FString& DefaultAssetName, const FString& PackagePath, UClass* AssetClass, UFactory* Factory, const FString& ModelAssetUID, const FString& ThumbAssetUID)
{
	AssetViewPtr->CreateNewAsset(DefaultAssetName, PackagePath, AssetClass, Factory, ModelAssetUID, ThumbAssetUID);
}

void SSketchfabAssetBrowserWindow::ForceCreateNewAsset(const FString& DefaultAssetName, const FString& PackagePath, const FString& ModelAssetUID, const FString& ThumbAssetUID)
{
	AssetViewPtr->ForceCreateNewAsset(DefaultAssetName, PackagePath, ModelAssetUID, ThumbAssetUID);
}

/*
void SSketchfabAssetBrowserWindow::NewAssetRequested(const FString& SelectedPath, TWeakObjectPtr<UClass> FactoryClass)
{
	if (ensure(SelectedPath.Len() > 0) && ensure(FactoryClass.IsValid()))
	{
		UFactory* NewFactory = NewObject<UFactory>(GetTransientPackage(), FactoryClass.Get());
		FEditorDelegates::OnConfigureNewAssetProperties.Broadcast(NewFactory);
		if (NewFactory->ConfigureProperties())
		{
			FString DefaultAssetName;
			FString PackageNameToUse;

			static FName AssetToolsModuleName = FName("AssetTools");
			FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>(AssetToolsModuleName);
			AssetToolsModule.Get().CreateUniqueAssetName(SelectedPath + TEXT("/") + NewFactory->GetDefaultNewAssetName(), TEXT(""), PackageNameToUse, DefaultAssetName);
			CreateNewAsset(DefaultAssetName, SelectedPath, NewFactory->GetSupportedClass(), NewFactory);
		}
	}
}
*/


//=================================================================================

void SSketchfabAssetBrowserWindow::AddAuthorization(TSharedRef<IHttpRequest> Request)
{
	if (!Token.IsEmpty())
	{
		FString bearer = "Bearer ";
		bearer += Token;
		Request->SetHeader("Authorization", bearer);
	}
}

/*Http call*/
void SSketchfabAssetBrowserWindow::GetModels(const FString &url)
{
	TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	Request->OnProcessRequestComplete().BindRaw(this, &SSketchfabAssetBrowserWindow::OnModelsReceived);

	//This is the url on which to process the request
	Request->SetURL(url);
	Request->SetVerb("GET");
	Request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	Request->SetHeader("Content-Type", "application/json");

	AddAuthorization(Request);

	Request->ProcessRequest();
}

/*Assigned function on successfull http call*/
void SSketchfabAssetBrowserWindow::OnModelsReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful)
		return;

	if (Response->GetContentType() != "application/json")
	{
		return;
	}

	//Create a pointer to hold the json serialized data
	TSharedPtr<FJsonObject> JsonObject;

	//Create a reader pointer to read the json data
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	//Deserialize the json data given Reader and the actual object to deserialize
	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		TArray<TSharedPtr<FJsonValue>> results = JsonObject->GetArrayField("results");
		FString next = JsonObject->GetStringField("next");

		for (int r = 0; r < results.Num(); r++)
		{
			TSharedPtr<FJsonObject> resultObj = results[r]->AsObject();

			FString modelUID = resultObj->GetStringField("uid");
			FString modelName = resultObj->GetStringField("name");

			FString thumbUID;
			FString thumbURL;
			int32 oldWidth = 0;
			int32 oldHeight = 0;

			TSharedPtr<FJsonObject> thumbnails = resultObj->GetObjectField("thumbnails");
			TArray<TSharedPtr<FJsonValue>> images = thumbnails->GetArrayField("images");
			for (int a = 0; a < images.Num(); a++)
			{
				TSharedPtr<FJsonObject> imageObj = images[a]->AsObject();
				FString url = imageObj->GetStringField("url");
				FString uid = imageObj->GetStringField("uid");
				int32 width = imageObj->GetIntegerField("width");
				int32 height = imageObj->GetIntegerField("height");

				if (thumbUID.IsEmpty())
				{
					thumbUID = uid;
					thumbURL = url;
				}

				if (width > oldWidth && width <= 512 && height > oldHeight && height <= 512)
				{
					thumbUID = uid;
					thumbURL = url;
					oldWidth = width;
					oldHeight = height;
				}
			}

			bool needDownload = false;

			FString jpg = thumbUID + ".jpg";
			FString FileName = CacheFolder / jpg;

			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
			if (!PlatformFile.FileExists(*FileName))
			{
				needDownload = true;
				GetThumbnail(thumbURL, thumbUID);
			}

			if (!needDownload)
			{
				ForceCreateNewAsset(modelName, CacheFolder, modelUID, thumbUID);
				AssetViewPtr->NeedRefresh();
			}
			else
			{
				FSketchfabAssetData asset;
				asset.ObjectUID = FName(*modelUID);
				asset.ThumbUID = FName(*thumbUID);
				asset.AssetName = FName(*modelName);
				asset.PackagePath = FName(*CacheFolder);
				AssetsToCreate.Add(thumbUID, asset);
			}
		}

		if ((--g_pageCount) > 0)
		{
			GetModels(next);
		}
	}
}


/*Http call*/
void SSketchfabAssetBrowserWindow::GetModelLink(const FString &uid)
{
	TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	Request->OnProcessRequestComplete().BindRaw(this, &SSketchfabAssetBrowserWindow::OnModelLinkReceived);
	//This is the url on which to process the request

	FString url = "https://api.sketchfab.com/v3/models/";
	url += uid;
	url += "/download";

	Request->SetURL(url);
	Request->SetVerb("GET");
	Request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	Request->SetHeader("Content-Type", "application/json");
	Request->SetHeader("UID", uid);

	AddAuthorization(Request);

	Request->ProcessRequest();
}

/*Assigned function on successfull http call*/
void SSketchfabAssetBrowserWindow::OnModelLinkReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful)
		return;

	if (Response->GetContentType() != "application/json")
	{
		return;
	}

	FString uid = Request->GetHeader("UID");

	//Create a pointer to hold the json serialized data
	TSharedPtr<FJsonObject> JsonObject;

	//Create a reader pointer to read the json data
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	//Deserialize the json data given Reader and the actual object to deserialize
	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		TSharedPtr<FJsonObject> gltfObject = JsonObject->GetObjectField("gltf");

		FString url = gltfObject->GetStringField("url");
		int32 size = gltfObject->GetIntegerField("size");
		int32 expires = gltfObject->GetIntegerField("expires");

		//Schedule the file to be downloaded
		GetModel(url, uid);
	}
}


/*Http call*/


//Notes
//FOnlineTitleFileHttp may be helpful as a reference, it downloads from the cloud and also handles progress.
void SSketchfabAssetBrowserWindow::GetModel(const FString &url, const FString &uid)
{
	TSharedRef<IHttpRequest> Request = Http->CreateRequest();

	ModelRequests.Add(Request, uid);

	Request->OnProcessRequestComplete().BindRaw(this, &SSketchfabAssetBrowserWindow::OnModelReceived);
	Request->OnRequestProgress().BindRaw(this, &SSketchfabAssetBrowserWindow::OnModelProgress);
	//This is the url on which to process the request
	Request->SetURL(url);
	Request->SetVerb("GET");
	bool bStarted = Request->ProcessRequest();
	if (!bStarted)
	{
		//UE_LOG(LogSketchfabAssetBrowserWindow, Warning, TEXT("GetModel request failed. %s"), *ErrorStr);
	}
}

void SSketchfabAssetBrowserWindow::OnModelProgress(FHttpRequestPtr HttpRequest, int32 BytesSent, int32 BytesReceived)
{
	FString uid = ModelRequests.FindChecked(HttpRequest);
	if (!uid.IsEmpty())
	{
		//Relay some progress
		//TriggerOnReadFileProgressDelegates(PendingRequest.FileName, BytesReceived);
		return;
	}

	//FPendingFileRequest PendingRequest = FileProgressRequestsMap.FindChecked(HttpRequest);
	// Just forward this to anyone that is listening
	//TriggerOnReadFileProgressDelegates(PendingRequest.FileName, BytesReceived);
}

void SSketchfabAssetBrowserWindow::OnModelReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	FString uid = ModelRequests.FindChecked(Request);
	ModelRequests.Remove(Request);

	if (!bWasSuccessful)
		return;

	FString contentType = Response->GetContentType();
	if (contentType == "application/xml")
	{
		const TArray<uint8> &blob = Response->GetContent();
		FString data = BytesToString(blob.GetData(), blob.Num());
		FString Fixed;
		for (int i = 0; i < data.Len(); i++)
		{
			const TCHAR c = data[i] - 1;
			Fixed.AppendChar(c);
		}

		return;
	}

	const TArray<uint8> &data = Response->GetContent();
	if (data.Num() > 0)
	{
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		FString jpg = uid + ".zip";
		FString FileName = CacheFolder / jpg;
		IFileHandle* FileHandle = PlatformFile.OpenWrite(*FileName);
		if (FileHandle)
		{
			// Write the bytes to the file
			FileHandle->Write(data.GetData(), data.Num());

			// Close the file again
			delete FileHandle;
		}
	}
}

/*Http call*/
void SSketchfabAssetBrowserWindow::GetThumbnail(const FString &url, const FString &uid)
{
	TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	Request->OnProcessRequestComplete().BindRaw(this, &SSketchfabAssetBrowserWindow::OnThumbnailReceived);

	//This is the url on which to process the request
	Request->SetURL(url);
	Request->SetVerb("GET");
	Request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	Request->SetHeader("Content-Type", "image/jpeg");
	Request->SetHeader("UID", uid);

	AddAuthorization(Request);

	Request->ProcessRequest();
}

void SSketchfabAssetBrowserWindow::OnThumbnailReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful)
		return;

	FString uid = Request->GetHeader("UID");

	if (Response->GetContentType() != "image/jpeg")
	{
		return;
	}

	const TArray<uint8> &data = Response->GetContent();
	if (data.Num() > 0)
	{
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		FString jpg = uid + ".jpg";
		FString FileName = CacheFolder / jpg;
		IFileHandle* FileHandle = PlatformFile.OpenWrite(*FileName);
		if (FileHandle)
		{
			// Write the bytes to the file
			FileHandle->Write(data.GetData(), data.Num());

			// Close the file again
			delete FileHandle;
		}
	}

	//This is not threadsafe
	const FSketchfabAssetData *asset = AssetsToCreate.Find(uid);
	if (asset)
	{
		ForceCreateNewAsset(asset->AssetName.ToString(), asset->PackagePath.ToString(), asset->ObjectUID.ToString(), asset->ThumbUID.ToString());
		AssetViewPtr->NeedRefresh();
		AssetsToCreate.Remove(uid);
	}
}


/*Http call*/
void SSketchfabAssetBrowserWindow::GetUserData()
{
	TSharedRef<IHttpRequest> Request = Http->CreateRequest();
	Request->OnProcessRequestComplete().BindRaw(this, &SSketchfabAssetBrowserWindow::OnUserDataReceived);

	FString url = "https://sketchfab.com/v3/me";

	Request->SetURL(url);
	Request->SetVerb("GET");
	Request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	Request->SetHeader("Content-Type", "image/jpeg");

	AddAuthorization(Request);

	Request->ProcessRequest();
}

void SSketchfabAssetBrowserWindow::OnUserDataReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful)
		return;

	//Create a pointer to hold the json serialized data
	TSharedPtr<FJsonObject> JsonObject;

	//Create a reader pointer to read the json data
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

	//Deserialize the json data given Reader and the actual object to deserialize
	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		FString name = JsonObject->GetStringField("displayName");

		return;
	}
}


#undef LOCTEXT_NAMESPACE

