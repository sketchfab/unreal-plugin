// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonMatrix4
{
	float M[16];

	static const FGLTFJsonMatrix4 Identity;

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteArray(TJsonWriter<CharType, PrintPolicy>& JsonWriter) const
	{
		JsonWriter.WriteArrayStart();

		for(int32 I = 0; I < 16; ++I)
		{
			FGLTFJsonUtility::WriteExactValue(JsonWriter, M[I]);
		}

		JsonWriter.WriteArrayEnd();
	}

	float& operator()(int32 Row, int32 Col)
	{
		return M[Row * 4 + Col];
	}

	bool operator==(const FGLTFJsonMatrix4& Other) const
	{
		for(int32 I = 0; I < 16; ++I)
		{
			if (M[I] != Other.M[I])
			{
				return false;
			}
		}

		return true;
	}

	bool operator!=(const FGLTFJsonMatrix4& Other) const
	{
		return !(*this == Other);
	}
};
