// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonEnums.h"
#include "Json/SKGLTFJsonIndex.h"
#include "Json/SKGLTFJsonAnimationPlayback.h"
#include "Json/SKGLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonAnimationChannelTarget
{
	FGLTFJsonNodeIndex Node;
	ESKGLTFJsonTargetPath Path;

	FGLTFJsonAnimationChannelTarget()
		: Path(ESKGLTFJsonTargetPath::None)
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

	ESKGLTFJsonInterpolation Interpolation;

	FGLTFJsonAnimationSampler()
		: Interpolation(ESKGLTFJsonInterpolation::Linear)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteObject(TJsonWriter<CharType, PrintPolicy>& JsonWriter, FGLTFJsonExtensions& Extensions) const
	{
		JsonWriter.WriteObjectStart();

		JsonWriter.WriteValue(TEXT("input"), Input);
		JsonWriter.WriteValue(TEXT("output"), Output);

		if (Interpolation != ESKGLTFJsonInterpolation::Linear)
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
			const ESKGLTFJsonExtension Extension = ESKGLTFJsonExtension::EPIC_AnimationPlayback;
			Extensions.Used.Add(Extension);

			JsonWriter.WriteObjectStart(TEXT("extensions"));
			JsonWriter.WriteIdentifierPrefix(FGLTFJsonUtility::ToString(Extension));
			Playback.WriteObject(JsonWriter, Extensions);
			JsonWriter.WriteObjectEnd();
		}

		JsonWriter.WriteObjectEnd();
	}
};
