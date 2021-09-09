// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FGLTFBoneUtility
{
	static FTransform GetBindTransform(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex);

	static void InitializeToSkeleton(FBoneContainer& BoneContainer, const USkeleton* Skeleton);

	static void RetargetTransform(const UAnimSequence* AnimSequence, FTransform& BoneTransform, int32 SkeletonBoneIndex, int32 BoneIndex, const FBoneContainer& RequiredBones);
};
