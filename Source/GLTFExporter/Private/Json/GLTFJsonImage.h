// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/GLTFJsonEnums.h"
#include "Json/GLTFJsonIndex.h"
#include "Json/GLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonImage
{
	FString Name;
	FString Uri;

	EGLTFJsonMimeType MimeType;

	FGLTFJsonBufferViewIndex BufferView;

	FGLTFJsonImage()
		: MimeType(EGLTFJsonMimeType::None)
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

		if (!Uri.IsEmpty())
		{
			JsonWriter.WriteValue(TEXT("uri"), Uri);
		}

		if (MimeType != EGLTFJsonMimeType::None)
		{
			JsonWriter.WriteValue(TEXT("mimeType"), FGLTFJsonUtility::ToString(MimeType));
		}

		if (BufferView != INDEX_NONE)
		{
			JsonWriter.WriteValue(TEXT("bufferView"), BufferView);
		}

		JsonWriter.WriteObjectEnd();
	}
};
