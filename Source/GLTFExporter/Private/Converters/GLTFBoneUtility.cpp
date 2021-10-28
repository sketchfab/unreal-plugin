// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFBoneUtility.h"
#include "Engine.h"

FTransform FGLTFBoneUtility::GetBindTransform(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex)
{
	const TArray<FMeshBoneInfo>& BoneInfos = RefSkeleton.GetRefBoneInfo();
	const TArray<FTransform>& BonePoses = RefSkeleton.GetRefBonePose();

	int32 CurBoneIndex = BoneIndex;
	FTransform BindTransform = FTransform::Identity;

	do
	{
		BindTransform = BindTransform * BonePoses[CurBoneIndex];
		CurBoneIndex = BoneInfos[CurBoneIndex].ParentIndex;
	} while (CurBoneIndex != INDEX_NONE);

	return BindTransform;
}

void FGLTFBoneUtility::InitializeToSkeleton(FBoneContainer& BoneContainer, const USkeleton* Skeleton)
{
	TArray<FBoneIndexType> RequiredBoneIndices;
	RequiredBoneIndices.AddUninitialized(Skeleton->GetReferenceSkeleton().GetNum());

	for (int32 BoneIndex = 0; BoneIndex < RequiredBoneIndices.Num(); ++BoneIndex)
	{
		RequiredBoneIndices[BoneIndex] = BoneIndex;
	}

	BoneContainer.SetUseRAWData(true);
	BoneContainer.InitializeTo(RequiredBoneIndices, FCurveEvaluationOption(true), *const_cast<USkeleton*>(Skeleton));
}

void FGLTFBoneUtility::RetargetTransform(const UAnimSequence* AnimSequence, FTransform& BoneTransform, int32 SkeletonBoneIndex, int32 BoneIndex, const FBoneContainer& RequiredBones)
{
	const USkeleton* Skeleton = AnimSequence->GetSkeleton();
	const FName& RetargetSource = AnimSequence->RetargetSource;
	FAnimationRuntime::RetargetBoneTransform(Skeleton, RetargetSource, BoneTransform, SkeletonBoneIndex, FCompactPoseBoneIndex(BoneIndex), RequiredBones, false);
}
