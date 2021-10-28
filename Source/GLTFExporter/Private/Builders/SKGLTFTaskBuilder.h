// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Builders/SKGLTFMessageBuilder.h"
#include "Tasks/SKGLTFTask.h"

class FGLTFTaskBuilder : public FGLTFMessageBuilder
{
protected:

	FGLTFTaskBuilder(const FString& FilePath, const USKGLTFExportOptions* ExportOptions);

public:

	template <typename TaskType, typename... TaskArgTypes, typename = typename TEnableIf<TIsDerivedFrom<TaskType, FGLTFTask>::Value>::Type>
	bool SetupTask(TaskArgTypes&&... Args)
	{
		return SetupTask(MakeUnique<TaskType>(Forward<TaskArgTypes>(Args)...));
	}

	template <typename TaskType, typename = typename TEnableIf<TIsDerivedFrom<TaskType, FGLTFTask>::Value>::Type>
	bool SetupTask(TUniquePtr<TaskType> Task)
	{
		return SetupTask(TUniquePtr<FGLTFTask>(Task.Release()));
	}

	bool SetupTask(TUniquePtr<FGLTFTask> Task);

	void CompleteAllTasks(FFeedbackContext* Context = GWarn);

private:

	static FText GetPriorityMessageFormat(ESKGLTFTaskPriority Priority);

	int32 PriorityIndexLock;
	TMap<ESKGLTFTaskPriority, TArray<TUniquePtr<FGLTFTask>>> TasksByPriority;
};
