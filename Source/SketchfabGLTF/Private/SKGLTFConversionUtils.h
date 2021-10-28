// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

THIRD_PARTY_INCLUDES_START
#include "tiny_gltf.h"
THIRD_PARTY_INCLUDES_END

#include "SKGLTFImporter.h"

namespace GLTFUtils
{
	template<typename T>
	T* FindOrCreateObject(UObject* InParent, const FString& InName, EObjectFlags Flags)
	{
		T* Object = FindObject<T>(InParent, *InName);

		if (!Object)
		{
			Object = NewObject<T>(InParent, FName(*InName), Flags);
		}

		return Object;
	}
}

namespace GLTFToUnreal
{
	static FString ConvertString(const std::string& InString)
	{
		return FString(ANSI_TO_TCHAR(InString.c_str()));
	}

	static FString ConvertString(const char* InString)
	{
		return FString(ANSI_TO_TCHAR(InString));
	}

	static FName ConvertName(const char* InString)
	{
		return FName(InString);
	}

	static FName ConvertName(const std::string& InString)
	{
		return FName(InString.c_str());
	}

	static FMatrix ConvertMatrix(const tinygltf::Node &Node)
	{
		FMatrix UnrealMtx = FMatrix::Identity;
		if (Node.matrix.size() == 16)
		{
			UnrealMtx = FMatrix(
				FPlane(Node.matrix[0], Node.matrix[1], Node.matrix[2], Node.matrix[3]),
				FPlane(Node.matrix[4], Node.matrix[5], Node.matrix[6], Node.matrix[7]),
				FPlane(Node.matrix[8], Node.matrix[9], Node.matrix[10], Node.matrix[11]),
				FPlane(Node.matrix[12], Node.matrix[13], Node.matrix[14], Node.matrix[15])
			);
		}
		else
		{
			bool found = false;
			FQuat rotation = FQuat::Identity;
			if (Node.rotation.size() == 4)
			{
				rotation.X = Node.rotation[0];
				rotation.Y = Node.rotation[1];
				rotation.Z = Node.rotation[2];
				rotation.W = Node.rotation[3];
				found = true;
			}

			FVector scale(1, 1, 1);
			if (Node.scale.size() == 3)
			{
				scale.X = Node.scale[0];
				scale.Y = Node.scale[1];
				scale.Z = Node.scale[2];
				found = true;
			}

			FVector translation(0, 0, 0);
			if (Node.translation.size() == 3)
			{
				translation.X = Node.translation[0];
				translation.Y = Node.translation[1];
				translation.Z = Node.translation[2];
				found = true;
			}

			if (found)
			{
				UnrealMtx = FScaleRotationTranslationMatrix(scale, rotation.Rotator(), translation);
			}
		}

		return UnrealMtx;
	}
}

