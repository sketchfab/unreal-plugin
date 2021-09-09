// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFVarationConverters.h"
#include "Converters/SKGLTFMeshUtility.h"
#include "Builders/SKGLTFContainerBuilder.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "VariantObjectBinding.h"
#include "PropertyValueMaterial.h"
#include "LevelVariantSets.h"
#include "PropertyValue.h"
#include "VariantSet.h"
#include "Variant.h"

namespace
{
	// TODO: replace hack with safe solution by adding proper API access to UPropertyValue
	class UPropertyValueHack : public UPropertyValue
	{
	public:
		const TArray<FCapturedPropSegment>& GetCapturedPropSegments() const
		{
			return CapturedPropSegments;
		}
	};
}

FGLTFJsonVariationIndex FGLTFVariationConverter::Convert(const ULevelVariantSets* LevelVariantSets)
{
	FGLTFJsonVariation JsonVariation;
	LevelVariantSets->GetName(JsonVariation.Name);

	for (const UVariantSet* VariantSet: LevelVariantSets->GetVariantSets())
	{
		FGLTFJsonVariantSet JsonVariantSet;
		JsonVariantSet.Name = VariantSet->GetDisplayText().ToString();

		for (const UVariant* Variant: VariantSet->GetVariants())
		{
			FGLTFJsonVariant JsonVariant;
			if (TryParseVariant(JsonVariant, Variant))
			{
				JsonVariantSet.Variants.Add(JsonVariant);
			}
		}

		if (JsonVariantSet.Variants.Num() > 0)
		{
			JsonVariation.VariantSets.Add(JsonVariantSet);
		}
	}

	if (JsonVariation.VariantSets.Num() == 0)
	{
		return FGLTFJsonVariationIndex(INDEX_NONE);
	}

	return Builder.AddVariation(JsonVariation);
}

bool FGLTFVariationConverter::TryParseVariant(FGLTFJsonVariant& OutVariant, const UVariant* Variant) const
{
	FGLTFJsonVariant JsonVariant;

	for (const UVariantObjectBinding* Binding: Variant->GetBindings())
	{
		TryParseVariantBinding(JsonVariant, Binding);
	}

	if (JsonVariant.Nodes.Num() == 0)
	{
		return false;
	}

	JsonVariant.Name = Variant->GetDisplayText().ToString();
	JsonVariant.bIsActive = const_cast<UVariant*>(Variant)->IsActive();

	if (const UTexture2D* Thumbnail = const_cast<UVariant*>(Variant)->GetThumbnail())
	{
		// TODO: if thumbnail has generic name "Texture2D", give it a variant-relevant name
		JsonVariant.Thumbnail = Builder.GetOrAddTexture(Thumbnail);
	}

	OutVariant = JsonVariant;
	return true;
}

bool FGLTFVariationConverter::TryParseVariantBinding(FGLTFJsonVariant& OutVariant, const UVariantObjectBinding* Binding) const
{
	bool bHasParsedAnyProperty = false;

	for (UPropertyValue* Property: Binding->GetCapturedProperties())
	{
		if (!const_cast<UPropertyValue*>(Property)->Resolve() || !Property->HasRecordedData())
		{
			continue;
		}

		const FName PropertyName = Property->GetPropertyName();
		const FFieldClass* PropertyClass = Property->GetPropertyClass();

		if (Property->IsA<UPropertyValueMaterial>())
		{
			if (Builder.ExportOptions->ExportMaterialVariants != ESKGLTFMaterialVariantMode::None && TryParseMaterialPropertyValue(OutVariant, Property))
			{
				bHasParsedAnyProperty = true;
			}
		}
		else if (PropertyName == TEXT("StaticMesh")) // TODO: should we not also check PropertyClass?
		{
			if (Builder.ExportOptions->bExportMeshVariants && TryParseStaticMeshPropertyValue(OutVariant, Property))
			{
				bHasParsedAnyProperty = true;
			}
		}
		else if (PropertyName == TEXT("SkeletalMesh")) // TODO: should we not also check PropertyClass?
		{
			if (Builder.ExportOptions->bExportMeshVariants && TryParseSkeletalMeshPropertyValue(OutVariant, Property))
			{
				bHasParsedAnyProperty = true;
			}
		}
		else if (PropertyName == TEXT("bVisible") && PropertyClass->IsChildOf(FBoolProperty::StaticClass()))
		{
			if (Builder.ExportOptions->bExportVisibilityVariants && TryParseVisibilityPropertyValue(OutVariant, Property))
			{
				bHasParsedAnyProperty = true;
			}
		}

		// TODO: add support for more properties
	}

	return bHasParsedAnyProperty;
}

