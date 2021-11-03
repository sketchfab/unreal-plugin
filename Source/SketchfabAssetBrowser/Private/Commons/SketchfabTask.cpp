// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SketchfabTask.h"
#include "HAL/RunnableThread.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include <Misc/EngineVersion.h>

#include "Misc/FileHelper.h"
#include <Runtime/Core/Public/Misc/Paths.h>

#define HOSTNAME "http://127.0.0.1"
#define PORT ":55002"

DEFINE_LOG_CATEGORY_STATIC(LogSketchfabTask, Verbose, All);

bool IsCompleteState(SketchfabRESTState state)
{
	switch (state)
	{
	case SRS_SEARCH_DONE:
	case SRS_GETTHUMBNAIL_DONE:
	case SRS_GETMODELLINK_DONE:
	case SRS_DOWNLOADMODEL_DONE:
	case SRS_GETUSERDATA_DONE:
	case SRS_GETUSERORGS_DONE:
	case SRS_GETUSERTHUMB_DONE:
	case SRS_UPLOADMODEL_DONE:
	case SRS_GETCATEGORIES_DONE:
		return true;
	}
	return false;
}

FSketchfabTask::FSketchfabTask(const FSketchfabTaskData& InTaskData)
	:
	TaskData(InTaskData),
	State(SRS_UNKNOWN)
{
	TaskData.TaskUploadComplete = false;
	DebugHttpRequestCounter.Set(0);
	IsCompleted.AtomicSet(false);
}

FSketchfabTask::~FSketchfabTask()
{
	UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, Warning, TEXT("Destroying Task With Model Id %s"), *TaskData.ModelUID);

	TArray<ReqPtr> OutPendingRequests;
	if (PendingRequests.GetKeys(OutPendingRequests) > 0)
	{
		for (int32 ItemIndex = 0; ItemIndex < OutPendingRequests.Num(); ++ItemIndex)
		{
			UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, Warning, TEXT("Pending Request for task with status %d"), (int32)OutPendingRequests[ItemIndex]->GetStatus());
			OutPendingRequests[ItemIndex]->CancelRequest();
		}
	}

	//unbind the delegates
	OnTaskFailed().Unbind();
	OnSearch().Unbind();
	OnThumbnailDownloaded().Unbind();
	OnModelLink().Unbind();
	OnModelDownloaded().Unbind();
	OnModelDownloadProgress().Unbind();
	OnUserData().Unbind();
	OnUserOrgs().Unbind();
	OnOrgsProjects().Unbind();
	OnModelUploaded().Unbind();
	OnUserThumbnailDelegate.Unbind();
	OnCategories().Unbind();
	OnModelInfo().Unbind();
}

SketchfabRESTState FSketchfabTask::GetState() const
{
	FScopeLock Lock(TaskData.StateLock);
	return State;
}

void FSketchfabTask::EnableDebugLogging()
{
	bEnableDebugLogging = true;
}

void FSketchfabTask::SetState(SketchfabRESTState InState)
{
	if (this != nullptr && TaskData.StateLock)
	{
		FScopeLock Lock(TaskData.StateLock);

		//do not update the state if either of these set is already set
		if (InState != State && (State != SRS_FAILED))
		{
			State = InState;
		}
		else if ((InState != State) && (State != SRS_FAILED) && (IsCompleteState(InState)))
		{
			State = InState;
			this->IsCompleted.AtomicSet(true);
		}
		/*
		else if ((InState != State) && (State != SRS_FAILED) && (InState == SRS_ASSETDOWNLOADED))
		{
			State = SRS_ASSETDOWNLOADED;
			this->IsCompleted.AtomicSet(true);
		}
		else if (InState != State && InState == SRS_FAILED)
		{
			State = SRS_ASSETDOWNLOADED;
		}
		*/
	}
	else
	{
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, Error, TEXT("Object in invalid state can not acquire state lock"));
	}

}

bool FSketchfabTask::IsFinished() const
{
	FScopeLock Lock(TaskData.StateLock);
	return IsCompleted;
}

// ~ HTTP Request method to communicate with Sketchfab REST Interface

