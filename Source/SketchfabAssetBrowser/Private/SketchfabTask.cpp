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

FSketchfabTask::FSketchfabTask(const FSketchfabTaskData& InTaskData)
	:
	TaskData(InTaskData),
	State(SRS_UNKNOWN)
{
	bMultiPartUploadInitialized = false;
	TaskData.TaskUploadComplete = false;
	DebugHttpRequestCounter.Set(0);
	IsCompleted.AtomicSet(false);
	APIKey = TEXT("LOCAL");
	TaskData.JobName = TEXT("UE4_SKETCHFAB");
}

FSketchfabTask::~FSketchfabTask()
{
	UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, Warning, TEXT("Destroying Task With Job Id %s"), *JobId);

	TArray<TSharedPtr<IHttpRequest>> OutPendingRequests;
	if (PendingRequests.GetKeys(OutPendingRequests) > 0)
	{
		for (int32 ItemIndex = 0; ItemIndex < OutPendingRequests.Num(); ++ItemIndex)
		{
			UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, Warning, TEXT("Pending Request for task with status %d"), (int32)OutPendingRequests[ItemIndex]->GetStatus());
			OutPendingRequests[ItemIndex]->CancelRequest();
		}
	}
	//check(DebugHttpRequestCounter.GetValue() == 0);
	UploadParts.Empty();

	//unbind the delegates
	OnAssetDownloaded().Unbind();
	OnAssetUploaded().Unbind();
	OnTaskFailed().Unbind();

}

void FSketchfabTask::CreateUploadParts(const int32 MaxUploadPartSize)
{
	//create upload parts
	UploadParts.Empty();
	TArray<uint8> fileBlob;
	if (FFileHelper::LoadFileToArray(fileBlob, *TaskData.ZipFilePath))
	{
		UploadSize = fileBlob.Num();
		if (fileBlob.Num() > MaxUploadPartSize)
		{
			int32 NumberOfPartsRequried = fileBlob.Num() / MaxUploadPartSize;
			int32 RemainingBytes = fileBlob.Num() % MaxUploadPartSize;

			for (int32 PartIndex = 0; PartIndex < NumberOfPartsRequried; ++PartIndex)
			{
				FSketchfabUploadPart* UploadPartData = new (UploadParts) FSketchfabUploadPart();
				int32 Offset = PartIndex * MaxUploadPartSize * sizeof(uint8);
				UploadPartData->Data.AddUninitialized(MaxUploadPartSize);
				FMemory::Memcpy(UploadPartData->Data.GetData(), fileBlob.GetData() + Offset, MaxUploadPartSize * sizeof(uint8));
				UploadPartData->PartNumber = PartIndex + 1;
				UploadPartData->PartUploaded = false;
			}

			if (RemainingBytes > 0)
			{
				//NOTE: need to set Offset before doing a new on UploadParts
				int32 Offset = UploadParts.Num() * MaxUploadPartSize;
				FSketchfabUploadPart* UploadPartData = new (UploadParts) FSketchfabUploadPart();
				UploadPartData->Data.AddUninitialized(RemainingBytes);
				FMemory::Memcpy(UploadPartData->Data.GetData(), fileBlob.GetData() + Offset, RemainingBytes * sizeof(uint8));
				UploadPartData->PartNumber = NumberOfPartsRequried + 1;
				UploadPartData->PartUploaded = false;
			}
		}
		else
		{
			FSketchfabUploadPart* UploadPartData = new (UploadParts) FSketchfabUploadPart();
			UploadPartData->Data.AddUninitialized(fileBlob.Num());
			FMemory::Memcpy(UploadPartData->Data.GetData(), fileBlob.GetData(), fileBlob.Num() * sizeof(uint8));
			UploadPartData->PartNumber = 1;
			UploadPartData->PartUploaded = false;
		}

		TotalParts = UploadParts.Num();
		RemainingPartsToUpload.Add(TotalParts);
	}
}

bool FSketchfabTask::NeedsMultiPartUpload()
{
	return UploadParts.Num() > 1;
}

