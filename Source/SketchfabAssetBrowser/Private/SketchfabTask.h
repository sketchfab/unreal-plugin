// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"
#include "HAL/FileManager.h"
#include "Misc/MessageDialog.h"
#include "Misc/SlowTask.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopeLock.h"
#include "Containers/Queue.h"

#include "Runtime/Online/HTTP/Public/Interfaces/IHttpResponse.h"
#include "Runtime/Online/HTTP/Public/Interfaces/IHttpRequest.h"
#include "Runtime/Online/HTTP/Public/HttpModule.h"

/** Enum representing state used by Sketchfab Server */
enum SketchfabRESTState
{
	SRS_UNKNOWN,
	SRS_FAILED,
	SRS_ASSETUPLOADED_PENDING,
	SRS_ASSETUPLOADED,
	SRS_JOBCREATED_PENDING,
	SRS_JOBCREATED,
	SRS_JOBSETTINGSUPLOADED_PENDING,
	SRS_JOBSETTINGSUPLOADED,
	SRS_JOBPROCESSING_PENDING,
	SRS_JOBPROCESSING,
	SRS_JOBPROCESSED,
	SRS_ASSETDOWNLOADED_PENDING,
	SRS_ASSETDOWNLOADED,
	SRS_GETMODELS,
	SRS_GETMODELS_PROCESSING,
	SRS_GETMODELS_DONE
};

/** Enum representing state used internally by REST Client to manage multi-part asset uploading to Sketchfab */
enum EUploadPartState
{
	UPS_BEGIN,			// start uploading (hand shake)
	UPS_UPLOADING_PART,	// upload part
	UPS_END				// upload transaction completed
};

/**
Intermediate struct to hold upload file chunks for multi-part upload
*/
struct FSketchfabUploadPart
{
	/** Upload part binary chunck */
	TArray<uint8> Data;

	/** Part number */
	int32 PartNumber;

	/** Bool if part has been uploaded */
	FThreadSafeBool PartUploaded;

	~FSketchfabUploadPart()
	{
		Data.Empty();
	}

};

/** Struct holding essential task data for task management. */
struct FSketchfabTaskData
{
	/** Download URL path to get a list of assets and download thumbnails */
	FString ModelSearchURL;

	/** Path to the zip file that needs to be uploaded */
	FString ZipFilePath;

	/** Path to spl file that needs to be uploaded */
	FString SplFilePath;

	/** Path to zip file containing resulting geometry */
	FString OutputZipFilePath;

	/** Swarm Job Directory */
	FString JobDirectory;

	/** Swarm Job Name - Can be used to track jobs using the Admin Utility*/
	FString JobName;

	/** Lock for synchornization between threads */
	FCriticalSection* StateLock;

	/** Unique Job Id */
	FGuid ProcessorJobID;

	/** Set if the task upload has been completed */
	FThreadSafeBool TaskUploadComplete;

	/** Supports Dithered Transition */
	bool bDitheredTransition;

    // Whether or not emissive should be outputted  
	bool bEmissive;

	//========================================================================

	/** Authentication Token */
	FString Token;

	/** Model Unique Identifier */
	FString ModelUID;

	/** Thumbnail Unique Identifier */
	FString ThumbnailUID;

	/** Thumbnail Download URL */
	FString ThumbnailURL;

	/** The Cache folder for downloaded content */
	FString CacheFolder;
};

/** Struct that hold intermediate data used to communicate next state to Sketchfab Server */
struct FSketchfabJsonResponse
{
	/** Unique JobId */
	FString JobId;

	/** Unique asset id returned from server.
	Note : This can be used to check if asset already is available on the server to save network bandwidth
	*/
	FString AssetId;

	/** Supports Dithered Transition */
	FString ErrorMessage;

	/** Supports Dithered Transition */
	uint32 Progress;

	/** Supports Dithered Transition */
	FString Status;

	/** Supports Dithered Transition */
	FString OutputAssetId;

	/** Supports Dithered Transition */
	FString UploadId;
};

/** Sketchfab Swarm Task. Responsible for communicating with the Server  */
class FSketchfabTask
{

public:
	DECLARE_DELEGATE_OneParam(FSketchfabTaskDelegate, const FSketchfabTask&);

	FSketchfabTask(const FSketchfabTaskData& InTaskData);
	~FSketchfabTask();

	/* Method to get/set Task Sate */
	SketchfabRESTState GetState() const;
	void SetState(SketchfabRESTState InState);

	bool IsFinished() const;

