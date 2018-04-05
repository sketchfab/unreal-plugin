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
	SRS_SEARCH,
	SRS_SEARCH_PROCESSING,
	SRS_SEARCH_DONE,
	SRS_GETTHUMBNAIL,
	SRS_GETTHUMBNAIL_PROCESSING,
	SRS_GETTHUMBNAIL_DONE,
	SRS_GETMODELLINK,
	SRS_GETMODELLINK_PROCESSING,
	SRS_GETMODELLINK_DONE,
	SRS_DOWNLOADMODEL,
	SRS_DOWNLOADMODEL_PROCESSING,
	SRS_DOWNLOADMODEL_DONE,
	SRS_GETUSERDATA,
	SRS_GETUSERDATA_PROCESSING,
	SRS_GETUSERDATA_DONE,
	SRS_GETUSERTHUMB,
	SRS_GETUSERTHUMB_PROCESSING,
	SRS_GETUSERTHUMB_DONE,
	SRS_GETCATEGORIES,
	SRS_GETCATEGORIES_PROCESSING,
	SRS_GETCATEGORIES_DONE,
};

bool IsCompleteState(SketchfabRESTState state);

struct FSketchfabCategory
{
	FString uid;
	FString uri;
	FString name;
	FString slug;
	bool active;

	FSketchfabCategory() 
	{
		active = false;
	}
};

struct FSketchfabTaskData
{
	/** Path to zip file containing resulting geometry */
	FString OutputZipFilePath;

	/** Lock for synchornization between threads */
	FCriticalSection* StateLock;

	/** Set if the task upload has been completed */
	FThreadSafeBool TaskUploadComplete;

	/** Supports Dithered Transition */
	bool bDitheredTransition;

    // Whether or not emissive should be outputted  
	bool bEmissive;

	//========================================================================
	/** Download URL path to get a list of assets and download thumbnails */
	FString ModelSearchURL;

	/** Authentication Token */
	FString Token;

	/** User Given Model Name */
	FString ModelName;

	/** Model Unique Identifier */
	FString ModelUID;

	/** Model Download URL */
	FString ModelURL;

	/** Download size of the model in bytes */
	uint64 ModelSize;

	/** Model Author Name */
	FString ModelAuthor;

	/** Download size of the model in bytes */
	uint64 DownloadedBytes;

	/** Thumbnail Unique Identifier */
	FString ThumbnailUID;

	/** Thumbnail Download URL */
	FString ThumbnailURL;

	int32 ThumbnailWidth;

	int32 ThumbnailHeight;

	/** The Cache folder for downloaded content */
	FString CacheFolder;

	/** The Logged in user name */
	FString UserName;

	/** The Logged in user id */
	FString UserUID;

	/** The Logged in user Thumbnail download URL */
	FString UserThumbnaillURL;

	/** The Logged in user Thumbnail download UID */
	FString UserThumbnaillUID;

	/** The Next Page URL to get */
	FString NextURL;

	FSketchfabTaskData()
	{
		ModelSize = 0;
		DownloadedBytes = 0;
		ThumbnailWidth = 0;
		ThumbnailHeight = 0;
	}

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
	FSketchfabTaskDelegate& OnTaskFailed() { return OnTaskFailedDelegate; }
	FSketchfabTaskDelegate& OnSearch() { return OnSearchDelegate; }
	FSketchfabTaskDelegate& OnThumbnailDownloaded() { return OnThumbnailDownloadedDelegate; }
	FSketchfabTaskDelegate& OnModelLink() { return OnModelLinkDelegate; }
	FSketchfabTaskDelegate& OnModelDownloaded() { return OnModelDownloadedDelegate; }
	FSketchfabTaskDelegate& OnModelDownloadProgress() { return OnModelDownloadProgressDelegate; }
	FSketchfabTaskDelegate& OnUserData() { return OnUserDataDelegate; }
	FSketchfabTaskDelegate& OnUserThumbnail() { return OnUserThumbnailDelegate; }
	FSketchfabTaskDelegate& OnCategories() { return OnCategoriesDelegate; }

	void EnableDebugLogging();	

	//~Being Rest methods
	void Search();
	void GetThumbnail();
	void GetModelLink();
	void DownloadModel();
	void GetUserData();
	void GetUserThumbnail();
	void GetCategories();
	//
	void AddAuthorization(TSharedRef<IHttpRequest> Request);
	void DownloadModelProgress(FHttpRequestPtr HttpRequest, int32 BytesSent, int32 BytesReceived);

	//~Being Response methods
	void Search_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void GetThumbnail_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void GetModelLink_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void DownloadModel_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void GetUserData_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void GetUserThumbnail_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void GetCategories_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	//~End Rest methods

	/** Essential Task Data */
	FSketchfabTaskData TaskData;

	/** All the data for every model found in a search call */
	TArray<TSharedPtr< struct FSketchfabTaskData>> SearchData;

	TArray<TSharedPtr< struct FSketchfabCategory>> Categories;
	
private:
	/** Task State */
	SketchfabRESTState State;

	/** Is Completed */
	FThreadSafeBool IsCompleted;

	/** Enable Debug Logging */
	bool bEnableDebugLogging;

	/** Debug HttpRequestCounter
	Note: This was added to track issues when two resposnes came for a completed job.
	Since the job was completed before the object is partially destoreyd when a new response came in.
	The import file method failed. This was added for debugging. The most likely cause if that the respose delegate is never cleaned up.
	(This must be zero else, Sometime two responses for job completed arrive which caused issue) */
	FThreadSafeCounter DebugHttpRequestCounter;

	//~ Delegates Begin
	FSketchfabTaskDelegate OnTaskFailedDelegate;
	FSketchfabTaskDelegate OnSearchDelegate;
	FSketchfabTaskDelegate OnThumbnailDownloadedDelegate;
	FSketchfabTaskDelegate OnModelLinkDelegate;
	FSketchfabTaskDelegate OnModelDownloadedDelegate;
	FSketchfabTaskDelegate OnModelDownloadProgressDelegate;
	FSketchfabTaskDelegate OnUserDataDelegate;
	FSketchfabTaskDelegate OnUserThumbnailDelegate;
	FSketchfabTaskDelegate OnCategoriesDelegate;
	//~ Delegates End

	/** Map that stores pending request. They need to be cleaned up when destroying the instance. Especially if job has completed*/
	TMap<TSharedPtr<IHttpRequest>, FString> PendingRequests;
};