void FSketchfabTask::AddAuthorization(ReqRef Request)
{
	if (!TaskData.Token.IsEmpty())
	{
		FString bearer = "Bearer ";
		bearer += TaskData.Token;
		Request->SetHeader("Authorization", bearer);
	}
	else {
		UE_LOG(LogSketchfabRESTClient, Display, TEXT("Authorization Token is empty"));
	}
}

// Helpers for http requests
bool FSketchfabTask::MakeRequest(
	FString url, 
	SketchfabRESTState successState, 
	void (FSketchfabTask::* completeCallback)(FHttpRequestPtr, FHttpResponsePtr, bool), 
	FString contentType, 
	void (FSketchfabTask::* progressCallback)(FHttpRequestPtr, int32, int32), 
	bool upload, 
	bool authorization) {
	
	// Form the request
	ReqRef request = FHttpModule::Get().CreateRequest();

	if (authorization)
		AddAuthorization(request);
	if(upload)
		SetUploadRequestContent(request);

	request->SetURL(url);
	request->SetVerb((upload ? TEXT("POST") : TEXT("GET")));
	request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	if (!contentType.IsEmpty())
		request->SetHeader("Content-Type", contentType);
	
	// Callbacks
	request->OnProcessRequestComplete().BindRaw(this, completeCallback);
	if(progressCallback)
		request->OnRequestProgress().BindRaw(this, progressCallback);

	UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("%s"), *request->GetURL());

	// Success / Failure
	if (!request->ProcessRequest())
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Failed to process Request %s"), *request->GetURL());
		return false;
	}
	else
	{
		DebugHttpRequestCounter.Increment();
		PendingRequests.Add(request, request->GetURL());
		SetState(successState);
		return true;
	}
}

bool FSketchfabTask::IsValid(FHttpRequestPtr Request, FHttpResponsePtr Response, FString expectedType, bool checkData, bool checkCache) {

	DebugHttpRequestCounter.Decrement();
	FString OutUrl = PendingRequests.FindRef(Request);
	if (!OutUrl.IsEmpty())
	{
		PendingRequests.Remove(Request);
	}
	if (!Response.IsValid())
	{
		UE_LOG(LogSketchfabRESTClient, Display, TEXT("Invalid response"));
		return false;
	}

	if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		// Was object destroyed ?
		if (this == nullptr)
		{
			UE_LOG(LogSketchfabRESTClient, Display, TEXT("Object has already been destroyed"));
			SetState(SRS_FAILED);
			return false;
		}
		// Check expected type
		if (!expectedType.IsEmpty()) {
			if (Response->GetContentType() != expectedType)
			{
				UE_LOG(LogSketchfabRESTClient, Display, TEXT("Content type is not as expected"));
				SetState(SRS_FAILED);
				return false;
			}
		}
		// Check data
		if (checkData) {
			FString data = Response->GetContentAsString();
			if (data.IsEmpty())
			{
				UE_LOG(LogSketchfabRESTClient, Display, TEXT("GetModels_Response content was empty"));
				SetState(SRS_FAILED);
				return false;
			}
		}
		// Check cache
		if (checkCache && TaskData.CacheFolder.IsEmpty())
		{
			ensure(false);
			UE_LOG(LogSketchfabRESTClient, Display, TEXT("Cache Folder not set internally"));
			SetState(SRS_FAILED);
			return false;
		}
		return true;
	}
	else
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());
		return false;
	}
}



/*Http call*/
void FSketchfabTask::CheckLatestPluginVersion()
{
	MakeRequest(
		"https://api.github.com/repos/sketchfab/Sketchfab-Unreal/releases",
		SRS_CHECK_LATEST_VERSION_PROCESSING,
		&FSketchfabTask::Check_Latest_Version_Response,
		"",
		nullptr,
		false,
		false
	);
}
void FSketchfabTask::Search()
{
	MakeRequest(
		TaskData.ModelSearchURL,
		SRS_SEARCH_PROCESSING,
		&FSketchfabTask::Search_Response,
		"application/json"
	);
}


void GetThumnailData(TSharedPtr<FJsonObject> JsonObject, int32 size, FString &thumbUID, FString &thumbURL, int32 &outWidth, int32 &outHeight)
{
	outWidth = 0;
	outHeight = 0;

	TSharedPtr<FJsonObject> thumbnails = JsonObject->GetObjectField("thumbnails");
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

		if (width > outWidth && width <= size && height > outHeight && height <= size)
		{
			thumbUID = uid;
			thumbURL = url;
			outWidth = width;
			outHeight = height;
		}
	}
}

