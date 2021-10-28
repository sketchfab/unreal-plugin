// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Json/SKGLTFJsonIndex.h"
#include "Converters/SKGLTFConverter.h"
#include "Converters/SKGLTFBuilderContext.h"
#include "Converters/SKGLTFMeshSection.h"
#include "Engine.h"

template <typename... InputTypes>
class TGLTFAccessorConverter : public FGLTFBuilderContext, public TGLTFConverter<FGLTFJsonAccessorIndex, InputTypes...>
{
	using FGLTFBuilderContext::FGLTFBuilderContext;
};

class FGLTFPositionBufferConverter final : public TGLTFAccessorConverter<const FGLTFMeshSection*, const FPositionVertexBuffer*>
{
	using TGLTFAccessorConverter::TGLTFAccessorConverter;

	virtual FGLTFJsonAccessorIndex Convert(const FGLTFMeshSection* MeshSection, const FPositionVertexBuffer* VertexBuffer) override;
};

class FGLTFColorBufferConverter final : public TGLTFAccessorConverter<const FGLTFMeshSection*, const FColorVertexBuffer*>
{
	using TGLTFAccessorConverter::TGLTFAccessorConverter;

	virtual FGLTFJsonAccessorIndex Convert(const FGLTFMeshSection* MeshSection, const FColorVertexBuffer* VertexBuffer) override;
};

class FGLTFNormalBufferConverter final : public TGLTFAccessorConverter<const FGLTFMeshSection*, const FStaticMeshVertexBuffer*>
{
	using TGLTFAccessorConverter::TGLTFAccessorConverter;

	virtual FGLTFJsonAccessorIndex Convert(const FGLTFMeshSection* MeshSection, const FStaticMeshVertexBuffer* VertexBuffer) override;

	template <typename DestinationType, typename SourceType>
	FGLTFJsonBufferViewIndex ConvertBufferView(const FGLTFMeshSection* MeshSection, const FStaticMeshVertexBuffer* VertexBuffer);
};

class FGLTFTangentBufferConverter final : public TGLTFAccessorConverter<const FGLTFMeshSection*, const FStaticMeshVertexBuffer*>
{
	using TGLTFAccessorConverter::TGLTFAccessorConverter;

	virtual FGLTFJsonAccessorIndex Convert(const FGLTFMeshSection* MeshSection, const FStaticMeshVertexBuffer* VertexBuffer) override;

	template <typename DestinationType, typename SourceType>
	FGLTFJsonBufferViewIndex ConvertBufferView(const FGLTFMeshSection* MeshSection, const FStaticMeshVertexBuffer* VertexBuffer);
};

class FGLTFUVBufferConverter final : public TGLTFAccessorConverter<const FGLTFMeshSection*, const FStaticMeshVertexBuffer*, uint32>
{
	using TGLTFAccessorConverter::TGLTFAccessorConverter;

	virtual FGLTFJsonAccessorIndex Convert(const FGLTFMeshSection* MeshSection, const FStaticMeshVertexBuffer* VertexBuffer, uint32 UVIndex) override;
};

class FGLTFBoneIndexBufferConverter final : public TGLTFAccessorConverter<const FGLTFMeshSection*, const FSkinWeightVertexBuffer*, uint32>
{
	using TGLTFAccessorConverter::TGLTFAccessorConverter;

	virtual FGLTFJsonAccessorIndex Convert(const FGLTFMeshSection* MeshSection, const FSkinWeightVertexBuffer* VertexBuffer, uint32 InfluenceOffset) override;

	template <typename IndexType>
	FGLTFJsonAccessorIndex Convert(const FGLTFMeshSection* MeshSection, const FSkinWeightVertexBuffer* VertexBuffer, uint32 InfluenceOffset) const;
};

class FGLTFBoneWeightBufferConverter final : public TGLTFAccessorConverter<const FGLTFMeshSection*, const FSkinWeightVertexBuffer*, uint32>
{
	using TGLTFAccessorConverter::TGLTFAccessorConverter;

	virtual FGLTFJsonAccessorIndex Convert(const FGLTFMeshSection* MeshSection, const FSkinWeightVertexBuffer* VertexBuffer, uint32 InfluenceOffset) override;
};

class FGLTFIndexBufferConverter final : public TGLTFAccessorConverter<const FGLTFMeshSection*>
{
	using TGLTFAccessorConverter::TGLTFAccessorConverter;

	virtual FGLTFJsonAccessorIndex Convert(const FGLTFMeshSection* MeshSection) override;

	template <typename IndexType>
	FGLTFJsonAccessorIndex Convert(const FGLTFMeshSection* MeshSection) const;
};
