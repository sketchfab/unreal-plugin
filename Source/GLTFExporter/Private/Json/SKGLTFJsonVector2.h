// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonVector2
{
	float X, Y;

	static const FGLTFJsonVector2 Zero;
	static const FGLTFJsonVector2 One;

	FGLTFJsonVector2(float X, float Y)
		: X(X), Y(Y)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteArray(TJsonWriter<CharType, PrintPolicy>& JsonWriter) const
	{
		JsonWriter.WriteArrayStart();
		FGLTFJsonUtility::WriteExactValue(JsonWriter, X);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, Y);
		JsonWriter.WriteArrayEnd();
	}

	bool operator==(const FGLTFJsonVector2& Other) const
	{
		return X == Other.X
			&& Y == Other.Y;
	}

	bool operator!=(const FGLTFJsonVector2& Other) const
	{
		return X != Other.X
			|| Y != Other.Y;
	}
};