void FSketchfabTask::Check_Latest_Version_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if(Response)
	{
		DebugHttpRequestCounter.Decrement();
		if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
		{
			if (this == nullptr)
			{
				UE_LOG(LogSketchfabRESTClient, Display, TEXT("Object has already been destroyed"));
				SetState(SRS_FAILED);
				return;
			}

			FString data = Response->GetContentAsString();
			if (data.IsEmpty())
			{
				UE_LOG(LogSketchfabRESTClient, Display, TEXT("CheckLatestVersion_Response content was empty"));
				SetState(SRS_FAILED);
			}
			else
			{
				//Create a pointer to hold the json serialized data
				TArray<TSharedPtr<FJsonValue>> results;

				//Create a reader pointer to read the json data
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

				//Deserialize the json data given Reader and the actual object to deserialize
				int separatorIndex = -1;
				if (FJsonSerializer::Deserialize(Reader, results))
				{
					for (int r = 0; r < results.Num(); r++)
					{
						TSharedPtr<FJsonObject> resultObj = results[r]->AsObject();
						FString release_tag = resultObj->GetStringField("tag_name");
						TaskData.LatestPluginVersion = release_tag;
						break;
					}
				}

				if (!this->IsCompleted &&  OnCheckLatestVersion().IsBound())
				{
					OnCheckLatestVersion().Execute(*this);
				}

				SetState(SRS_CHECK_LATEST_VERSION_DONE);
				return;
			}
		}
		else
		{
			SetState(SRS_FAILED);
			UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());
		}
	}
	else
	{
		UE_LOG(LogSketchfabRESTClient, Display, TEXT("Invalid responde while checking plugin version, check your internet connection"));
		SetState(SRS_FAILED);	
	}
}


/*Assigned function on successfull http call*/
void FSketchfabTask::Search_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if(IsValid(Request, Response, "application/json", true)){

		//Create a pointer to hold the json serialized data
		TSharedPtr<FJsonObject> JsonObject;

		//Create a reader pointer to read the json data
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		//Deserialize the json data given Reader and the actual object to deserialize
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			TArray<TSharedPtr<FJsonValue>> results = JsonObject->GetArrayField("results");
			TaskData.NextURL = JsonObject->GetStringField("next");

			for (int r = 0; r < results.Num(); r++)
			{
				TSharedPtr<FJsonObject> resultObj = results[r]->AsObject();

				FString modelUID = resultObj->GetStringField("uid");
				FString modelName = resultObj->GetStringField("name");

				FString authorName;
				TSharedPtr<FJsonObject> userObj = resultObj->GetObjectField("user");
				if (userObj.IsValid())
				{
					authorName = userObj->GetStringField("username");
				}

				TSharedPtr<FSketchfabTaskData> OutItem = MakeShareable(new FSketchfabTaskData());

				GetThumnailData(resultObj, 512, OutItem->ThumbnailUID, OutItem->ThumbnailURL, OutItem->ThumbnailWidth, OutItem->ThumbnailHeight);
				GetThumnailData(resultObj, 1024, OutItem->ThumbnailUID_1024, OutItem->ThumbnailURL_1024, OutItem->ThumbnailWidth_1024, OutItem->ThumbnailHeight_1024);

				OutItem->ModelPublishedAt = GetPublishedAt(resultObj);
				OutItem->ModelName = modelName;
				OutItem->ModelUID = modelUID;
				OutItem->CacheFolder = TaskData.CacheFolder;
				OutItem->Token = TaskData.Token;
				OutItem->ModelAuthor = authorName;

				TSharedPtr<FJsonObject> archives = resultObj->GetObjectField("archives");
				if (archives.IsValid())
				{
					TSharedPtr<FJsonObject> gltf = archives->GetObjectField("gltf");
					if (gltf.IsValid())
					{
						OutItem->ModelSize = gltf->GetIntegerField("size");
					}
				}

				SearchData.Add(OutItem);
			}
		}
		OnSearch().Execute(*this);
		SetState(SRS_SEARCH_DONE);
		return;
	}
}