SketchfabRESTState FSketchfabTask::GetState() const
{
	FScopeLock Lock(TaskData.StateLock);
	return State;
}

void FSketchfabTask::SetHost(FString InHostAddress)
{
	HostName = InHostAddress;
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
		if (InState != State
			&& (State != SRS_ASSETDOWNLOADED && State != SRS_FAILED))
		{
			State = InState;
		}
		else if ((InState != State) && (State != SRS_FAILED) && (InState == SRS_ASSETDOWNLOADED))
		{
			State = SRS_ASSETDOWNLOADED;
			this->IsCompleted.AtomicSet(true);
		}
		else if (InState != State && InState == SRS_FAILED)
		{
			State = SRS_ASSETDOWNLOADED;
		}

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

bool FSketchfabTask::ParseJsonMessage(FString InJsonMessage, FSketchfabJsonResponse& OutResponseData)
{
	bool sucess = false;

	TSharedPtr<FJsonObject> JsonParsed;
	TSharedRef<TJsonReader<TCHAR>> JsonReader = TJsonReaderFactory<TCHAR>::Create(InJsonMessage);
	if (FJsonSerializer::Deserialize(JsonReader, JsonParsed))
	{
		FString Status;
		if (JsonParsed->HasField("JobId"))
		{
			JsonParsed->TryGetStringField("JobId", OutResponseData.JobId);
		}

		if (JsonParsed->HasField("Status"))
		{
			OutResponseData.Status = JsonParsed->GetStringField("Status");
		}

		if (JsonParsed->HasField("OutputAssetId"))
		{
			JsonParsed->TryGetStringField("OutputAssetId", OutResponseData.OutputAssetId);
		}

		if (JsonParsed->HasField("AssetId"))
		{
			JsonParsed->TryGetStringField("AssetId", OutResponseData.AssetId);
		}

		if (JsonParsed->HasField("ProgressPercentage"))
		{
			JsonParsed->TryGetNumberField("ProgressPercentage", OutResponseData.Progress);
		}

		if (JsonParsed->HasField("ProgressPercentage"))
		{
			JsonParsed->TryGetNumberField("ProgressPercentage", OutResponseData.Progress);
		}

		if (JsonParsed->HasField("UploadId"))
		{
			JsonParsed->TryGetStringField("UploadId", OutResponseData.UploadId);
		}

		sucess = true;
	}


	return sucess;
}


// ~ HTTP Request method to communicate with Sketchfab REST Interface


void FSketchfabTask::AccountInfo()
{
	TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();

	AddAuthenticationHeader(request);
	request->SetURL(FString::Printf(TEXT("%s/2.3/account?apikey=%s"), *HostName, *APIKey));
	request->SetVerb(TEXT("GET"));

	UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("%s"), *request->GetURL());

	request->OnProcessRequestComplete().BindRaw(this, &FSketchfabTask::AccountInfo_Response);

	if (!request->ProcessRequest())
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Failed to process Request %s"), *request->GetURL());
	}
	else
	{
		DebugHttpRequestCounter.Increment();
		PendingRequests.Add(request, request->GetURL());
	}
}

void FSketchfabTask::AccountInfo_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	DebugHttpRequestCounter.Decrement();
	FString OutUrl = PendingRequests.FindRef(Request);

	if (!OutUrl.IsEmpty())
	{
		PendingRequests.Remove(Request);
	}

	if (!Response.IsValid())
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response Invalid %s"), *Request->GetURL());
		return;
	}

	if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		FString msg = Response->GetContentAsString();
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response Message %s"), *msg);
	}
	else
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());
	}
}


void FSketchfabTask::CreateJob()
{
	TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();

	FString url = FString::Printf(TEXT("%s/2.3/job/create?apikey=%s&job_name=%s&asset_id=%s"), *HostName, *APIKey, *TaskData.JobName, *InputAssetId);
	AddAuthenticationHeader(request);
	request->SetURL(url);
	request->SetVerb(TEXT("POST"));

	request->OnProcessRequestComplete().BindRaw(this, &FSketchfabTask::CreateJob_Response);

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
		SetState(SRS_JOBCREATED_PENDING);
	}
}

