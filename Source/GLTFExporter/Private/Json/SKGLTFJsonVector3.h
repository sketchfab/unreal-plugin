// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonVector2.h"
#include "Json/SKGLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonVector3
{
	float X, Y, Z;

	static const FGLTFJsonVector3 Zero;
	static const FGLTFJsonVector3 One;

	FGLTFJsonVector3(float X, float Y, float Z)
		: X(X), Y(Y), Z(Z)
	{
	}

	FGLTFJsonVector3(const FGLTFJsonVector2& Vector2, float Z)
		: X(Vector2.X), Y(Vector2.Y), Z(Z)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteArray(TJsonWriter<CharType, PrintPolicy>& JsonWriter) const
	{
		JsonWriter.WriteArrayStart();
		FGLTFJsonUtility::WriteExactValue(JsonWriter, X);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, Y);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, Z);
		JsonWriter.WriteArrayEnd();
	}

	bool operator==(const FGLTFJsonVector3& Other) const
	{
		return X == Other.X
			&& Y == Other.Y
			&& Z == Other.Z;
	}

	bool operator!=(const FGLTFJsonVector3& Other) const
	{
		return X != Other.X
			|| Y != Other.Y
			|| Z != Other.Z;
	}
};