void FSketchfabTask::GetThumbnail()
{
	MakeRequest(
		TaskData.ThumbnailURL,
		SRS_GETTHUMBNAIL_PROCESSING,
		&FSketchfabTask::GetThumbnail_Response,
		"image/jpeg"
	);
}

void FSketchfabTask::GetThumbnail_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (IsValid(Request, Response, "image/jpeg", false, true)) {

		const TArray<uint8> &data = Response->GetContent();
		if (data.Num() > 0)
		{
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

			FString jpg = TaskData.ThumbnailUID + ".jpg";
			FString FileName = TaskData.CacheFolder / jpg;
			IFileHandle* FileHandle = PlatformFile.OpenWrite(*FileName);
			if (FileHandle)
			{
				// Write the bytes to the file
				FileHandle->Write(data.GetData(), data.Num());

				// Close the file again
				delete FileHandle;
			}
		}

		if (!this->IsCompleted &&  OnThumbnailDownloaded().IsBound())
		{
			//UE_LOG(LogSketchfabRESTClient, Display, TEXT("Thumbnail downloaded"));
			OnThumbnailDownloaded().Execute(*this);
		}

		SetState(SRS_GETTHUMBNAIL_DONE);
	}
}

void FSketchfabTask::GetModelLink()
{

	FString url = "https://api.sketchfab.com/v3/";
	if (TaskData.OrgModel) {

		url += "orgs/" + TaskData.OrgUID + "/models/" + TaskData.ModelUID + "/download";
	}
	else {
		url += "models/" + TaskData.ModelUID + "/download";
	}

	UE_LOG(LogSketchfabTask, Display, TEXT("Requesting model link %s"), *url);

	MakeRequest(
		url,
		SRS_GETMODELLINK_PROCESSING,
		&FSketchfabTask::GetModelLink_Response,
		"application/json",
		nullptr
	);
}

void FSketchfabTask::GetModelLink_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (IsValid(Request, Response, "application/json")) {

		//Create a pointer to hold the json serialized data
		TSharedPtr<FJsonObject> JsonObject;

		//Create a reader pointer to read the json data
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		//Deserialize the json data given Reader and the actual object to deserialize
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			TSharedPtr<FJsonObject> gltfObject = JsonObject->GetObjectField("gltf");

			FString url = gltfObject->GetStringField("url");

			TaskData.ModelURL = url;

			if (!this->IsCompleted &&  OnModelLink().IsBound())
			{
				//UE_LOG(LogSketchfabRESTClient, Display, TEXT("Thumbnail downloaded"));
				OnModelLink().Execute(*this);
				SetState(SRS_GETTHUMBNAIL_DONE);
				return;
			}
		}
		SetState(SRS_FAILED);
	}
}


void FSketchfabTask::DownloadModel()
{
	MakeRequest(
		TaskData.ModelURL,
		SRS_DOWNLOADMODEL_PROCESSING,
		&FSketchfabTask::DownloadModel_Response,
		"",
		&FSketchfabTask::DownloadModelProgress,
		false,
		false
	);
}

void FSketchfabTask::DownloadModel_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (IsValid(Request, Response, "", false, true)) {

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

			SetState(SRS_FAILED);
			return;
		}

		const TArray<uint8> &data = Response->GetContent();
		if (data.Num() > 0)
		{
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

			FString fn = TaskData.ModelUID + ".zip";
			FString FileName = TaskData.CacheFolder / fn;
			IFileHandle* FileHandle = PlatformFile.OpenWrite(*FileName);
			if (FileHandle)
			{
				// Write the bytes to the file
				FileHandle->Write(data.GetData(), data.Num());

				// Close the file again
				delete FileHandle;

				if (!this->IsCompleted &&  OnModelDownloaded().IsBound())
				{
					//UE_LOG(LogSketchfabRESTClient, Display, TEXT("Thumbnail downloaded"));
					OnModelDownloaded().Execute(*this);
					SetState(SRS_DOWNLOADMODEL_DONE);
					return;
				}
			}
		}
		SetState(SRS_FAILED);
	}
}

