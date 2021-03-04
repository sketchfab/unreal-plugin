// Copyright Epic Games, Inc. All Rights Reserved.

#include "Builders/GLTFTaskBuilder.h"
#include "Misc/FeedbackContext.h"
#include "Misc/ScopedSlowTask.h"

FGLTFTaskBuilder::FGLTFTaskBuilder(const FString& FilePath, const UGLTFExportOptions* ExportOptions)
	: FGLTFMessageBuilder(FilePath, ExportOptions)
	, PriorityIndexLock(INDEX_NONE)
{
}

void FGLTFTaskBuilder::CompleteAllTasks(FFeedbackContext* Context)
{
	const int32 PriorityCount = static_cast<int32>(EGLTFTaskPriority::MAX);
	for (int32 PriorityIndex = 0; PriorityIndex < PriorityCount; PriorityIndex++)
	{
		PriorityIndexLock = PriorityIndex;
		const EGLTFTaskPriority Priority = static_cast<EGLTFTaskPriority>(PriorityIndex);

		TArray<TUniquePtr<FGLTFTask>>* Tasks = TasksByPriority.Find(Priority);
		if (Tasks == nullptr)
		{
			continue;
		}

		const FText MessageFormat = GetPriorityMessageFormat(Priority);
		FScopedSlowTask Progress(Tasks->Num(), FText::Format(MessageFormat, FText()), true, *Context);
		Progress.MakeDialog();

		for (TUniquePtr<FGLTFTask>& Task : *Tasks)
		{
			const FText Name = FText::FromString(Task->GetName());
			const FText Message = FText::Format(MessageFormat, Name);
			Progress.EnterProgressFrame(1, Message);

			Task->Complete();
		}
	}

	TasksByPriority.Empty();
	PriorityIndexLock = INDEX_NONE;
}

bool FGLTFTaskBuilder::SetupTask(TUniquePtr<FGLTFTask> Task)
{
	const EGLTFTaskPriority Priority = Task->Priority;
	if (static_cast<int32>(Priority) >= static_cast<int32>(EGLTFTaskPriority::MAX))
	{
		checkNoEntry();
		return false;
	}

	if (static_cast<int32>(Priority) <= PriorityIndexLock)
	{
		checkNoEntry();
		return false;
	}

	TasksByPriority.FindOrAdd(Priority).Add(MoveTemp(Task));
	return true;
}

FText FGLTFTaskBuilder::GetPriorityMessageFormat(EGLTFTaskPriority Priority)
{
	switch (Priority)
	{
		case EGLTFTaskPriority::Animation: return NSLOCTEXT("GLTFExporter", "AnimationTaskMessage", "Animation(s): {0}");
		case EGLTFTaskPriority::Mesh:      return NSLOCTEXT("GLTFExporter", "MeshTaskMessage", "Mesh(es): {0}");
		case EGLTFTaskPriority::Material:  return NSLOCTEXT("GLTFExporter", "MaterialTaskMessage", "Material(s): {0}");
		case EGLTFTaskPriority::Texture:   return NSLOCTEXT("GLTFExporter", "TextureTaskMessage", "Texture(s): {0}");
		default:
			checkNoEntry();
			return {};
	}
}
