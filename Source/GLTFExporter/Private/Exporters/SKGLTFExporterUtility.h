// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

class UStaticMesh;
class UMaterialInterface;
class UAnimSequence;
class ULevelSequence;

struct FGLTFExporterUtility
{
	static const UStaticMesh* GetPreviewMesh(const UMaterialInterface* Material);
	static const USkeletalMesh* GetPreviewMesh(const UAnimSequence* AnimSequence);

	static TArray<UWorld*> GetReferencedWorlds(const UObject* LevelSequence);
};
