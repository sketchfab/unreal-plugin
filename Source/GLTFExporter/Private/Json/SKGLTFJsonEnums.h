// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

enum class ESKGLTFJsonExtension
{
	KHR_LightsPunctual,
	KHR_MaterialsClearCoat,
	KHR_MaterialsUnlit,
	KHR_MeshQuantization,
	KHR_TextureTransform,
	EPIC_AnimationHotspots,
	EPIC_AnimationPlayback,
	EPIC_BlendModes,
	EPIC_CameraControls,
	EPIC_HDRIBackdrops,
	EPIC_LevelVariantSets,
	EPIC_LightmapTextures,
	EPIC_SkySpheres,
	EPIC_TextureHDREncoding
};

enum class ESKGLTFJsonShadingModel
{
	None = -1,
	Default,
	Unlit,
	ClearCoat
};

enum class ESKGLTFJsonHDREncoding
{
	None = -1,
	RGBM,
	RGBE
};

enum class ESKGLTFJsonCubeFace
{
	None = -1,
	PosX,
	NegX,
	PosY,
	NegY,
	PosZ,
	NegZ
};

enum class ESKGLTFJsonAccessorType
{
	None = -1,
	Scalar,
	Vec2,
	Vec3,
	Vec4,
	Mat2,
	Mat3,
	Mat4
};

enum class ESKGLTFJsonComponentType
{
	None = -1,
	S8 = 5120, // signed byte
	U8 = 5121, // unsigned byte
	S16 = 5122, // signed short
	U16 = 5123, // unsigned short
	S32 = 5124, // unused
	U32 = 5125, // unsigned int -- only valid for indices, not attributes
	F32 = 5126  // float
};

enum class ESKGLTFJsonBufferTarget
{
	None = -1,
	ArrayBuffer = 34962,
	ElementArrayBuffer = 34963
};

enum class ESKGLTFJsonPrimitiveMode
{
	// unsupported
	Points = 0,
	Lines = 1,
	LineLoop = 2,
	LineStrip = 3,
	// initially supported
	Triangles = 4,
	// will be supported prior to release
	TriangleStrip = 5,
	TriangleFan = 6
};

enum class ESKGLTFJsonAlphaMode
{
	None = -1,
	Opaque,
	Mask,
	Blend
};

enum class ESKGLTFJsonBlendMode
{
	None = -1,
	Additive,
	Modulate,
	AlphaComposite
};

enum class ESKGLTFJsonMimeType
{
	None = -1,
	PNG,
	JPEG
};

enum class ESKGLTFJsonTextureFilter
{
	None = -1,
	// valid for Min & Mag
	Nearest = 9728,
	Linear = 9729,
	// valid for Min only
	NearestMipmapNearest = 9984,
	LinearMipmapNearest = 9985,
	NearestMipmapLinear = 9986,
	LinearMipmapLinear = 9987
};

enum class ESKGLTFJsonTextureWrap
{
	None = -1,
	Repeat = 10497,
	MirroredRepeat = 33648,
	ClampToEdge = 33071
};

enum class ESKGLTFJsonInterpolation
{
	None = -1,
	Linear,
	Step,
	CubicSpline,
};

enum class ESKGLTFJsonTargetPath
{
	None = -1,
	Translation,
	Rotation,
	Scale,
	Weights
};

enum class ESKGLTFJsonCameraType
{
	None = -1,
	Orthographic,
	Perspective
};

enum class ESKGLTFJsonLightType
{
	None = -1,
	Directional,
	Point,
	Spot
};

enum class ESKGLTFJsonCameraControlMode
{
	None = -1,
	FreeLook,
	Orbital
};
