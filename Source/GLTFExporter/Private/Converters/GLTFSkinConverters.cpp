// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFSkinConverters.h"
#include "Converters/SKGLTFConverterUtility.h"
#include "Converters/SKGLTFBoneUtility.h"
#include "Builders/SKGLTFConvertBuilder.h"

FGLTFJsonSkinIndex FGLTFSkinConverter::Convert(FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh)
{
	FGLTFJsonSkin Skin;

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
	Skin.Name = SkeletalMesh->Skeleton != nullptr ? SkeletalMesh->Skeleton->GetName() : SkeletalMesh->GetName();
	Skin.Skeleton = RootNode;
	const int32 BoneCount = SkeletalMesh->RefSkeleton.GetNum();
#else
	Skin.Name = SkeletalMesh->GetSkeleton() != nullptr ? SkeletalMesh->GetSkeleton()->GetName() : SkeletalMesh->GetName();
	Skin.Skeleton = RootNode;
	const int32 BoneCount = SkeletalMesh->GetRefSkeleton().GetNum();
#endif

	if (BoneCount == 0)
	{
		// TODO: report warning
		return FGLTFJsonSkinIndex(INDEX_NONE);
	}

	Skin.Joints.AddUninitialized(BoneCount);

	for (int32 BoneIndex = 0; BoneIndex < BoneCount; ++BoneIndex)
	{
		Skin.Joints[BoneIndex] = Builder.GetOrAddNode(RootNode, SkeletalMesh, BoneIndex);
	}

	TArray<FGLTFJsonMatrix4> InverseBindMatrices;
	InverseBindMatrices.AddUninitialized(BoneCount);

	for (int32 BoneIndex = 0; BoneIndex < BoneCount; ++BoneIndex)
	{
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 26
		const FTransform InverseBindTransform = FGLTFBoneUtility::GetBindTransform(SkeletalMesh->RefSkeleton, BoneIndex).Inverse();
#else
		const FTransform InverseBindTransform = FGLTFBoneUtility::GetBindTransform(SkeletalMesh->GetRefSkeleton(), BoneIndex).Inverse();
#endif
		InverseBindMatrices[BoneIndex] = FGLTFConverterUtility::ConvertTransform(InverseBindTransform, Builder.ExportOptions->ExportUniformScale);
	}

	FGLTFJsonAccessor JsonAccessor;
	JsonAccessor.BufferView = Builder.AddBufferView(InverseBindMatrices);
	JsonAccessor.ComponentType = ESKGLTFJsonComponentType::F32;
	JsonAccessor.Count = BoneCount;
	JsonAccessor.Type = ESKGLTFJsonAccessorType::Mat4;

	Skin.InverseBindMatrices = Builder.AddAccessor(JsonAccessor);

	return Builder.AddSkin(Skin);
}