bool FGLTFVariationConverter::TryParseVisibilityPropertyValue(FGLTFJsonVariant& OutVariant, const UPropertyValue* Property) const
{
	const USceneComponent* Target = static_cast<USceneComponent*>(Property->GetPropertyParentContainerAddress());
	if (Target == nullptr)
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Target object for property is invalid, the property will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	const AActor* Owner = Target->GetOwner();
	if (Owner == nullptr)
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Invalid scene component, the property will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	if (Builder.bSelectedActorsOnly && !Owner->IsSelected())
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Target actor for property is not selected for export, the property will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	bool bIsVisible;

	if (!TryGetPropertyValue(const_cast<UPropertyValue*>(Property), bIsVisible))
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Failed to parse recorded data for property, it will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	const FGLTFJsonNodeIndex NodeIndex = Builder.GetOrAddNode(Target);
	const FGLTFJsonNodeIndex ComponentNodeIndex = Builder.GetComponentNodeIndex(NodeIndex);

	FGLTFJsonVariantNodeProperties& NodeProperties = OutVariant.Nodes.FindOrAdd(ComponentNodeIndex);

	NodeProperties.Node = ComponentNodeIndex;
	NodeProperties.bIsVisible = bIsVisible;
	return true;
}

bool FGLTFVariationConverter::TryParseMaterialPropertyValue(FGLTFJsonVariant& OutVariant, const UPropertyValue* Property) const
{
	UPropertyValueMaterial* MaterialProperty = Cast<UPropertyValueMaterial>(const_cast<UPropertyValue*>(Property));
	if (MaterialProperty == nullptr)
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Material property is invalid, the property will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	const USceneComponent* Target = static_cast<USceneComponent*>(Property->GetPropertyParentContainerAddress());
	if (Target == nullptr)
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Target object for property is invalid, the property will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	const AActor* Owner = Target->GetOwner();
	if (Owner == nullptr)
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Invalid scene component, the property will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	if (Builder.bSelectedActorsOnly && !Owner->IsSelected())
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Target actor for property is not selected for export, the property will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	const TArray<FCapturedPropSegment>& CapturedPropSegments = reinterpret_cast<UPropertyValueHack*>(MaterialProperty)->GetCapturedPropSegments();
	const int32 NumPropSegments = CapturedPropSegments.Num();

	if (NumPropSegments < 1)
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Failed to parse element index to apply the material to, the property will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	// NOTE: UPropertyValueMaterial::GetMaterial does *not* ensure that the recorded data has been loaded,
	// so we need to call UProperty::GetRecordedData first to make that happen.
	MaterialProperty->GetRecordedData();

	// TODO: find way to determine whether the material is null because "None" was selected, or because it failed to resolve
	const UMaterialInterface* Material = MaterialProperty->GetMaterial();
	const int32 MaterialIndex = CapturedPropSegments[NumPropSegments - 1].PropertyIndex;

	const FGLTFMeshData* MeshData = nullptr;
	FGLTFIndexArray SectionIndices;

	if (Builder.ExportOptions->ExportMaterialVariants == ESKGLTFMaterialVariantMode::UseMeshData)
	{
		if (Builder.ExportOptions->BakeMaterialInputs == ESKGLTFMaterialBakeMode::UseMeshData)
		{
			const int32 DefaultLODIndex = Builder.ExportOptions->DefaultLevelOfDetail;

			if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(Target))
			{
				const UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();
				const int32 LODIndex = FGLTFMeshUtility::GetLOD(StaticMesh, StaticMeshComponent, DefaultLODIndex);
				const FStaticMeshLODResources& MeshLOD = StaticMesh->GetLODForExport(LODIndex);

				SectionIndices = FGLTFMeshUtility::GetSectionIndices(MeshLOD, MaterialIndex);
				MeshData = Builder.StaticMeshDataConverter.GetOrAdd(StaticMesh, StaticMeshComponent, LODIndex);
			}
			else if (const USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Target))
			{
				const USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->SkeletalMesh;
				const int32 LODIndex = FGLTFMeshUtility::GetLOD(SkeletalMesh, SkeletalMeshComponent, DefaultLODIndex);
				const FSkeletalMeshRenderData* RenderData = SkeletalMesh->GetResourceForRendering();
				const FSkeletalMeshLODRenderData& MeshLOD = RenderData->LODRenderData[LODIndex];

				SectionIndices = FGLTFMeshUtility::GetSectionIndices(MeshLOD, MaterialIndex);
				MeshData = Builder.SkeletalMeshDataConverter.GetOrAdd(SkeletalMesh, SkeletalMeshComponent, LODIndex);
			}
		}
		else
		{
			// TODO: report warning (about materials won't be export using mesh data because BakeMaterialInputs is not set to UseMeshData)
		}
	}

	FGLTFJsonVariantMaterial VariantMaterial;
	VariantMaterial.Material = Builder.GetOrAddMaterial(Material, MeshData, SectionIndices);
	VariantMaterial.Index = MaterialIndex;

	const FGLTFJsonNodeIndex NodeIndex = Builder.GetOrAddNode(Target);
	const FGLTFJsonNodeIndex ComponentNodeIndex = Builder.GetComponentNodeIndex(NodeIndex);
	FGLTFJsonVariantNodeProperties& NodeProperties = OutVariant.Nodes.FindOrAdd(ComponentNodeIndex);

	NodeProperties.Node = ComponentNodeIndex;
	NodeProperties.Materials.Add(VariantMaterial);
	return true;
}