void FSketchfabTask::DownloadModelProgress(FHttpRequestPtr HttpRequest, int32 BytesSent, int32 BytesReceived)
{
	if (!this->IsCompleted &&  OnModelDownloadProgress().IsBound())
	{
		TaskData.DownloadedBytes = BytesReceived;
		OnModelDownloadProgress().Execute(*this);
	}
}


void FSketchfabTask::GetUserOrgs()
{
	MakeRequest(
		"https://sketchfab.com/v3/me/orgs",
		SRS_GETUSERORGS_PROCESSING,
		&FSketchfabTask::GetUserOrgs_Response,
		""
	);
}

void FSketchfabTask::GetUserOrgs_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (IsValid(Request, Response)) {

		//Create a pointer to hold the json serialized data
		TSharedPtr<FJsonObject> JsonObject;

		//Create a reader pointer to read the json data
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		//Deserialize the json data given Reader and the actual object to deserialize
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			TArray<TSharedPtr<FJsonValue>> results = JsonObject->GetArrayField("results");
			TaskData.NextURL = JsonObject->GetStringField("next");

			if(results.Num()==0)
				UE_LOG(LogSketchfabRESTClient, Warning, TEXT("User is not a member of any org"));

			for (int r = 0; r < results.Num(); r++)
			{
				TSharedPtr<FJsonObject> resultObj = results[r]->AsObject();

				FSketchfabOrg org;
				org.name = resultObj->GetStringField("displayName");
				org.uid = resultObj->GetStringField("uid");
				org.url = resultObj->GetStringField("publicProfileUrl");

				UE_LOG(LogSketchfabRESTClient, Display, TEXT("User is member of an org %s (%s): %s"), *(org.name), *(org.uid), *(org.url));

				TaskData.Orgs.Add(org);
			}
		}

		if (!this->IsCompleted && OnUserOrgs().IsBound())
		{
			OnUserOrgs().Execute(*this);
		}

		SetState(SRS_GETUSERORGS_DONE);
	}
}

void FSketchfabTask::GetOrgsProjects()
{
	MakeRequest(
		"https://sketchfab.com/v3/orgs/" + TaskData.org->uid + "/projects",
		SRS_GETORGSPROJECTS_PROCESSING,
		&FSketchfabTask::GetOrgsProjects_Response,
		""
	);
}

void FSketchfabTask::GetOrgsProjects_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (IsValid(Request, Response)) {

		//Create a pointer to hold the json serialized data
		TSharedPtr<FJsonObject> JsonObject;

		//Create a reader pointer to read the json data
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		
		//Deserialize the json data given Reader and the actual object to deserialize
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			TArray<TSharedPtr<FJsonValue>> results = JsonObject->GetArrayField("results");
			TaskData.NextURL = JsonObject->GetStringField("next");

			if (results.Num() == 0)
				UE_LOG(LogSketchfabRESTClient, Display, TEXT("No projects available in the org"));

			for (int r = 0; r < results.Num(); r++)
			{
				TSharedPtr<FJsonObject> resultObj = results[r]->AsObject();

				FSketchfabProject project;
				project.name        = resultObj->GetStringField("name");
				project.uid         = resultObj->GetStringField("uid");
				project.slug        = resultObj->GetStringField("slug");
				project.modelCount  = resultObj->GetNumberField("modelCount");
				project.memberCount = resultObj->GetNumberField("memberCount");

				TaskData.Projects.Add(project);
			}
		}

		if (!this->IsCompleted && OnOrgsProjects().IsBound())
		{
			OnOrgsProjects().Execute(*this);
		}
		
		SetState(SRS_GETORGSPROJECTS_DONE);
	}
}

void FSketchfabTask::GetUserData()
{
	MakeRequest(
		"https://sketchfab.com/v3/me",
		SRS_GETUSERDATA_PROCESSING,
		&FSketchfabTask::GetUserData_Response,
		""
	);
}

