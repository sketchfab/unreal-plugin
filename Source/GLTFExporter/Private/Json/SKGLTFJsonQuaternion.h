// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonUtility.h"
#include "Serialization/JsonSerializer.h"

struct FGLTFJsonQuaternion
{
	float X, Y, Z, W;

	static const FGLTFJsonQuaternion Identity;

	FGLTFJsonQuaternion(float X, float Y, float Z, float W)
		: X(X), Y(Y), Z(Z), W(W)
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

	bool operator==(const FGLTFJsonQuaternion& Other) const
	{
		return X == Other.X
			&& Y == Other.Y
			&& Z == Other.Z
			&& W == Other.W;
	}

	bool operator!=(const FGLTFJsonQuaternion& Other) const
	{
		return X != Other.X
			|| Y != Other.Y
			|| Z != Other.Z
			|| W != Other.W;
	}
};