void FSketchfabTask::CreateJob_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	DebugHttpRequestCounter.Decrement();

	FString OutUrl = PendingRequests.FindRef(Request);

	if (!OutUrl.IsEmpty())
	{
		PendingRequests.Remove(Request);
	}

	if (!Response.IsValid())
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response Invalid %s"), *Request->GetURL());
		return;
	}

	if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		FString msg = Response->GetContentAsString();

		FSketchfabJsonResponse Data;
		if (ParseJsonMessage(msg, Data))
		{
			if (!Data.JobId.IsEmpty())
			{
				JobId = Data.JobId;
				UE_LOG(LogSketchfabRESTClient, Display, TEXT("Created JobId: %s"), *Data.JobId);
				SetState(SRS_JOBCREATED);
			}
			else
				UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, Warning, TEXT("Empty JobId for task"));

		}
		else
		{
			SetState(SRS_FAILED);
			UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Failed to parse message %s for Request %s"), *msg, *Request->GetURL());
		}


	}
	else
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());
	}
}

void FSketchfabTask::UploadJobSettings()
{
	TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();

	FString url = FString::Printf(TEXT("%s/2.3/job/%s/uploadsettings?apikey=%s"), *HostName, *JobId, *APIKey);

	TArray<uint8> data;
	FFileHelper::LoadFileToArray(data, *TaskData.SplFilePath);

	AddAuthenticationHeader(request);
	request->SetHeader(TEXT("Content-Type"), TEXT("application/octet-stream"));
	request->SetURL(url);
	request->SetVerb(TEXT("POST"));
	request->SetContent(data);

	UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("%s"), *request->GetURL());

	request->OnProcessRequestComplete().BindRaw(this, &FSketchfabTask::UploadJobSettings_Response);

	if (!request->ProcessRequest())
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Failed to process Request %s"), *request->GetURL());
	}
	else
	{
		DebugHttpRequestCounter.Increment();
		PendingRequests.Add(request, request->GetURL());
		SetState(SRS_JOBSETTINGSUPLOADED_PENDING);
	}
}

void FSketchfabTask::UploadJobSettings_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	DebugHttpRequestCounter.Decrement();

	FString OutUrl = PendingRequests.FindRef(Request);

	if (!OutUrl.IsEmpty())
	{
		PendingRequests.Remove(Request);
	}

	if (!Response.IsValid())
	{
		SetState(SRS_FAILED);
		return;
	}

	if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{

		if (!OnAssetUploaded().IsBound())
		{
			UE_LOG(LogSketchfabRESTClient, Error, TEXT("OnAssetUploaded delegate not bound to any object"));
		}
		else
		{
			OnAssetUploaded().Execute(*this);
			SetState(SRS_JOBSETTINGSUPLOADED);
		}
	}
	else
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());
	}
}

void FSketchfabTask::ProcessJob()
{
	SetState(SRS_JOBPROCESSING_PENDING);

	TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();

	FString url = FString::Printf(TEXT("%s/2.3/job/%s/Process?apikey=%s"), *HostName, *JobId, *APIKey);
	AddAuthenticationHeader(request);
	request->SetURL(url);
	request->SetVerb(TEXT("PUT"));

	UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("%s"), *request->GetURL());

	request->OnProcessRequestComplete().BindRaw(this, &FSketchfabTask::ProcessJob_Response);

	if (!request->ProcessRequest())
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Failed to process Request %s Response code %d"), *request->GetURL());
	}
	else
	{
		DebugHttpRequestCounter.Increment();
		PendingRequests.Add(request, request->GetURL());
	}
}

void FSketchfabTask::ProcessJob_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	DebugHttpRequestCounter.Decrement();

	FString OutUrl = PendingRequests.FindRef(Request);

	if (!OutUrl.IsEmpty())
	{
		PendingRequests.Remove(Request);
	}

	if (!Response.IsValid())
	{
		SetState(SRS_FAILED);
		return;
	}

	if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		SetState(SRS_JOBPROCESSING);

	}
	else
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());
	}
}

