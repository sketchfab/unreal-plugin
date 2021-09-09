// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonColor3
{
	float R, G, B;

	static const FGLTFJsonColor3 Black;
	static const FGLTFJsonColor3 White;

	FGLTFJsonColor3(float R, float G, float B)
		: R(R), G(G), B(B)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteArray(TJsonWriter<CharType, PrintPolicy>& JsonWriter) const
	{
		JsonWriter.WriteArrayStart();
		FGLTFJsonUtility::WriteExactValue(JsonWriter, R);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, G);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, B);
		JsonWriter.WriteArrayEnd();
	}

	bool operator==(const FGLTFJsonColor3& Other) const
	{
		return R == Other.R
			&& G == Other.G
			&& B == Other.B;
	}

	bool operator!=(const FGLTFJsonColor3& Other) const
	{
		return R != Other.R
			|| G != Other.G
			|| B != Other.B;
	}
};
