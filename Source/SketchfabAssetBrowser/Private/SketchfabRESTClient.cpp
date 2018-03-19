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
	JobLimit = 20;
	DelayBetweenRuns = 5;
	DelayBetweenRuns = DelayBetweenRuns <= 5 ? 5 : DelayBetweenRuns;
	/*
	FString IP = GetDefault<UEditorPerProjectUserSettings>()->SketchfabServerIP;
	bEnableDebugging = GetDefault<UEditorPerProjectUserSettings>()->bEnableSketchfabDebugging;
	if (IP != "")
	{
		if (!IP.Contains("http://"))
		{
			HostName = "http://" + IP;
		}
		else
		{
			HostName = IP;
		}
	}
	else
	{
		HostName = TEXT(HOSTNAME);
	}

	HostName += TEXT(PORT);
	JobLimit = GetDefault<UEditorPerProjectUserSettings>()->SketchfabNumOfConcurrentJobs;


	DelayBetweenRuns = GetDefault<UEditorPerProjectUserSettings>()->SketchfabDelay / 1000;
	DelayBetweenRuns = DelayBetweenRuns <= 5 ? 5 : DelayBetweenRuns;
	*/

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
	Wait(5);
	//float SleepDelay = DelayBetweenRuns;
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
		case SRS_ASSETUPLOADED_PENDING:
			SketchfabTask->UploadAsset();
			break;
		case SRS_FAILED:
			TasksMarkedForRemoval.Add(SketchfabTask);
			break;
		case SRS_ASSETUPLOADED:
			SketchfabTask->CreateJob();
			break;
		case SRS_JOBCREATED:
			SketchfabTask->UploadJobSettings();
			break;
		case SRS_JOBSETTINGSUPLOADED:
			SketchfabTask->ProcessJob();
			break;
		case SRS_JOBPROCESSING:
			SketchfabTask->GetJob();
			break;
		case SRS_JOBPROCESSED:
			SketchfabTask->DownloadAsset();
			break;
		case SRS_ASSETDOWNLOADED:
			TasksMarkedForRemoval.Add(SketchfabTask);
			break;
		case SRS_GETMODELS:
			SketchfabTask->GetModels();
			break;
		case SRS_GETMODELS_PROCESSING:
			SketchfabTask->GetModels();
			break;
		case SRS_GETMODELS_DONE:
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
					OutItem->CreateUploadParts(MaxUploadSizeInBytes);
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
	InTask->SetHost(HostName);
	PendingJobs.Enqueue(InTask);
}

void FSketchfabRESTClient::SetMaxUploadSizeInBytes(int32 InMaxUploadSizeInBytes)
{
	MaxUploadSizeInBytes = InMaxUploadSizeInBytes;
}

void FSketchfabRESTClient::Exit()
{

}