void FSketchfabTask::GetJob()
{
	TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();

	FString url = FString::Printf(TEXT("%s/2.3/job/%s?apikey=%s"), *HostName, *JobId, *APIKey);
	AddAuthenticationHeader(request);
	request->SetURL(url);
	request->SetVerb(TEXT("GET"));
	UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("%s"), *request->GetURL());
	request->OnProcessRequestComplete().BindRaw(this, &FSketchfabTask::GetJob_Response);

	if (!request->ProcessRequest())
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Failed to process Request %s"), *request->GetURL());
	}
	else
	{
		DebugHttpRequestCounter.Increment();
		PendingRequests.Add(request, request->GetURL());
	}
}

void FSketchfabTask::GetJob_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	DebugHttpRequestCounter.Decrement();

	FString OutUrl = PendingRequests.FindRef(Request);

	if (!OutUrl.IsEmpty())
	{
		PendingRequests.Remove(Request);
	}

	if (!Response.IsValid())
	{
		SetState(SRS_FAILED);
		return;
	}
	else if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
	{
		FString msg = Response->GetContentAsString();

		FSketchfabJsonResponse Data;
		if (!msg.IsEmpty() && ParseJsonMessage(msg, Data))
		{

			if (!Data.Status.IsEmpty())
			{
				if (Data.Status == "Processed")
				{
					if (!Data.OutputAssetId.IsEmpty())
					{
						OutputAssetId = Data.OutputAssetId;
						SetState(SRS_JOBPROCESSED);
						//UE_LOG(LogSketchfabRESTClient, Log, TEXT("Job with Id %s processed"), *Data.JobId);

					}
					else
					{
						SetState(SRS_FAILED);
					}
				}
				if (Data.Status == "Failed")
				{
					SetState(SRS_FAILED);
					UE_LOG(LogSketchfabRESTClient, Error, TEXT("Job with id %s Failed %s"), *Data.JobId);
				}
			}
		}
		else
		{
			//SetState(SRS_FAILED);
		}
	}
	else
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());
	}
}

void FSketchfabTask::UploadAsset()
{
	//if multipart upload is need then use multi part request else use older request to upload data
	if (NeedsMultiPartUpload())
	{
		int32 PartsToUpload = RemainingPartsToUpload.GetValue();
		int32 PartIndex = TotalParts - PartsToUpload;
		if (!bMultiPartUploadInitialized && PartsToUpload > 0)
		{
			MultiPartUploadBegin();
		}
		else if (PartsToUpload > 0)
		{
			MultiPartUploadPart(UploadParts[PartIndex].PartNumber);
		}
		else if (PartsToUpload == 0)
		{
			if (!TaskData.TaskUploadComplete)
				MultiPartUploadEnd();
			else
			{
				//FPlatformProcess::Sleep(0.1f);
				MultiPartUploadGet();
			}

		}
	}
	else
	{
		//bail if part has already been uploaded
		if (UploadParts[0].PartUploaded)
		{
			UE_LOG(LogSketchfabRESTClient, Display, TEXT("Skip Already Uploaded Asset."));
			return;
		}

		TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();

		FString url = FString::Printf(TEXT("%s/2.3/asset/upload?apikey=%s&asset_name=%s"), *HostName, *APIKey, *TaskData.JobName);

		AddAuthenticationHeader(request);
		request->SetHeader(TEXT("Content-Type"), TEXT("application/octet-stream"));
		request->SetURL(url);
		request->SetVerb(TEXT("POST"));
		request->SetContent(UploadParts[0].Data);

		request->OnProcessRequestComplete().BindRaw(this, &FSketchfabTask::UploadAsset_Response);


		uint32 bufferSize = UploadParts[0].Data.Num();

		FHttpModule::Get().SetMaxReadBufferSize(bufferSize);
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
			SetState(SRS_ASSETUPLOADED_PENDING);
		}
	}


}