void FSketchfabTask::GetUserData_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (IsValid(Request, Response)) {

		//Create a pointer to hold the json serialized data
		TSharedPtr<FJsonObject> JsonObject;

		//Create a reader pointer to read the json data
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		//Deserialize the json data given Reader and the actual object to deserialize
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			if (JsonObject->HasField("username"))
			{
				TaskData.UserSession.UserName = JsonObject->GetStringField("username");
			}

			if (JsonObject->HasField("displayName"))
			{
				TaskData.UserSession.UserDisplayName = JsonObject->GetStringField("displayName");
			}

			if (JsonObject->HasField("account"))
			{
				FString accountType = JsonObject->GetStringField("account");
				if(accountType.Compare("pro") == 0)
				{
					TaskData.UserSession.UserPlan = ACCOUNT_PRO;
				}
				else if(accountType.Compare("prem") == 0)
				{
					TaskData.UserSession.UserPlan = ACCOUNT_PREMIUM;
				}
				else if(accountType.Compare("biz") == 0)
				{
					TaskData.UserSession.UserPlan = ACCOUNT_BUSINESS;
				}
				else if(accountType.Compare("ent") == 0)
				{
					TaskData.UserSession.UserPlan = ACCOUNT_ENTERPRISE;
				}
				else if(accountType.Compare("plus") == 0)
				{
					TaskData.UserSession.UserPlan = ACCOUNT_PLUS;
				}
				else
				{
					TaskData.UserSession.UserPlan = ACCOUNT_BASIC;
				}
			}

			if (JsonObject->HasField("uid"))
			{
				TaskData.UserSession.UserUID = JsonObject->GetStringField("uid");
			}

			if (JsonObject->HasField("avatar"))
			{
				TSharedPtr<FJsonObject> AvatarObj = JsonObject->GetObjectField("avatar");
				if (AvatarObj->HasField("images"))
				{
					TArray<TSharedPtr<FJsonValue>> ImagesArray = AvatarObj->GetArrayField("images");
					if (ImagesArray.Num() > 0)
					{
						TSharedPtr<FJsonObject> ImageObj = ImagesArray[0]->AsObject();
						if (ImageObj->HasField("url"))
						{
							TaskData.UserSession.UserThumbnaillURL = ImageObj->GetStringField("url");
						}
						if (ImageObj->HasField("uid"))
						{
							TaskData.UserSession.UserThumbnaillUID = ImageObj->GetStringField("uid");
						}
					}
				}
			}
		}

		if (!this->IsCompleted &&  OnUserData().IsBound())
		{
			OnUserData().Execute(*this);
		}

		SetState(SRS_GETUSERDATA_DONE);
	}
}

void FSketchfabTask::GetUserThumbnail()
{
	MakeRequest(
		TaskData.UserSession.UserThumbnaillURL,
		SRS_GETUSERTHUMB_PROCESSING,
		&FSketchfabTask::GetUserThumbnail_Response,
		"image/jpeg"
	);
}

void FSketchfabTask::GetUserThumbnail_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (IsValid(Request, Response, "image/jpeg")) {

		const TArray<uint8> &data = Response->GetContent();
		if (data.Num() > 0)
		{
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

			FString uid = TaskData.UserSession.UserThumbnaillUID;
			if (uid.IsEmpty())
			{
				//The json file currently does not contain the uid of the thumbnail, so for now I will use the users uid
				uid = TaskData.UserSession.UserUID;
			}

			FString jpg = uid + ".jpg";
			FString FileName = TaskData.CacheFolder / jpg;
			IFileHandle* FileHandle = PlatformFile.OpenWrite(*FileName);
			if (FileHandle)
			{
				// Write the bytes to the file
				FileHandle->Write(data.GetData(), data.Num());

				// Close the file again
				delete FileHandle;
			}
		}

		if (!this->IsCompleted &&  OnUserThumbnail().IsBound())
		{
			//UE_LOG(LogSketchfabRESTClient, Display, TEXT("Thumbnail downloaded"));
			OnUserThumbnail().Execute(*this);
		}

		SetState(SRS_GETUSERTHUMB_DONE);
	}
}


void FSketchfabTask::GetCategories()
{
	MakeRequest(
		"https://api.sketchfab.com/v3/categories",
		SRS_GETCATEGORIES_PROCESSING,
		&FSketchfabTask::GetCategories_Response,
		""
	);
}

