// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonEnums.h"

struct FGLTFJsonExtensions
{
	TSet<ESKGLTFJsonExtension> Used;
	TSet<ESKGLTFJsonExtension> Required;
};
