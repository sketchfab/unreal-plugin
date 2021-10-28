// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#include "SketchfabRESTClient.h"
#include "HAL/RunnableThread.h"
#include "Editor/EditorPerProjectUserSettings.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#define HOSTNAME "http://127.0.0.1"
#define PORT ":55002"

DEFINE_LOG_CATEGORY_STATIC(LogSketchfabRESTClient, Verbose, All);

FSketchfabRESTClient* FSketchfabRESTClient::Runnable = nullptr;

FSketchfabRESTClient::FSketchfabRESTClient()
	:
	Thread(nullptr),
	HostName(TEXT(HOSTNAME)),
	APIKey(TEXT("LOCAL")),
	bEnableDebugging(false)
{
	bEnableDebugging = false;
	HostName = "";
	JobLimit = 24; //Same number of thumbs on a page
	DelayBetweenRuns = 0.1; //Time in seconds before next batch is handled. We don't need to wait much for this.
	Thread = FRunnableThread::Create(this, TEXT("SketchfabRESTClient"));
}

FSketchfabRESTClient::~FSketchfabRESTClient()
{
	if (Thread != nullptr)
	{
		delete Thread;
		Thread = nullptr;
	}
}


bool FSketchfabRESTClient::Init()
{
	return true;
}


void FSketchfabRESTClient::Wait(const float InSeconds, const float InSleepTime /* = 0.1f */)
{
	// Instead of waiting the given amount of seconds doing nothing
	// check periodically if there's been any Stop requests.
	for (float TimeToWait = InSeconds; TimeToWait > 0.0f && ShouldStop() == false; TimeToWait -= InSleepTime)
	{
		FPlatformProcess::Sleep(FMath::Min(InSleepTime, TimeToWait));
	}
}

uint32 FSketchfabRESTClient::Run()
{
	do
	{

		MoveItemsToBoundedArray();

		UpdateTaskStates();

		Wait(DelayBetweenRuns);

	} while (ShouldStop() == false);


	return 0;
}