void FSketchfabTask::GetCategories_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (IsValid(Request, Response, "application/json", true)) {
		//Create a pointer to hold the json serialized data
		TSharedPtr<FJsonObject> JsonObject;

		//Create a reader pointer to read the json data
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		//Deserialize the json data given Reader and the actual object to deserialize
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			TArray<TSharedPtr<FJsonValue>> results = JsonObject->GetArrayField("results");
			TaskData.NextURL = JsonObject->GetStringField("next");

			for (int r = 0; r < results.Num(); r++)
			{
				TSharedPtr<FJsonObject> resultObj = results[r]->AsObject();

				FSketchfabCategory cat;

				cat.uid = resultObj->GetStringField("uid");
				cat.uri = resultObj->GetStringField("uri");
				cat.name = resultObj->GetStringField("name");
				cat.slug = resultObj->GetStringField("slug");

				TaskData.Categories.Add(cat);
			}
		}
		OnCategories().Execute(*this);
		SetState(SRS_GETCATEGORIES_DONE);
		return;
	}
}


void FSketchfabTask::GetModelInfo()
{

	FString url = "https://api.sketchfab.com/v3/";
	if (TaskData.OrgModel) {

		url += "orgs/" + TaskData.OrgUID + "/models/" + TaskData.ModelUID;
	}
	else {
		url += "models/" + TaskData.ModelUID;
	}

	UE_LOG(LogSketchfabTask, Display, TEXT("Getting model info for model %s"), *(TaskData.ModelUID));

	MakeRequest(
		url,
		SRS_GETMODELINFO_PROCESSING,
		&FSketchfabTask::GetModelInfo_Response,
		"image/jpeg"
	);
}

void FSketchfabTask::GetModelInfo_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (IsValid(Request, Response, "application/json", true)) {

		//Create a pointer to hold the json serialized data
		TSharedPtr<FJsonObject> JsonObject;

		//Create a reader pointer to read the json data
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

		//Deserialize the json data given Reader and the actual object to deserialize
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			TaskData.ModelVertexCount = JsonObject->GetIntegerField("vertexCount");
			TaskData.ModelFaceCount = JsonObject->GetIntegerField("faceCount");
			TaskData.AnimationCount = JsonObject->GetIntegerField("animationCount");
			//TaskData.ModelSize = JsonObject->GetObjectField("archives")->GetObjectField("gltf")->GetIntegerField("size");
			TaskData.ModelPublishedAt = GetPublishedAt(JsonObject);

			TSharedPtr<FJsonObject> archives = JsonObject->GetObjectField("archives");
			if (archives.IsValid())
			{
				TSharedPtr<FJsonObject> gltf = archives->GetObjectField("gltf");
				if (gltf.IsValid())
				{
					TaskData.ModelSize = gltf->GetIntegerField("size");
				}
			}

			TSharedPtr<FJsonObject> license = JsonObject->GetObjectField("license");
			if (license.IsValid())
			{
				TaskData.LicenceType = license->GetStringField("fullName");
				TaskData.LicenceInfo = license->GetStringField("requirements");
			}

			if (TaskData.LicenceType.IsEmpty()) // Personal models have no license data
			{
				TaskData.LicenceType = "Personal";
				TaskData.LicenceInfo = "You own this model";
			}

			GetThumnailData(JsonObject, 1024, TaskData.ThumbnailUID_1024, TaskData.ThumbnailURL_1024, TaskData.ThumbnailWidth_1024, TaskData.ThumbnailHeight_1024);
		}

		OnModelInfo().Execute(*this);
		SetState(SRS_GETMODELINFO_DONE);
	}
}

FDateTime FSketchfabTask::GetPublishedAt(TSharedPtr<FJsonObject> JsonObject)
{
	FDateTime ret;
	FString modelPublishedAt = JsonObject->GetStringField("publishedAt").Left(10);
	TArray<FString> publishedAt;
	modelPublishedAt.ParseIntoArray(publishedAt, TEXT("-"));

	if (publishedAt.Num() == 3)
	{
		ret = FDateTime(FCString::Atoi(*publishedAt[0]), FCString::Atoi(*publishedAt[1]), FCString::Atoi(*publishedAt[2]));
	}
	else
	{
		ret = FDateTime::Today();
	}
	return ret;
}



FString BoundaryBegin = "";
FString BoundaryLabel = "";
FString BoundaryEnd = "";
FString RequestData = "";
TArray<uint8> RequestBytes;

