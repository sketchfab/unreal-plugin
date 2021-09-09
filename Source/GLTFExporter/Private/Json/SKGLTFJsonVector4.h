// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonVector3.h"
#include "Json/SKGLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonVector4
{
	float X, Y, Z, W;

	static const FGLTFJsonVector4 Zero;
	static const FGLTFJsonVector4 One;

	FGLTFJsonVector4(float X, float Y, float Z, float W)
		: X(X), Y(Y), Z(Z), W(W)
	{
	}

	FGLTFJsonVector4(const FGLTFJsonVector3& Vector3, float W)
		: X(Vector3.X), Y(Vector3.Y), Z(Vector3.Z), W(W)
	{
	}

	template <class CharType = TCHAR, class PrintPolicy = TPrettyJsonPrintPolicy<CharType>>
	void WriteArray(TJsonWriter<CharType, PrintPolicy>& JsonWriter) const
	{
		JsonWriter.WriteArrayStart();
		FGLTFJsonUtility::WriteExactValue(JsonWriter, X);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, Y);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, Z);
		FGLTFJsonUtility::WriteExactValue(JsonWriter, W);
		JsonWriter.WriteArrayEnd();
	}

	operator FGLTFJsonVector3() const
	{
		return { X, Y, Z };
	}

	bool operator==(const FGLTFJsonVector4& Other) const
	{
		return X == Other.X
			&& Y == Other.Y
			&& Z == Other.Z
			&& W == Other.W;
	}

	bool operator!=(const FGLTFJsonVector4& Other) const
	{
		return X != Other.X
			|| Y != Other.Y
			|| Z != Other.Z
			|| W != Other.W;
	}
};
