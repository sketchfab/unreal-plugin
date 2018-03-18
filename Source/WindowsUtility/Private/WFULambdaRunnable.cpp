#pragma once

#include "WindowsFileUtilityPrivatePCH.h"
#include "WFULambdaRunnable.h"

uint64 WFULambdaRunnable::ThreadNumber = 0;

WFULambdaRunnable::WFULambdaRunnable(TFunction< void()> InFunction)
{
	FunctionPointer = InFunction;
	Finished = false;
	Number = ThreadNumber;
	
	FString threadStatGroup = FString::Printf(TEXT("FLambdaRunnable%d"), ThreadNumber);
	Thread = NULL; 
	Thread = FRunnableThread::Create(this, *threadStatGroup, 0, TPri_BelowNormal); //windows default = 8mb for thread, could specify more
	ThreadNumber++;

	//Runnables.Add(this);
}

WFULambdaRunnable::~WFULambdaRunnable()
{
	if (Thread == NULL)
	{
		delete Thread;
		Thread = NULL;
	}

	//Runnables.Remove(this);
}

//Init
bool WFULambdaRunnable::Init()
{
	//UE_LOG(LogClass, Log, TEXT("FLambdaRunnable %d Init"), Number);
	return true;
}

//Run
uint32 WFULambdaRunnable::Run()
{
	if (FunctionPointer)
		FunctionPointer();

	//UE_LOG(LogClass, Log, TEXT("FLambdaRunnable %d Run complete"), Number);
	return 0;
}

//stop
void WFULambdaRunnable::Stop()
{
	Finished = true;
}

void WFULambdaRunnable::Exit()
{
	Finished = true;
	//UE_LOG(LogClass, Log, TEXT("FLambdaRunnable %d Exit"), Number);

	//delete ourselves when we're done
	delete this;
}

void WFULambdaRunnable::EnsureCompletion()
{
	Stop();
	Thread->WaitForCompletion();
}

WFULambdaRunnable* WFULambdaRunnable::RunLambdaOnBackGroundThread(TFunction< void()> InFunction)
{
	WFULambdaRunnable* Runnable;
	if (FPlatformProcess::SupportsMultithreading())
	{
		Runnable = new WFULambdaRunnable(InFunction);
		//UE_LOG(LogClass, Log, TEXT("FLambdaRunnable RunLambdaBackGroundThread"));
		return Runnable;
	}
	else 
	{
		return nullptr;
	}
}

FGraphEventRef WFULambdaRunnable::RunShortLambdaOnGameThread(TFunction< void()> InFunction)
{
	return FFunctionGraphTask::CreateAndDispatchWhenReady(InFunction, TStatId(), nullptr, ENamedThreads::GameThread);
}

void WFULambdaRunnable::ShutdownThreads()
{
	/*for (auto Runnable : Runnables)
	{
		if (Runnable != nullptr)
		{
			delete Runnable;
		}
		Runnable = nullptr;
	}*/
}
