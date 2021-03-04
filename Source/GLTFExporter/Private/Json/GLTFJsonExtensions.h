// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/GLTFJsonEnums.h"

struct FGLTFJsonExtensions
{
	TSet<EGLTFJsonExtension> Used;
	TSet<EGLTFJsonExtension> Required;
};
