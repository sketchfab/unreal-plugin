// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SceneTypes.h"
#include "UObject/NameTypes.h"

/** Structure extending EMaterialProperty to allow detailed information about custom output */
struct FMaterialPropertyEx
{
	FMaterialPropertyEx(EMaterialProperty Type = MP_MAX, const FName& CustomOutput = NAME_None)
		: Type(Type)
		, CustomOutput(CustomOutput)
	{}

	FMaterialPropertyEx(const FName& CustomOutput)
		: Type(MP_CustomOutput)
		, CustomOutput(CustomOutput)
	{}

	FMaterialPropertyEx(const TCHAR* CustomOutput)
		: Type(MP_CustomOutput)
		, CustomOutput(CustomOutput)
	{}

	bool IsCustomOutput() const
	{
		return Type == MP_CustomOutput;
	}

	bool Equals(const FMaterialPropertyEx& Other) const
	{
		return Type == Other.Type && (!IsCustomOutput() || CustomOutput == Other.CustomOutput);
	}

	int32 Compare(const FMaterialPropertyEx& Other) const
	{
		return !IsCustomOutput() || !Other.IsCustomOutput() ? Type - Other.Type : CustomOutput.Compare(Other.CustomOutput);
	}

	FString ToString() const
	{
		if (!IsCustomOutput())
		{
			const UEnum* Enum = StaticEnum<EMaterialProperty>();
			FName Name = Enum->GetNameByValue(Type);
			FString TrimmedName = Name.ToString();
			TrimmedName.RemoveFromStart(TEXT("MP_"), ESearchCase::CaseSensitive);
			return TrimmedName;
		}

		return CustomOutput.ToString();
	}

	bool operator ==(const FMaterialPropertyEx& Other) const
	{
		return Equals(Other);
	}

	bool operator !=(const FMaterialPropertyEx& Other) const
	{
		return !Equals(Other);
	}

	bool operator >=(const FMaterialPropertyEx& Other) const
	{
		return Compare(Other) >= 0;
	}

	bool operator >(const FMaterialPropertyEx& Other) const
	{
		return Compare(Other) > 0;
	}

	bool operator <=(const FMaterialPropertyEx& Other) const
	{
		return Compare(Other) <= 0;
	}

	bool operator <(const FMaterialPropertyEx& Other) const
	{
		return Compare(Other) < 0;
	}

	friend uint32 GetTypeHash(const FMaterialPropertyEx& Other)
	{
		return !Other.IsCustomOutput() ? GetTypeHash(Other.Type) : GetTypeHash(Other.CustomOutput);
	}

	/** The material property */
	EMaterialProperty Type;

	/** The name of a specific custom output. Only used if property is MP_CustomOutput */
	FName CustomOutput;
};