void FSketchfabRESTClient::UpdateTaskStates()
{
	TArray<TSharedPtr<class FSketchfabTask>> TasksMarkedForRemoval;
	//TasksMarkedForRemoval.AddUninitialized(JobLimit);

	FScopeLock Lock(&CriticalSectionData);

	for (int32 Index = 0; Index < JobsBuffer.Num(); Index++)
	{
		//
		TSharedPtr<FSketchfabTask>& SketchfabTask = JobsBuffer[Index];
		switch (SketchfabTask->GetState())
		{
		case SRS_UNKNOWN:
			break;
		case SRS_FAILED:
			TasksMarkedForRemoval.Add(SketchfabTask);
			break;
		case SRS_SEARCH:
			SketchfabTask->Search();
			break;
		case SRS_SEARCH_PROCESSING:
			break;
		case SRS_SEARCH_DONE:
			TasksMarkedForRemoval.Add(SketchfabTask);
			break;
		case SRS_GETTHUMBNAIL:
			SketchfabTask->GetThumbnail();
			break;
		case SRS_GETTHUMBNAIL_PROCESSING:
			break;
		case SRS_GETTHUMBNAIL_DONE:
			TasksMarkedForRemoval.Add(SketchfabTask);
			break;
		case SRS_GETMODELLINK:
			SketchfabTask->GetModelLink();
			break;
		case SRS_GETMODELLINK_PROCESSING:
			break;
		case SRS_GETMODELLINK_DONE:
			TasksMarkedForRemoval.Add(SketchfabTask);
			break;
		case SRS_DOWNLOADMODEL:
			SketchfabTask->DownloadModel();
			break;
		case SRS_DOWNLOADMODEL_PROCESSING:
			break;
		case SRS_DOWNLOADMODEL_DONE:
			TasksMarkedForRemoval.Add(SketchfabTask);
			break;
		case SRS_GETUSERDATA:
			SketchfabTask->GetUserData();
			break;
		case SRS_GETUSERDATA_PROCESSING:
			break;
		case SRS_GETUSERDATA_DONE:
			TasksMarkedForRemoval.Add(SketchfabTask);
			break;
		case SRS_GETUSERORGS:
			SketchfabTask->GetUserOrgs();
			break;
		case SRS_GETUSERORGS_PROCESSING:
			break;
		case SRS_GETUSERORGS_DONE:
			TasksMarkedForRemoval.Add(SketchfabTask);
			break;
		case SRS_GETORGSPROJECTS:
			SketchfabTask->GetOrgsProjects();
			break;
		case SRS_GETORGSPROJECTS_PROCESSING:
			break;
		case SRS_GETORGSPROJECTS_DONE:
			TasksMarkedForRemoval.Add(SketchfabTask);
			break;
		case SRS_GETUSERTHUMB:
			SketchfabTask->GetUserThumbnail();
			break;
		case SRS_GETUSERTHUMB_PROCESSING:
			break;
		case SRS_GETUSERTHUMB_DONE:
			TasksMarkedForRemoval.Add(SketchfabTask);
			break;
		case SRS_GETCATEGORIES:
			SketchfabTask->GetCategories();
			break;
		case SRS_GETCATEGORIES_PROCESSING:
			break;
		case SRS_GETCATEGORIES_DONE:
			TasksMarkedForRemoval.Add(SketchfabTask);
			break;
		case SRS_GETMODELINFO:
			SketchfabTask->GetModelInfo();
			break;
		case SRS_GETMODELINFO_PROCESSING:
			break;
		case SRS_GETMODELINFO_DONE:
			TasksMarkedForRemoval.Add(SketchfabTask);
			break;
		case SRS_UPLOADMODEL:
			SketchfabTask->UploadModel();
			break;
		case SRS_UPLOADMODEL_PROCESSING:
			break;
		case SRS_UPLOADMODEL_DONE:
			TasksMarkedForRemoval.Add(SketchfabTask);
			break;
		}
	}

	for (int32 Index = 0; Index < TasksMarkedForRemoval.Num(); Index++)
	{

		if (TasksMarkedForRemoval[Index]->GetState() == SRS_FAILED)
		{
			TasksMarkedForRemoval[Index]->OnTaskFailed().ExecuteIfBound(*TasksMarkedForRemoval[Index]);
		}
		JobsBuffer.Remove(TasksMarkedForRemoval[Index]);
	}

	TasksMarkedForRemoval.Empty();
}

void FSketchfabRESTClient::MoveItemsToBoundedArray()
{
	if (!PendingJobs.IsEmpty())
	{
		int32 ItemsToInsert = 0;
		if (JobsBuffer.Num() >= JobLimit)
		{
			ItemsToInsert = JobLimit;
		}
		else if (JobsBuffer.Num() < JobLimit)
		{
			JobsBuffer.Shrink();
			ItemsToInsert = JobLimit - JobsBuffer.Num();

			FScopeLock Lock(&CriticalSectionData);
			do
			{
				TSharedPtr<FSketchfabTask> OutItem;
				if (PendingJobs.Dequeue(OutItem))
				{
					JobsBuffer.Add(OutItem);
				}

			} while (!PendingJobs.IsEmpty() && JobsBuffer.Num() < JobLimit);
		}

	}
}

FSketchfabRESTClient* FSketchfabRESTClient::Get()
{
	if (!Runnable && FPlatformProcess::SupportsMultithreading())
	{
		Runnable = new FSketchfabRESTClient();
	}
	return Runnable;
}

void FSketchfabRESTClient::Shutdown()
{
	if (Runnable)
	{
		Runnable->EnusureCompletion();
		delete Runnable;
		Runnable = nullptr;
	}
}

void FSketchfabRESTClient::Stop()
{
	StopTaskCounter.Increment();
}

void FSketchfabRESTClient::EnusureCompletion()
{
	Stop();
	Thread->WaitForCompletion();
}

void FSketchfabRESTClient::AddTask(TSharedPtr<FSketchfabTask>& InTask)
{
	//FScopeLock Lock(&CriticalSectionData);
	PendingJobs.Enqueue(InTask);
}

void FSketchfabRESTClient::SetMaxUploadSizeInBytes(int32 InMaxUploadSizeInBytes)
{
	MaxUploadSizeInBytes = InMaxUploadSizeInBytes;
}

void FSketchfabRESTClient::Exit()
{

}

