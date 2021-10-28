// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

class FGLTFConvertBuilder;

class FGLTFBuilderContext
{
public:

	FGLTFBuilderContext(FGLTFConvertBuilder& Builder)
		: Builder(Builder)
	{
	}

protected:

	FGLTFConvertBuilder& Builder;
};
