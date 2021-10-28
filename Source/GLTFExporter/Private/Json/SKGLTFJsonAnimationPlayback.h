// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonAnimationPlayback
{
	FString Name;

	bool bLoop;
	bool bAutoPlay;

	float PlayRate;
	float StartTime;

	FGLTFJsonAnimationPlayback()
		: bLoop(true)
		, bAutoPlay(true)
		, PlayRate(1)
		, StartTime(0)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		if (!Name.IsEmpty())
		{
			JsonWriter.WriteValue(TEXT("name"), Name);
		}

		if (bLoop != true)
		{
			JsonWriter.WriteValue(TEXT("loop"), bLoop);
		}

		if (bAutoPlay != true)
		{
			JsonWriter.WriteValue(TEXT("autoPlay"), bAutoPlay);
		}

		if (PlayRate != 1)
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("playRate"), PlayRate);
		}

		if (StartTime != 0)
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, TEXT("startTime"), StartTime);
		}

		JsonWriter.WriteObjectEnd();
	}

	bool operator==(const FGLTFJsonAnimationPlayback& Other) const
	{
		return bLoop == Other.bLoop
			&& bAutoPlay == Other.bAutoPlay
			&& PlayRate == Other.PlayRate
			&& StartTime == Other.StartTime;
	}

	bool operator!=(const FGLTFJsonAnimationPlayback& Other) const
	{
		return !(*this == Other);
	}
};
