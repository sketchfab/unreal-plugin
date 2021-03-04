// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

enum class EGLTFTaskPriority : uint8
{
	Animation,
	Mesh,
	Material,
	Texture,
	MAX
};

class FGLTFTask
{
public:

	const EGLTFTaskPriority Priority;

	FGLTFTask(EGLTFTaskPriority Priority)
		: Priority(Priority)
	{
	}

	virtual ~FGLTFTask() = default;

	virtual FString GetName() = 0;

	virtual void Complete() = 0;
};
