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

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 25
typedef TSharedRef<IHttpRequest> ReqRef;
typedef TSharedPtr<IHttpRequest> ReqPtr;
#else
typedef TSharedRef<IHttpRequest, ESPMode::ThreadSafe> ReqRef;
typedef TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ReqPtr;
#endif

/** Enum representing state used by Sketchfab Server */
enum SketchfabRESTState
{
	SRS_UNKNOWN,
	SRS_FAILED,
	SRS_CHECK_LATEST_VERSION,
	SRS_CHECK_LATEST_VERSION_PROCESSING,
	SRS_CHECK_LATEST_VERSION_DONE,
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
	SRS_GETUSERORGS,
	SRS_GETUSERORGS_PROCESSING,
	SRS_GETUSERORGS_DONE,
	SRS_GETORGSPROJECTS,
	SRS_GETORGSPROJECTS_PROCESSING,
	SRS_GETORGSPROJECTS_DONE,
	SRS_GETUSERTHUMB,
	SRS_GETUSERTHUMB_PROCESSING,
	SRS_GETUSERTHUMB_DONE,
	SRS_GETCATEGORIES,
	SRS_GETCATEGORIES_PROCESSING,
	SRS_GETCATEGORIES_DONE,
	SRS_GETMODELINFO,
	SRS_GETMODELINFO_PROCESSING,
	SRS_GETMODELINFO_DONE,
	SRS_UPLOADMODEL,
	SRS_UPLOADMODEL_PROCESSING,
	SRS_UPLOADMODEL_DONE,
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

struct FSketchfabProject
{
	FString uid;
	FString name;
	FString slug;
	bool active;
	int modelCount;
	int memberCount;

	FSketchfabProject()
	{
		active = false;
	}
};

struct FSketchfabOrg
{
	FString uid;
	FString name;
	FString url;
	bool active;
	TArray<FSketchfabProject> Projects;

	FSketchfabOrg()
	{
		active = false;
	}
};

/** Sketchfab plans (to determine which features can be accessed)**/
enum SketchfabAccountType
{
	ACCOUNT_BASIC,
	ACCOUNT_PLUS,
	ACCOUNT_PRO,
	ACCOUNT_PREMIUM,
	ACCOUNT_BUSINESS,
	ACCOUNT_ENTERPRISE,
};

struct FSketchfabSession
{
	/** The Logged in user name */
	FString UserName;

	/** The Logged in user display name */
	FString UserDisplayName;

	/** The type of user account */
	SketchfabAccountType UserPlan;

	/** The Logged in user id */
	FString UserUID;

	/** The Logged in user Thumbnail download URL */
	FString UserThumbnaillURL;

	/** The Logged in user Thumbnail download UID */
	FString UserThumbnaillUID;
};

struct FSketchfabTaskData
{
	/** Latest version of the plugin **/
	FString LatestPluginVersion;

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

	/** Date model was published */
	FDateTime ModelPublishedAt;

	/** Model Author Name */
	FString ModelAuthor;

	/** Download size of the model in bytes */
	uint64 DownloadedBytes;

	/** Thumbnail Unique Identifier */
	FString ThumbnailUID;

	/** Thumbnail Unique Identifier for a thumbnail that is up to 1024 in size */
	FString ThumbnailUID_1024;

	/** Thumbnail Download URL */
	FString ThumbnailURL;

	/** Thumbnail Download URL */
	FString ThumbnailURL_1024;

	int32 ThumbnailWidth;
	int32 ThumbnailWidth_1024;

	int32 ThumbnailHeight;
	int32 ThumbnailHeight_1024;

	/** The Cache folder for downloaded content */
	FString CacheFolder;

	/** Current user session */
	FSketchfabSession UserSession;

	/** The Next Page URL to get */
	FString NextURL;

	TArray<FSketchfabCategory> Categories;
	TArray<FSketchfabOrg> Orgs;
	TArray<FSketchfabProject> Projects;
	FSketchfabOrg* org;

	bool OrgModel;
	FString OrgUID;
	FString ProjectUID;
	bool UsesOrgProfile;

	int32 ModelVertexCount;
	int32 ModelFaceCount;
	int32 AnimationCount;

	FString LicenceType;
	FString LicenceInfo;

	/** Upload info */
	// Already has ModelName
	FString ModelDescription;
	FString ModelTags;
	FString FileToUpload;
	FString ModelPassword;
	bool Draft;
	bool Private;
	bool BakeMaterials;
	// Return value (to get uploaded model link)
	FString uid;
	FString uri;

	FSketchfabTaskData()
	{
		ModelSize = 0;
		DownloadedBytes = 0;
		ThumbnailWidth = 0;
		ThumbnailHeight = 0;
		ModelVertexCount = 0;
		ModelFaceCount = 0;
		AnimationCount = 0;
	}

};

/** Sketchfab Swarm Task. Responsible for communicating with the Server  */
class FSketchfabTask
{

public:
	DECLARE_DELEGATE_OneParam(FSketchfabTaskDelegate, const FSketchfabTask&);