bool FGLTFVariationConverter::TryParseStaticMeshPropertyValue(FGLTFJsonVariant& OutVariant, const UPropertyValue* Property) const
{
	const USceneComponent* Target = static_cast<USceneComponent*>(Property->GetPropertyParentContainerAddress());
	if (Target == nullptr)
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Target object for property is invalid, the property will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	const AActor* Owner = Target->GetOwner();
	if (Owner == nullptr)
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Invalid scene component, the property will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	if (Builder.bSelectedActorsOnly && !Owner->IsSelected())
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Target actor for property is not selected for export, the property will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	const UMeshComponent* MeshComponent = Cast<UMeshComponent>(Target);
	if (MeshComponent == nullptr)
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Target object for property has no mesh-component, the property will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	const UStaticMesh* StaticMesh;

	if (!TryGetPropertyValue(const_cast<UPropertyValue*>(Property), StaticMesh))
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Failed to parse recorded data for property, it will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	const FGLTFMaterialArray OverrideMaterials(MeshComponent->OverrideMaterials);
	const FGLTFJsonNodeIndex NodeIndex = Builder.GetOrAddNode(Target);
	const FGLTFJsonNodeIndex ComponentNodeIndex = Builder.GetComponentNodeIndex(NodeIndex);
	const FGLTFJsonMeshIndex MeshIndex = Builder.GetOrAddMesh(StaticMesh, OverrideMaterials);
	FGLTFJsonVariantNodeProperties& NodeProperties = OutVariant.Nodes.FindOrAdd(ComponentNodeIndex);

	NodeProperties.Node = ComponentNodeIndex;
	NodeProperties.Mesh = MeshIndex;
	return true;
}

