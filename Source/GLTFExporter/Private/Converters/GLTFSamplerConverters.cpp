// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFSamplerConverters.h"
#include "Builders/SKGLTFContainerBuilder.h"
#include "Converters/SKGLTFConverterUtility.h"
#include "Converters/SKGLTFTextureUtility.h"

FGLTFJsonSamplerIndex FGLTFSamplerConverter::Convert(const UTexture* Texture)
{
	// TODO: reuse existing samplers, don't create a unique sampler per texture

	FGLTFJsonSampler JsonSampler;
	Texture->GetName(JsonSampler.Name);

	if (Texture->IsA<ULightMapTexture2D>())
	{
		// Override default filter settings for LightMap textures (which otherwise is "nearest")
		JsonSampler.MinFilter = ESKGLTFJsonTextureFilter::LinearMipmapLinear;
		JsonSampler.MagFilter = ESKGLTFJsonTextureFilter::Linear;
	}
	else
	{
		JsonSampler.MinFilter = FGLTFConverterUtility::ConvertMinFilter(Texture->Filter, Texture->LODGroup);
		JsonSampler.MagFilter = FGLTFConverterUtility::ConvertMagFilter(Texture->Filter, Texture->LODGroup);
	}

	if (FGLTFTextureUtility::IsCubemap(Texture))
	{
		JsonSampler.WrapS = ESKGLTFJsonTextureWrap::ClampToEdge;
		JsonSampler.WrapT = ESKGLTFJsonTextureWrap::ClampToEdge;
	}
	else
	{
		const TTuple<TextureAddress, TextureAddress> AddressXY = FGLTFTextureUtility::GetAddressXY(Texture);
		// TODO: report error if AddressX == TA_MAX or AddressY == TA_MAX

		JsonSampler.WrapS = FGLTFConverterUtility::ConvertWrap(AddressXY.Get<0>());
		JsonSampler.WrapT = FGLTFConverterUtility::ConvertWrap(AddressXY.Get<1>());
	}

	return Builder.AddSampler(JsonSampler);
}
