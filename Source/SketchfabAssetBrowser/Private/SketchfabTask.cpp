// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SketchfabTask.h"
#include "HAL/RunnableThread.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

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

	TArray<TSharedPtr<IHttpRequest>> OutPendingRequests;
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

void FSketchfabTask::AddAuthorization(TSharedRef<IHttpRequest> Request)
{
	if (!TaskData.Token.IsEmpty())
	{
		FString bearer = "Bearer ";
		bearer += TaskData.Token;
		Request->SetHeader("Authorization", bearer);
	}
}

/*Http call*/
void FSketchfabTask::Search()
{
	TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();

	FString url = TaskData.ModelSearchURL;

	AddAuthorization(request);

	request->SetURL(url);
	request->SetVerb(TEXT("GET"));
	request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	request->SetHeader("Content-Type", "application/json");
	request->OnProcessRequestComplete().BindRaw(this, &FSketchfabTask::Search_Response);

	UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("%s"), *request->GetURL());

	if (!request->ProcessRequest())
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Failed to process Request %s"), *request->GetURL());
	}
	else
	{
		DebugHttpRequestCounter.Increment();
		PendingRequests.Add(request, request->GetURL());
		SetState(SRS_SEARCH_PROCESSING);
	}
}

/*Assigned function on successfull http call*/
void FSketchfabTask::Search_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	DebugHttpRequestCounter.Decrement();

	FString OutUrl = PendingRequests.FindRef(Request);

	if (!OutUrl.IsEmpty())
	{
		PendingRequests.Remove(Request);
	}

	if (!Response.IsValid())
	{
		return;
	}

	if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		if (this == nullptr)
		{
			UE_LOG(LogSketchfabRESTClient, Display, TEXT("Object has already been destoryed or job id was empty"));
			SetState(SRS_FAILED);
			return;
		}

		if (Response->GetContentType() != "application/json")
		{
			UE_LOG(LogSketchfabRESTClient, Display, TEXT("GetModels_Response content type is not json"));
			SetState(SRS_FAILED);
			return;
		}

		FString data = Response->GetContentAsString();
		if (data.IsEmpty())
		{
			UE_LOG(LogSketchfabRESTClient, Display, TEXT("GetModels_Response content was empty"));
			SetState(SRS_FAILED);
		}
		else
		{
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

					TSharedPtr<FSketchfabTaskData> OutItem = MakeShareable(new FSketchfabTaskData());
					OutItem->ThumbnailUID = thumbUID;
					OutItem->ThumbnailURL = thumbURL;
					OutItem->ModelName = modelName;
					OutItem->ModelUID = modelUID;
					OutItem->CacheFolder = TaskData.CacheFolder;
					OutItem->Token = TaskData.Token;

					SearchData.Add(OutItem);
				}
			}
			OnSearch().Execute(*this);
			SetState(SRS_SEARCH_DONE);
			return;
		}
	}
	else
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());
	}
}

void FSketchfabTask::GetThumbnail()
{
	TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();

	AddAuthorization(request);

	request->SetURL(TaskData.ThumbnailURL);
	request->SetVerb(TEXT("GET"));
	request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	request->SetHeader("Content-Type", "image/jpeg");
	request->SetHeader("UID", TaskData.ThumbnailUID);
	request->OnProcessRequestComplete().BindRaw(this, &FSketchfabTask::GetThumbnail_Response);

	UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("%s"), *request->GetURL());

	if (!request->ProcessRequest())
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Failed to process Request %s"), *request->GetURL());
	}
	else
	{
		DebugHttpRequestCounter.Increment();
		PendingRequests.Add(request, request->GetURL());
		SetState(SRS_GETTHUMBNAIL_PROCESSING);
	}
}

void FSketchfabTask::GetThumbnail_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	DebugHttpRequestCounter.Decrement();

	FString OutUrl = PendingRequests.FindRef(Request);

	if (!OutUrl.IsEmpty())
	{
		PendingRequests.Remove(Request);
	}

	if (!Response.IsValid())
	{
		return;
	}

	if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		if (this == nullptr)
		{
			UE_LOG(LogSketchfabRESTClient, Display, TEXT("Object has already been destoryed or job id was empty"));
			SetState(SRS_FAILED);
			return;
		}

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
	else
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());
	}
}

