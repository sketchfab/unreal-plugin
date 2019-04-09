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
#include "SketchfabTask.h"

class FSketchfabRESTClient : public FRunnable
{
public:

	/* Add a task to the Queue*/
	void AddTask(TSharedPtr<FSketchfabTask>& InTask);

	void SetMaxUploadSizeInBytes(int32 InMaxUploadSizeInBytes);

	/*Get pointer to the runnable*/
	static FSketchfabRESTClient* Get();

	static void Shutdown();

private:


	FSketchfabRESTClient();

	~FSketchfabRESTClient();

	//~ Begin FRunnable Interface
	virtual bool Init();

	virtual uint32 Run();

	/* Method that goes through all task and checks their status*/
	void UpdateTaskStates();

	/* Moves jobs into the Jobs Buffer*/
	void MoveItemsToBoundedArray();

	virtual void Stop();

	virtual void Exit();
	//~ End FRunnable Interface


	void Wait(const float InSeconds, const float InSleepTime = 0.1f);

	void EnusureCompletion();

	/** Checks if there's been any Stop requests */
	FORCEINLINE bool ShouldStop() const
	{
		return StopTaskCounter.GetValue() > 0;
	}

private:

	/*Static instance*/
	static FSketchfabRESTClient* Runnable;

	/*Critical Section*/
	FCriticalSection CriticalSectionData;

	FThreadSafeCounter StopTaskCounter;

	/* a local buffer as to limit the number of concurrent jobs. */
	TArray<TSharedPtr< class FSketchfabTask>> JobsBuffer;

	/* Pending Jobs Queue */
	TQueue<TSharedPtr<class FSketchfabTask>, EQueueMode::Mpsc> PendingJobs;

	/* Thread*/
	FRunnableThread* Thread;

	/* Sketchfab Server IP Address*/
	FString HostName;

	/* API Key*/
	FString APIKey;

	/* Add a swarm task to the Queue*/
	bool bEnableDebugging;

	/* Sleep time between status updates*/
	float DelayBetweenRuns;

	/* Number of Simultaneous Jobs to Manage*/
	int32 JobLimit;

	/* Max Upload size in bytes . Should not be more than the 2GB data limit for the Server*/
	int32 MaxUploadSizeInBytes;
};