	FSketchfabTask(const FSketchfabTaskData& InTaskData);
	~FSketchfabTask();

	bool MakeRequest(
		FString url,
		SketchfabRESTState successState,
		void (FSketchfabTask::* completeCallback)(FHttpRequestPtr, FHttpResponsePtr, bool),
		FString contentType = "",
		void (FSketchfabTask::* progressCallback)(FHttpRequestPtr, int32, int32) = nullptr,
		bool upload = false,
		bool authorization = true
	);

	bool IsValid(FHttpRequestPtr Request, FHttpResponsePtr Response, FString expectedType = "", bool checkData = false, bool checkCache = false );


	/* Method to get/set Task Sate */
	SketchfabRESTState GetState() const;
	void SetState(SketchfabRESTState InState);

	bool IsFinished() const;

	/*~ Helpers */
	FDateTime GetPublishedAt(TSharedPtr<FJsonObject> JsonObject);

	/*~ Events */
	FSketchfabTaskDelegate& OnTaskFailed() { return OnTaskFailedDelegate; }
	FSketchfabTaskDelegate& OnCheckLatestVersion() { return OnCheckLatestVersionDelegate; }
	FSketchfabTaskDelegate& OnSearch() { return OnSearchDelegate; }
	FSketchfabTaskDelegate& OnThumbnailDownloaded() { return OnThumbnailDownloadedDelegate; }
	FSketchfabTaskDelegate& OnModelLink() { return OnModelLinkDelegate; }
	FSketchfabTaskDelegate& OnModelDownloaded() { return OnModelDownloadedDelegate; }
	FSketchfabTaskDelegate& OnModelDownloadProgress() { return OnModelDownloadProgressDelegate; }
	FSketchfabTaskDelegate& OnUserData() { return OnUserDataDelegate; }
	FSketchfabTaskDelegate& OnUserOrgs() { return OnUserOrgsDelegate; }
	FSketchfabTaskDelegate& OnOrgsProjects() { return OnUserOrgsDelegate; }
	FSketchfabTaskDelegate& OnUserThumbnail() { return OnUserThumbnailDelegate; }
	FSketchfabTaskDelegate& OnCategories() { return OnCategoriesDelegate; }
	FSketchfabTaskDelegate& OnModelInfo() { return OnModelInfoDelegate; }
	FSketchfabTaskDelegate& OnModelUploaded() { return OnModelUploadedDelegate; }

	void EnableDebugLogging();

	//~Being Rest methods
	void CheckLatestPluginVersion();
	void Search();
	void GetThumbnail();
	void GetModelLink();
	void DownloadModel();
	void GetUserData();
	void GetUserOrgs();
	void GetOrgsProjects();
	void GetUserThumbnail();
	void GetCategories();
	void GetModelInfo();
	void UploadModel();
	void SetUploadRequestContent(ReqRef Request);

	//
	void AddAuthorization(ReqRef Request);
	void DownloadModelProgress(FHttpRequestPtr HttpRequest, int32 BytesSent, int32 BytesReceived);

	//~Being Response methods
	void Check_Latest_Version_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void Search_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void GetThumbnail_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void GetModelLink_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void DownloadModel_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void GetUserData_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void GetUserOrgs_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void GetOrgsProjects_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void GetUserThumbnail_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void GetCategories_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void GetModelInfo_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);
	void UploadModel_Response(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	//~End Rest methods

	/** Essential Task Data */
	FSketchfabTaskData TaskData;

	/** All the data for every model found in a search call */
	TArray<TSharedPtr< struct FSketchfabTaskData>> SearchData;

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
	FSketchfabTaskDelegate OnCheckLatestVersionDelegate;
	FSketchfabTaskDelegate OnSearchDelegate;
	FSketchfabTaskDelegate OnThumbnailDownloadedDelegate;
	FSketchfabTaskDelegate OnModelLinkDelegate;
	FSketchfabTaskDelegate OnModelDownloadedDelegate;
	FSketchfabTaskDelegate OnModelDownloadProgressDelegate;
	FSketchfabTaskDelegate OnUserDataDelegate;
	FSketchfabTaskDelegate OnUserOrgsDelegate;
	FSketchfabTaskDelegate OnOrgsProjectsDelegate;
	FSketchfabTaskDelegate OnUserThumbnailDelegate;
	FSketchfabTaskDelegate OnCategoriesDelegate;
	FSketchfabTaskDelegate OnModelInfoDelegate;
	FSketchfabTaskDelegate OnModelUploadedDelegate;
	//~ Delegates End

	/** Map that stores pending request. They need to be cleaned up when destroying the instance. Especially if job has completed*/
	TMap<ReqPtr, FString> PendingRequests;
};