bool FGLTFVariationConverter::TryParseSkeletalMeshPropertyValue(FGLTFJsonVariant& OutVariant, const UPropertyValue* Property) const
{
	const USceneComponent* Target = static_cast<USceneComponent*>(Property->GetPropertyParentContainerAddress());
	if (Target == nullptr)
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Target object for property is invalid, the property will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	const AActor* Owner = Target->GetOwner();
	if (Owner == nullptr)
	{
		Builder.AddWarningMessage(FString::Printf(TEXT("Invalid scene component, the property will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	if (Builder.bSelectedActorsOnly && !Owner->IsSelected())
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Target actor for property is not selected for export, the property will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	const UMeshComponent* MeshComponent = Cast<UMeshComponent>(Target);
	if (MeshComponent == nullptr)
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Target object for property has no mesh-component, the property will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	const USkeletalMesh* SkeletalMesh;

	if (!TryGetPropertyValue(const_cast<UPropertyValue*>(Property), SkeletalMesh) || SkeletalMesh == nullptr)
	{
		Builder.AddWarningMessage(FString::Printf(
			TEXT("Failed to parse recorded data for property, it will be skipped. Context: %s"),
			*GetLogContext(Property)));
		return false;
	}

	const FGLTFMaterialArray OverrideMaterials(MeshComponent->OverrideMaterials);
	const FGLTFJsonNodeIndex NodeIndex = Builder.GetOrAddNode(Target);
	const FGLTFJsonMeshIndex MeshIndex = Builder.GetOrAddMesh(SkeletalMesh, OverrideMaterials);
	const FGLTFJsonNodeIndex ComponentNodeIndex = Builder.GetComponentNodeIndex(NodeIndex);
	FGLTFJsonVariantNodeProperties& NodeProperties = OutVariant.Nodes.FindOrAdd(ComponentNodeIndex);

	NodeProperties.Node = ComponentNodeIndex;
	NodeProperties.Mesh = MeshIndex;
	return true;
}

template<typename T>
bool FGLTFVariationConverter::TryGetPropertyValue(UPropertyValue* Property, T& OutValue) const
{
	if (Property == nullptr || !Property->HasRecordedData())
	{
		return false;
	}

	FMemory::Memcpy(&OutValue, Property->GetRecordedData().GetData(), sizeof(T));
	return true;
}

FString FGLTFVariationConverter::GetLogContext(const UPropertyValue* Property) const
{
	const UVariantObjectBinding* Parent = Property->GetParent();
	return GetLogContext(Parent) + TEXT("/") + Property->GetFullDisplayString();
}

FString FGLTFVariationConverter::GetLogContext(const UVariantObjectBinding* Binding) const
{
	const UVariant* Parent = const_cast<UVariantObjectBinding*>(Binding)->GetParent();
	return GetLogContext(Parent) + TEXT("/") + Binding->GetDisplayText().ToString();
}

FString FGLTFVariationConverter::GetLogContext(const UVariant* Variant) const
{
	const UVariantSet* Parent = const_cast<UVariant*>(Variant)->GetParent();
	return GetLogContext(Parent) + TEXT("/") + Variant->GetDisplayText().ToString();
}

FString FGLTFVariationConverter::GetLogContext(const UVariantSet* VariantSet) const
{
	const ULevelVariantSets* Parent = const_cast<UVariantSet*>(VariantSet)->GetParent();
	return GetLogContext(Parent) + TEXT("/") + VariantSet->GetDisplayText().ToString();
}

FString FGLTFVariationConverter::GetLogContext(const ULevelVariantSets* LevelVariantSets) const
{
	return LevelVariantSets->GetName();
}