void FSketchfabTask::UploadAsset_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	DebugHttpRequestCounter.Decrement();

	FString OutUrl = PendingRequests.FindRef(Request);

	if (!OutUrl.IsEmpty())
	{
		PendingRequests.Remove(Request);
	}

	if (!bWasSuccessful)
	{
		SetState(SRS_FAILED);
		if (Response.IsValid())
		{
			UE_LOG(LogSketchfabRESTClient, Warning, TEXT("Upload asset Response: %i"), Response->GetResponseCode());
		}
		else
			UE_LOG(LogSketchfabRESTClient, Error, TEXT("Upload asset response failed."));
	}
	else
	{
		if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
		{
			FString msg = Response->GetContentAsString();

			FSketchfabJsonResponse Data;
			if (ParseJsonMessage(msg, Data))
			{
				if (!Data.AssetId.IsEmpty())
				{
					InputAssetId = Data.AssetId;
					UploadParts[0].PartUploaded = true;
					SetState(SRS_ASSETUPLOADED);
				}
				else
					UE_LOG(LogSketchfabRESTClient, Display, TEXT("Could not parse Input asset Id for job: %s"), *Data.JobId);
			}
			else
			{
				SetState(SRS_FAILED);
			}
		}
		else
		{
			UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());
		}

	}

}

void FSketchfabTask::DownloadAsset()
{
	TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();

	if (OutputAssetId.IsEmpty())
	{
		SetState(SRS_FAILED);
	}

	FString url = FString::Printf(TEXT("%s/2.3/asset/%s/download?apikey=%s"), *HostName, *OutputAssetId, *APIKey);
	AddAuthenticationHeader(request);
	request->SetURL(url);
	request->SetVerb(TEXT("GET"));
	FHttpModule::Get().

		FHttpModule::Get().SetHttpTimeout(300);
	UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("%s"), *request->GetURL());

	request->OnProcessRequestComplete().BindRaw(this, &FSketchfabTask::DownloadAsset_Response);

	if (!request->ProcessRequest())
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Failed to process Request %s"), *request->GetURL());
	}
	else
	{
		DebugHttpRequestCounter.Increment();
		PendingRequests.Add(request, request->GetURL());

		UE_LOG(LogSketchfabRESTClient, Log, TEXT("Downloading Job with Id %s"), *JobId);
		SetState(SRS_ASSETDOWNLOADED_PENDING);
	}
}

void FSketchfabTask::DownloadAsset_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
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
		if (this == nullptr || JobId.IsEmpty())
		{
			UE_LOG(LogSketchfabRESTClient, Display, TEXT("Object has already been destoryed or job id was empty"));
			return;
		}
		TArray<uint8> data = Response->GetContent();
		if (data.Num() > 0)
		{
			if (!TaskData.OutputZipFilePath.IsEmpty() && !FFileHelper::SaveArrayToFile(data, *TaskData.OutputZipFilePath))
			{
				UE_LOG(LogSketchfabRESTClient, Display, TEXT("Unable to save file %s"), *TaskData.OutputZipFilePath);
				SetState(SRS_FAILED);

			}
			else
			{
				if (!this->IsCompleted &&  OnAssetDownloaded().IsBound())
				{

					UE_LOG(LogSketchfabRESTClient, Display, TEXT("Asset downloaded"));
					OnAssetDownloaded().Execute(*this);
					SetState(SRS_ASSETDOWNLOADED);
				}
				else
					UE_LOG(LogSketchfabRESTClient, Display, TEXT("OnAssetDownloaded delegate not bound to any objects"));
			}
		}
	}
	else
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());
	}
}

void FSketchfabTask::MultiPartUploadBegin()
{
	TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();

	FString url = FString::Printf(TEXT("%s/2.3/asset/uploadpart?apikey=%s&asset_name=%s"), *HostName, *APIKey, *TaskData.JobName);


	FArrayWriter Writer;
	AddAuthenticationHeader(request);
	request->SetURL(url);
	request->SetVerb(TEXT("POST"));
	request->OnProcessRequestComplete().BindRaw(this, &FSketchfabTask::MultiPartUploadBegin_Response);
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

		SetState(SRS_ASSETUPLOADED_PENDING);
	}
}