	/*~ Events */
	FSketchfabTaskDelegate& OnAssetDownloaded() { return OnAssetDownloadedDelegate; }
	FSketchfabTaskDelegate& OnAssetUploaded() { return OnAssetUploadedDelegate; }
	FSketchfabTaskDelegate& OnTaskFailed() { return OnTaskFailedDelegate; }

	FSketchfabTaskDelegate& OnModelList() { return OnModelListDelegate; }
	FSketchfabTaskDelegate& OnThumbnailRequired() { return OnThumbnailRequiredDelegate; }

	void EnableDebugLogging();	

	/**
	Method to setup the server the task should use.
	Currently only Single host is supported.
	*/
	void SetHost(FString InHostAddress);

	/*
	The following method is used to split the asset into multiple parts for a multi part upload.
	*/
	void CreateUploadParts(const int32 MaxUploadFileSize);

	/*
	Tells if the current task needs to be uploaded as separate part due to data size limits
	*/
	bool NeedsMultiPartUpload();

	//~Being Rest methods
	void AccountInfo();
	void CreateJob();
	void UploadJobSettings();
	void ProcessJob();
	void GetJob();
	void UploadAsset();
	void DownloadAsset();

	void GetModels();

	/*
	Note for multi part upload the following need to happen
	Call upload being to setup the multi part upload
	upload parts using MultiPartUplaodPart
	Call MultiPartupload end
	*/
	void MultiPartUploadBegin();
	void MultiPartUploadPart(const uint32 partNo);
	void MultiPartUploadEnd();
	void MultiPartUploadGet();

	//~Being Response methods
	void AccountInfo_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void CreateJob_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void UploadJobSettings_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void ProcessJob_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void GetJob_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void UploadAsset_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void DownloadAsset_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void AddAuthenticationHeader(TSharedRef<IHttpRequest> request);

	//
	void AddAuthorization(TSharedRef<IHttpRequest> Request);
	void GetModels_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	//Multi part upload responses
	void MultiPartUploadBegin_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void MultiPartUploadPart_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void MultiPartUploadEnd_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void MultiPartUploadGet_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	//~End Rest methods

	/** Helper method use to deserialze json respone from Sketchfab and popupate the FSwarmJsonRepose Struct*/
	bool ParseJsonMessage(FString InJsonMessage, FSketchfabJsonResponse& OutResponseData);

	/** Essential Task Data */
	FSketchfabTaskData TaskData;

private:
	/** Task State */
	SketchfabRESTState State;

	/** Job Id */
	FString JobId;

	/** Asset Id returned from the server */
	FString InputAssetId;

	/** Output Asset id */
	FString OutputAssetId;

	/** Is Completed */
	FThreadSafeBool IsCompleted;

	/** Enable Debug Logging */
	bool bEnableDebugLogging;

	/** Parts left to upload (Multi-part Uploading) */
	FThreadSafeCounter RemainingPartsToUpload;

	/** Debug HttpRequestCounter
	Note: This was added to track issues when two resposnes came for a completed job.
	Since the job was completed before the object is partially destoreyd when a new response came in.
	The import file method failed. This was added for debugging. The most likely cause if that the respose delegate is never cleaned up.
	(This must be zero else, Sometime two responses for job completed arrive which caused issue) */
	FThreadSafeCounter DebugHttpRequestCounter;

	/** Multipart upload has been initalized*/
	bool bMultiPartUploadInitialized;

	/** Multi part upload data*/
	TIndirectArray<FSketchfabUploadPart> UploadParts;

	/** Total number of parts to upload*/
	uint32 TotalParts;

	/** Sketchfab Server IP address */
	FString HostName;

	/** API Key used to communicate with the Server */
	FString APIKey;

	/** Upload Id used for multipart upload */
	FString UploadId;

	/** Total upload size */
	int32 UploadSize;

	//~ Delegates Begin
	FSketchfabTaskDelegate OnAssetDownloadedDelegate;
	FSketchfabTaskDelegate OnAssetUploadedDelegate;
	FSketchfabTaskDelegate OnProgressUpdated;
	FSketchfabTaskDelegate OnTaskFailedDelegate;
	FSketchfabTaskDelegate OnThumbnailRequiredDelegate;
	FSketchfabTaskDelegate OnModelListDelegate;
	//~ Delegates End

	/** Map that stores pending request. They need to be cleaned up when destroying the instance. Especially if job has completed*/
	TMap<TSharedPtr<IHttpRequest>, FString> PendingRequests;
};
