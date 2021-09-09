// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFSkinConverters.h"
#include "Converters/SKGLTFConverterUtility.h"
#include "Converters/SKGLTFBoneUtility.h"
#include "Builders/SKGLTFConvertBuilder.h"

FGLTFJsonSkinIndex FGLTFSkinConverter::Convert(FGLTFJsonNodeIndex RootNode, const USkeletalMesh* SkeletalMesh)
{
	FGLTFJsonSkin Skin;
	Skin.Name = SkeletalMesh->Skeleton != nullptr ? SkeletalMesh->Skeleton->GetName() : SkeletalMesh->GetName();
	Skin.Skeleton = RootNode;

	const int32 BoneCount = SkeletalMesh->RefSkeleton.GetNum();
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
		const FTransform InverseBindTransform = FGLTFBoneUtility::GetBindTransform(SkeletalMesh->RefSkeleton, BoneIndex).Inverse();
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