void FSketchfabTask::GetModelLink()
{
	TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();

	AddAuthorization(request);

	FString url = "https://api.sketchfab.com/v3/models/";
	url += TaskData.ModelUID;
	url += "/download";

	request->SetURL(url);
	request->SetVerb(TEXT("GET"));
	request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	request->SetHeader("Content-Type", "image/jpeg");
	request->OnProcessRequestComplete().BindRaw(this, &FSketchfabTask::GetModelLink_Response);

	UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("%s"), *request->GetURL());

	if (!request->ProcessRequest())
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Failed to process Request %s"), *request->GetURL());
	}
	else
	{
		DebugHttpRequestCounter.Increment();
		PendingRequests.Add(request, request->GetURL());
		SetState(SRS_GETMODELLINK_PROCESSING);
	}
}

void FSketchfabTask::GetModelLink_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	DebugHttpRequestCounter.Decrement();

	FString OutUrl = PendingRequests.FindRef(Request);

	if (!OutUrl.IsEmpty())
	{
		PendingRequests.Remove(Request);
	}

	if (!Response.IsValid())
	{
		return;
	}

	if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		if (this == nullptr)
		{
			UE_LOG(LogSketchfabRESTClient, Display, TEXT("Object has already been destoryed or job id was empty"));
			SetState(SRS_FAILED);
			return;
		}

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
			TSharedPtr<FJsonObject> gltfObject = JsonObject->GetObjectField("gltf");

			FString url = gltfObject->GetStringField("url");
			int32 size = gltfObject->GetIntegerField("size");

			TaskData.ModelURL = url;
			TaskData.ModelSize = size;

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
	else
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());
	}
}


void FSketchfabTask::DownloadModel()
{
	TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();

	request->SetURL(TaskData.ModelURL);
	request->SetVerb(TEXT("GET"));
	request->OnProcessRequestComplete().BindRaw(this, &FSketchfabTask::DownloadModel_Response);
	request->OnRequestProgress().BindRaw(this, &FSketchfabTask::DownloadModelProgress);

	UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("%s"), *request->GetURL());

	if (!request->ProcessRequest())
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Failed to process Request %s"), *request->GetURL());
	}
	else
	{
		DebugHttpRequestCounter.Increment();
		PendingRequests.Add(request, request->GetURL());
		SetState(SRS_DOWNLOADMODEL_PROCESSING);
	}
}

void FSketchfabTask::DownloadModel_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	DebugHttpRequestCounter.Decrement();

	FString OutUrl = PendingRequests.FindRef(Request);

	if (!OutUrl.IsEmpty())
	{
		PendingRequests.Remove(Request);
	}

	if (!Response.IsValid())
	{
		return;
	}

	if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		if (this == nullptr)
		{
			UE_LOG(LogSketchfabRESTClient, Display, TEXT("Object has already been destoryed or job id was empty"));
			SetState(SRS_FAILED);
			return;
		}

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
	else
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());
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


void FSketchfabTask::GetUserData()
{
	TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();

	AddAuthorization(request);

	FString url = "https://sketchfab.com/v3/me";

	request->SetURL(url);
	request->SetVerb("GET");
	request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	request->OnProcessRequestComplete().BindRaw(this, &FSketchfabTask::GetUserData_Response);

	UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("%s"), *request->GetURL());

	if (!request->ProcessRequest())
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Failed to process Request %s"), *request->GetURL());
	}
	else
	{
		DebugHttpRequestCounter.Increment();
		PendingRequests.Add(request, request->GetURL());
		SetState(SRS_GETUSERDATA);
	}
}

void FSketchfabTask::GetUserData_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	DebugHttpRequestCounter.Decrement();

	FString OutUrl = PendingRequests.FindRef(Request);

	if (!OutUrl.IsEmpty())
	{
		PendingRequests.Remove(Request);
	}

	if (!Response.IsValid())
	{
		return;
	}

	if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		if (this == nullptr)
		{
			UE_LOG(LogSketchfabRESTClient, Display, TEXT("Object has already been destoryed or job id was empty"));
			SetState(SRS_FAILED);
			return;
		}

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
			TaskData.UserName = name;
		}

		if (!this->IsCompleted &&  OnUserData().IsBound())
		{
			OnUserData().Execute(*this);
		}

		SetState(SRS_GETUSERDATA_DONE);
	}
	else
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());
	}
}