void FSketchfabTask::MultiPartUploadBegin_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	DebugHttpRequestCounter.Decrement();

	FString OutUrl = PendingRequests.FindRef(Request);

	if (!OutUrl.IsEmpty())
	{
		PendingRequests.Remove(Request);
	}

	if (!bWasSuccessful)
	{
		SetState(SRS_FAILED);
		if (Response.IsValid())
		{
			UE_LOG(LogSketchfabRESTClient, Warning, TEXT("Upload asset Response: %i"), Response->GetResponseCode());
		}
		else
			UE_LOG(LogSketchfabRESTClient, Error, TEXT("Upload asset response failed."));
	}
	else
	{
		if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
		{
			FString msg = Response->GetContentAsString();
			FSketchfabJsonResponse Data;

			if (ParseJsonMessage(msg, Data))
			{
				if (!Data.UploadId.IsEmpty())
				{
					UploadId = Data.UploadId;
					bMultiPartUploadInitialized = true;
				}
			}
			else
			{

				SetState(SRS_FAILED);

			}
		}
		else
			UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());

	}
}

void FSketchfabTask::MultiPartUploadPart(const uint32 InPartNumber)
{

	//should bailout if part has already been uploaded
	int32 PartIndex = InPartNumber - 1;
	if (UploadParts[PartIndex].PartUploaded)
		return;

	TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();

	FString url = FString::Printf(TEXT("%s/2.3/asset/uploadpart/%s/upload?apikey=%s&part_number=%d"), *HostName, *UploadId, *APIKey, UploadParts[PartIndex].PartNumber);

	FArrayWriter Writer;
	AddAuthenticationHeader(request);
	request->SetHeader(TEXT("Content-Type"), TEXT("application/octet-stream"));
	request->SetURL(url);
	request->SetVerb(TEXT("PUT"));
	request->SetContent(UploadParts[PartIndex].Data);
	FHttpModule::Get().SetMaxReadBufferSize(UploadParts[PartIndex].Data.Num());

	request->OnProcessRequestComplete().BindRaw(this, &FSketchfabTask::MultiPartUploadPart_Response);

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
	}

}

void FSketchfabTask::MultiPartUploadPart_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{

	DebugHttpRequestCounter.Decrement();
	FString OutUrl = PendingRequests.FindRef(Request);

	if (!OutUrl.IsEmpty())
	{
		PendingRequests.Remove(Request);
	}

	if (!bWasSuccessful)
	{
		SetState(SRS_FAILED);
		if (Response.IsValid())
		{
			UE_LOG(LogSketchfabRESTClient, Warning, TEXT("Upload asset Response: %i"), Response->GetResponseCode());
		}
		else
			UE_LOG(LogSketchfabRESTClient, Error, TEXT("Upload asset response failed."));
	}
	else
	{
		if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
		{
			FString msg = Response->GetContentAsString();

			//get part_number from query string
			FString PartNumStr = Request->GetURLParameter("part_number");
			if (!PartNumStr.IsEmpty())
			{
				int32 PartNum = FCString::Atoi(*PartNumStr);
				PartNum -= 1;
				if (!UploadParts[PartNum].PartUploaded)
				{
					RemainingPartsToUpload.Decrement();
					UploadParts[PartNum].PartUploaded = true;
				}
			}
		}
		else
		{
			UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());
		}

	}
}

void FSketchfabTask::MultiPartUploadEnd()
{
	TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();

	FString url = FString::Printf(TEXT("%s/2.3/asset/uploadpart/%s/Complete?apikey=%s&part_count=%d&upload_size=%d"), *HostName, *UploadId, *APIKey, TotalParts, UploadSize);

	FArrayWriter Writer;
	AddAuthenticationHeader(request);
	request->SetURL(url);
	request->SetVerb(TEXT("POST"));
	request->OnProcessRequestComplete().BindRaw(this, &FSketchfabTask::MultiPartUploadEnd_Response);

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
	}
}