TArray<uint8> FStringToUint8(const FString& InString)
{
	TArray<uint8> OutBytes;

	// Handle empty strings
	if (InString.Len() > 0)
	{
		FTCHARToUTF8 Converted(*InString); // Convert to UTF8
		OutBytes.Append(reinterpret_cast<const uint8*>(Converted.Get()), Converted.Length());
	}

	return OutBytes;
}


void AddField(const FString& Name, const FString& Value)
{
	RequestData += FString(TEXT("\r\n"))
		+ BoundaryBegin
		+ FString(TEXT("Content-Disposition: form-data; name=\""))
		+ Name
		+ FString(TEXT("\"\r\n\r\n"))
		+ Value;
}
void AddFile(const FString& Name, const FString& FileName, const FString& FilePath) {

	TArray<uint8> UpFileRawData;
	FFileHelper::LoadFileToArray(UpFileRawData, *FilePath);

	RequestData += FString(TEXT("\r\n"))
		+ BoundaryBegin
		+ FString(TEXT("Content-Disposition: form-data; name=\""))
		+ "modelFile"
		+ FString(TEXT("\"; filename=\""))
		+ FileName
		+ FString(TEXT("\"\r\n\r\n"));

	RequestBytes = FStringToUint8(RequestData);
	RequestBytes.Append(UpFileRawData);

}

void FSketchfabTask::SetUploadRequestContent(ReqRef Request) {
	// Name/Value fields
	AddField(TEXT("name"), TaskData.ModelName);
	AddField(TEXT("description"), TaskData.ModelDescription);
	AddField(TEXT("tags"), TaskData.ModelTags);
	AddField(TEXT("password"), TaskData.ModelPassword);
	AddField(TEXT("isPublished"), TaskData.Draft ? TEXT("false") : TEXT("true"));
	AddField(TEXT("isPrivate"), TaskData.Private ? TEXT("true") : TEXT("false"));
	AddField(TEXT("source"), TEXT("unreal"));

	if (!TaskData.ProjectUID.IsEmpty()) {
		AddField(TEXT("orgProject"), TaskData.ProjectUID);
	}
	else {
		UE_LOG(LogSketchfabTask, Error, TEXT("No project selected (uid empty)"));
	}

	// File
	AddFile(TEXT("modelFile"), FPaths::GetCleanFilename(TaskData.FileToUpload), TaskData.FileToUpload);

	// Boundary end
	RequestBytes.Append(FStringToUint8(BoundaryEnd));

	// Set the content of the request
	Request->SetContent(RequestBytes);
}

//CombinedContent.Append(FStringToUint8(BoundaryEnd));

void FSketchfabTask::UploadModel()
{

	// Set the multipart boundary properties
	BoundaryLabel = FString(TEXT("e543322540af456f9a3773049ca02529-")) + FString::FromInt(FMath::Rand());
	BoundaryBegin = FString(TEXT("--")) + BoundaryLabel + FString(TEXT("\r\n"));
	BoundaryEnd = FString(TEXT("\r\n--")) + BoundaryLabel + FString(TEXT("--\r\n"));

	FString uploadUrl = "https://api.sketchfab.com/v3/";
	if ( TaskData.UsesOrgProfile ) {
		uploadUrl += "orgs/" + TaskData.OrgUID + "/models";
	}
	else {
		uploadUrl += "models";
	}

	//uploadUrl += "models";

	UE_LOG(LogSketchfabTask, Error, TEXT("Uploading to: %s"), *uploadUrl);

	MakeRequest(
		uploadUrl,
		SRS_UPLOADMODEL_PROCESSING,
		&FSketchfabTask::UploadModel_Response,
		"multipart/form-data; boundary=" + BoundaryLabel,
		nullptr,
		true
	);
}

void FSketchfabTask::UploadModel_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (IsValid(Request, Response)) {

		// Serialize the request response (uri and model uid)
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			TaskData.uid = JsonObject->GetStringField("uid");
			TaskData.uri = JsonObject->GetStringField("uri");
		}

		if (!this->IsCompleted &&  OnModelUploaded().IsBound())
		{
			OnModelUploaded().Execute(*this);
		}

		SetState(SRS_UPLOADMODEL_DONE);
	}
}