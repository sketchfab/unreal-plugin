// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MaterialCompiler.h"

class FGLTFProxyMaterialCompiler : public FProxyMaterialCompiler
{
	using FProxyMaterialCompiler::FProxyMaterialCompiler;

	virtual int32 RayTracingQualitySwitchReplace(int32 Normal, int32 RayTraced) override
	{
		return Normal; // ignore RayTraced branch
	}

	virtual int32 ReflectionAboutCustomWorldNormal(int32 CustomWorldNormal, int32 bNormalizeCustomWorldNormal) override
	{
		return Compiler->ReflectionAboutCustomWorldNormal(CustomWorldNormal, bNormalizeCustomWorldNormal);
	}

	virtual int32 ParticleRelativeTime() override
	{
		return Compiler->ParticleRelativeTime();
	}

	virtual int32 ParticleMotionBlurFade() override
	{
		return Compiler->ParticleMotionBlurFade();
	}

	virtual int32 ParticleRandom() override
	{
		return Compiler->ParticleRandom();
	}

	virtual int32 ParticleDirection() override
	{
		return Compiler->ParticleDirection();
	}

	virtual int32 ParticleSpeed() override
	{
		return Compiler->ParticleSpeed();
	}

	virtual int32 ParticleSize() override
	{
		return Compiler->ParticleSize();
	}

	virtual int32 VertexInterpolator(uint32 InterpolatorIndex) override
	{
		return Compiler->VertexInterpolator(InterpolatorIndex);
	}

	virtual int32 MaterialBakingWorldPosition() override
	{
		return Compiler->MaterialBakingWorldPosition();
	}

	virtual EMaterialCompilerType GetCompilerType() const override
	{
		return EMaterialCompilerType::MaterialProxy;
	}

#if !(ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION <= 25)
	virtual int32 PreSkinVertexOffset() override
	{
		return Compiler->PreSkinVertexOffset();
	}

	virtual int32 PostSkinVertexOffset() override
	{
		return Compiler->PostSkinVertexOffset();
	}
#endif
};
