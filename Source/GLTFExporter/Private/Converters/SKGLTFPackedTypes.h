// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"

// UE4 uses an endianness dependent memory layout (ABGR for little endian, ARGB for big endian)
// glTF uses RGBA independent of endianness, hence why we need this dedicated struct
struct FGLTFPackedColor
{
	uint8 R, G, B, A;

	FGLTFPackedColor(uint8 R, uint8 G, uint8 B, uint8 A = 255)
		: R(R), G(G), B(B), A(A)
	{
	}
};

// Primarily for type-safety
struct FGLTFPackedVector8
{
	int8 X, Y, Z, W;

	FGLTFPackedVector8(int8 X, int8 Y, int8 Z, int8 W = 0)
		: X(X), Y(Y), Z(Z), W(W)
	{
	}
};

// Primarily for type-safety
struct FGLTFPackedVector16
{
	int16 X, Y, Z, W;

	FGLTFPackedVector16(int16 X, int16 Y, int16 Z, int16 W = 0)
		: X(X), Y(Y), Z(Z), W(W)
	{
	}
};