void FSketchfabTask::MultiPartUploadEnd_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	DebugHttpRequestCounter.Decrement();

	FString OutUrl = PendingRequests.FindRef(Request);

	if (!OutUrl.IsEmpty())
	{
		PendingRequests.Remove(Request);
	}

	if (!bWasSuccessful)
	{
		SetState(SRS_FAILED);
		if (Response.IsValid())
		{
			UE_LOG(LogSketchfabRESTClient, Warning, TEXT("Upload asset Response: %i"), Response->GetResponseCode());
		}
		else
			UE_LOG(LogSketchfabRESTClient, Error, TEXT("Upload asset response failed."));
	}
	else
	{
		if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
		{
			FString msg = Response->GetContentAsString();

			FSketchfabJsonResponse Data;
			if (ParseJsonMessage(msg, Data))
			{
				if (!Data.UploadId.IsEmpty())
				{
					this->TaskData.TaskUploadComplete = true;
				}

			}
			else
			{
				SetState(SRS_FAILED);
			}
		}
		else
			UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());

	}
}

void FSketchfabTask::MultiPartUploadGet()
{
	TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();

	FString url = FString::Printf(TEXT("%s/2.3/asset/uploadpart/%s?apikey=%s"), *HostName, *UploadId, *APIKey);

	FArrayWriter Writer;
	AddAuthenticationHeader(request);
	request->SetURL(url);
	request->SetVerb(TEXT("GET"));
	request->OnProcessRequestComplete().BindRaw(this, &FSketchfabTask::MultiPartUploadGet_Response);

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
	}
}

void FSketchfabTask::MultiPartUploadGet_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	DebugHttpRequestCounter.Decrement();
	FString OutUrl = PendingRequests.FindRef(Request);

	if (!OutUrl.IsEmpty())
	{
		PendingRequests.Remove(Request);
	}

	if (!bWasSuccessful)
	{
		SetState(SRS_FAILED);
		if (Response.IsValid())
		{
			UE_LOG(LogSketchfabRESTClient, Warning, TEXT("%s: %i"), __FUNCTION__, Response->GetResponseCode());
		}
		else
			UE_LOG(LogSketchfabRESTClient, Error, TEXT("Upload asset response failed."));
	}
	else
	{
		if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
		{
			FString msg = Response->GetContentAsString();

			FSketchfabJsonResponse Data;

			if (ParseJsonMessage(msg, Data))
			{
				if (!Data.AssetId.IsEmpty())
				{
					InputAssetId = Data.AssetId;
					SetState(SRS_ASSETUPLOADED);
					UploadParts.Empty();
				}
			}
		}
		else
			UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());
	}
}

void FSketchfabTask::AddAuthenticationHeader(TSharedRef<IHttpRequest> request)
{
	//Basic User:User
	request->SetHeader("Authorization", "Basic dXNlcjp1c2Vy");
}

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
void FSketchfabTask::GetModels()
{
	TSharedRef<IHttpRequest> request = FHttpModule::Get().CreateRequest();

	FString url = TaskData.ModelSearchURL;

	AddAuthorization(request);

	request->SetURL(url);
	request->SetVerb(TEXT("GET"));
	request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	request->SetHeader("Content-Type", "application/json");
	request->OnProcessRequestComplete().BindRaw(this, &FSketchfabTask::GetModels_Response);

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
		SetState(SRS_GETMODELS_PROCESSING);
	}
}

/*Assigned function on successfull http call*/
void FSketchfabTask::GetModels_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
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

					FString jpg = thumbUID + ".jpg";
					FString FileName = TaskData.CacheFolder / jpg;

					IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
					if (!PlatformFile.FileExists(*FileName))
					{
						TaskData.ThumbnailUID = thumbUID;
						TaskData.ThumbnailURL = thumbURL;

						if (!this->IsCompleted &&  OnThumbnailRequired().IsBound())
						{
							//UE_LOG(LogSketchfabRESTClient, Display, TEXT("Model downloaded"));
							OnThumbnailRequired().Execute(*this);
						}
					}
				}
			}
			OnModelList().Execute(*this);
			SetState(SRS_GETMODELS_DONE);
		}
	}
	else
	{
		SetState(SRS_FAILED);
		UE_CLOG(bEnableDebugLogging, LogSketchfabRESTClient, VeryVerbose, TEXT("Response failed %s Code %d"), *Request->GetURL(), Response->GetResponseCode());
	}
}
