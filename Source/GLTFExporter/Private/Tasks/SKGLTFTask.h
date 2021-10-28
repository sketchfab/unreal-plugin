// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

enum class ESKGLTFTaskPriority : uint8
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

	const ESKGLTFTaskPriority Priority;

	FGLTFTask(ESKGLTFTaskPriority Priority)
		: Priority(Priority)
	{
	}

	virtual ~FGLTFTask() = default;

	virtual FString GetName() = 0;

	virtual void Complete() = 0;
};
