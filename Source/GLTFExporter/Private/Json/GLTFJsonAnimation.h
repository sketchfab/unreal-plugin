// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/GLTFJsonEnums.h"
#include "Json/GLTFJsonIndex.h"
#include "Json/GLTFJsonAnimationPlayback.h"
#include "Json/GLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonAnimationChannelTarget
{
	FGLTFJsonNodeIndex Node;
	EGLTFJsonTargetPath Path;

	FGLTFJsonAnimationChannelTarget()
		: Path(EGLTFJsonTargetPath::None)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		JsonWriter.WriteValue(TEXT("node"), Node);
		JsonWriter.WriteValue(TEXT("path"), FGLTFJsonUtility::ToString(Path));

		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonAnimationChannel
{
	FGLTFJsonAnimationSamplerIndex Sampler;
	FGLTFJsonAnimationChannelTarget Target;

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		JsonWriter.WriteValue(TEXT("sampler"), Sampler);

		JsonWriter.WriteIdentifierPrefix(TEXT("target"));
		Target.WriteObject(JsonWriter, Extensions);

		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonAnimationSampler
{
	FGLTFJsonAccessorIndex Input;
	FGLTFJsonAccessorIndex Output;

	EGLTFJsonInterpolation Interpolation;

	FGLTFJsonAnimationSampler()
		: Interpolation(EGLTFJsonInterpolation::Linear)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		JsonWriter.WriteValue(TEXT("input"), Input);
		JsonWriter.WriteValue(TEXT("output"), Output);

		if (Interpolation != EGLTFJsonInterpolation::Linear)
		{
			JsonWriter.WriteValue(TEXT("interpolation"), FGLTFJsonUtility::ToString(Interpolation));
		}

		JsonWriter.WriteObjectEnd();
	}
};

struct FGLTFJsonAnimation
{
	FString Name;

	TArray<FGLTFJsonAnimationChannel> Channels;
	TArray<FGLTFJsonAnimationSampler> Samplers;

	FGLTFJsonAnimationPlayback Playback;

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		if (!Name.IsEmpty())
		{
			JsonWriter.WriteValue(TEXT("name"), Name);
		}

		FGLTFJsonUtility::WriteObjectArray(JsonWriter, TEXT("channels"), Channels, Extensions, true);
		FGLTFJsonUtility::WriteObjectArray(JsonWriter, TEXT("samplers"), Samplers, Extensions, true);

		if (Playback != FGLTFJsonAnimationPlayback())
		{
			const EGLTFJsonExtension Extension = EGLTFJsonExtension::EPIC_AnimationPlayback;
			Extensions.Used.Add(Extension);

			JsonWriter.WriteObjectStart(TEXT("extensions"));
			JsonWriter.WriteIdentifierPrefix(FGLTFJsonUtility::ToString(Extension));
			Playback.WriteObject(JsonWriter, Extensions);
			JsonWriter.WriteObjectEnd();
		}

		JsonWriter.WriteObjectEnd();
	}
};
