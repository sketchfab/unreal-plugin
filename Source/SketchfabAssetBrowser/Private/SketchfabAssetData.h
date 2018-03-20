// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "UObject/Class.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/Package.h"
#include "UObject/ObjectRedirector.h"
#include "Misc/PackageName.h"
#include "UObject/LinkerLoad.h"
#include "PrimaryAssetId.h"

#include "SketchfabAssetData.generated.h"

ASSETREGISTRY_API DECLARE_LOG_CATEGORY_EXTERN(LogSketchfabAssetData, Log, All);

USTRUCT()
struct FSketchfabAssetData
{
	GENERATED_BODY()
public:
	FName ModelUID;
	FName ContentFolder;
	FName AssetName;
	FName ThumbUID;
	FAssetDataTagMapSharedView TagsAndValues;

public:
	/** Default constructor */
	FSketchfabAssetData()
	{}

	/** Constructor */
	FSketchfabAssetData(FName InContentFolder, FName InAssetName, FName InModelUID, FName InThumbUID, FAssetDataTagMap InTags = FAssetDataTagMap())
		: ContentFolder(InContentFolder)
		, AssetName(InAssetName)
		, ModelUID(InModelUID)
		, ThumbUID(InThumbUID)
		, TagsAndValues(MoveTemp(InTags))
	{
	}

	/** FAssetDatas are equal if their object paths match */
	bool operator==(const FSketchfabAssetData& Other) const
	{
		return ModelUID == Other.ModelUID;
	}

	bool operator!=(const FSketchfabAssetData& Other) const
	{
		return ModelUID != Other.ModelUID;
	}

	bool operator>(const FSketchfabAssetData& Other) const
	{
		return ModelUID > Other.ModelUID;
	}

	bool operator<(const FSketchfabAssetData& Other) const
	{
		return ModelUID < Other.ModelUID;
	}

	/** Checks to see if this AssetData refers to an asset or is NULL */
	bool IsValid() const
	{
		return ModelUID != NAME_None;
	}

	/** Returns the full name for the asset in the form: Class ObjectPath */
	FString GetFullName() const
	{
		FString FullName;
		GetFullName(FullName);
		return FullName;
	}

	void GetFullName(FString& OutFullName) const
	{
		OutFullName.Reset();
		ContentFolder.AppendString(OutFullName);
		ModelUID.AppendString(OutFullName);
	}

	/** Returns the name for the asset in the form: Class'ObjectPath' */
	FString GetExportTextName() const
	{
		FString ExportTextName;
		GetExportTextName(ExportTextName);
		return ExportTextName;
	}

	/** Populates OutExportTextName with the name for the asset in the form: Class'ObjectPath' */
	void GetExportTextName(FString& OutExportTextName) const
	{
		OutExportTextName.Reset();
		OutExportTextName.AppendChar('\'');
		ModelUID.AppendString(OutExportTextName);
		OutExportTextName.AppendChar('\'');
	}

	/** Convert to a SoftObjectPath for loading */
	FSoftObjectPath ToSoftObjectPath() const
	{
		return FSoftObjectPath(ModelUID.ToString());
	}

	DEPRECATED(4.18, "ToStringReference was renamed to ToSoftObjectPath")
	FSoftObjectPath ToStringReference() const
	{
		return ToSoftObjectPath();
	}
	
	/** Gets primary asset id of this data */
	FPrimaryAssetId GetPrimaryAssetId() const
	{
		FName PrimaryAssetType, PrimaryAssetName;
		GetTagValueNameImpl(FPrimaryAssetId::PrimaryAssetTypeTag, PrimaryAssetType);
		GetTagValueNameImpl(FPrimaryAssetId::PrimaryAssetNameTag, PrimaryAssetName);

		if (PrimaryAssetType != NAME_None && PrimaryAssetName != NAME_None)
		{
			return FPrimaryAssetId(PrimaryAssetType, PrimaryAssetName);
		}

		return FPrimaryAssetId();
	}

	/** Returns the asset UObject if it is loaded or loads the asset if it is unloaded then returns the result */
	UObject* GetAsset() const
	{
		if ( !IsValid() )
		{
			// Dont even try to find the object if the objectpath isn't set
			return NULL;
		}

		UObject* Asset = FindObject<UObject>(NULL, *ModelUID.ToString());
		if ( Asset == NULL )
		{
			Asset = LoadObject<UObject>(NULL, *ModelUID.ToString());
		}

		return Asset;
	}

	/** Try and get the value associated with the given tag as a type converted value */
	template <typename ValueType>
	bool GetTagValue(const FName InTagName, ValueType& OutTagValue) const;

	/** Try and get the value associated with the given tag as a type converted value, or an empty value if it doesn't exist */
	template <typename ValueType>
	ValueType GetTagValueRef(const FName InTagName) const;

	/** Returns true if the asset is loaded */
	bool IsAssetLoaded() const
	{
		return IsValid() && FindObject<UObject>(NULL, *ModelUID.ToString()) != NULL;
	}

	/** Prints the details of the asset to the log */
	void PrintAssetData() const
	{
		UE_LOG(LogSketchfabAssetData, Log, TEXT("    FSketchfabAssetData for %s"), *ModelUID.ToString());
		UE_LOG(LogSketchfabAssetData, Log, TEXT("    ============================="));
		UE_LOG(LogSketchfabAssetData, Log, TEXT("        PackagePath: %s"), *ContentFolder.ToString());
		UE_LOG(LogSketchfabAssetData, Log, TEXT("        TagsAndValues: %d"), TagsAndValues.Num());
		for (const auto& TagValue: TagsAndValues)
		{
			UE_LOG(LogSketchfabAssetData, Log, TEXT("            %s : %s"), *TagValue.Key.ToString(), *TagValue.Value);
		}
	}

private:
	bool GetTagValueStringImpl(const FName InTagName, FString& OutTagValue) const
	{
		if (const FString* FoundValue = TagsAndValues.Find(InTagName))
		{
			bool bIsHandled = false;
			if (FTextStringHelper::IsComplexText(**FoundValue))
			{
				FText TmpText;
				if (FTextStringHelper::ReadFromString(**FoundValue, TmpText))
				{
					bIsHandled = true;
					OutTagValue = TmpText.ToString();
				}
			}

			if (!bIsHandled)
			{
				OutTagValue = *FoundValue;
			}

			return true;
		}

		return false;
	}

	bool GetTagValueTextImpl(const FName InTagName, FText& OutTagValue) const
	{
		if (const FString* FoundValue = TagsAndValues.Find(InTagName))
		{
			if (!FTextStringHelper::ReadFromString(**FoundValue, OutTagValue))
			{
				OutTagValue = FText::FromString(*FoundValue);
			}
			return true;
		}
		return false;
	}

	bool GetTagValueNameImpl(const FName InTagName, FName& OutTagValue) const
	{
		FString StrValue;
		if (GetTagValueStringImpl(InTagName, StrValue))
		{
			OutTagValue = *StrValue;
			return true;
		}
		return false;
	}
};


template<>
struct TStructOpsTypeTraits<FSketchfabAssetData> : public TStructOpsTypeTraitsBase2<FSketchfabAssetData>
{
	enum
	{
		WithIdenticalViaEquality = true
	};
};

template <typename ValueType>
inline bool FSketchfabAssetData::GetTagValue(const FName InTagName, ValueType& OutTagValue) const
{
	if (const FString* FoundValue = TagsAndValues.Find(InTagName))
	{
		FMemory::Memzero(&OutTagValue, sizeof(ValueType));
		Lex::FromString(OutTagValue, **FoundValue);
		return true;
	}
	return false;
}

template <>
inline bool FSketchfabAssetData::GetTagValue<FString>(const FName InTagName, FString& OutTagValue) const
{
	return GetTagValueStringImpl(InTagName, OutTagValue);
}

template <>
inline bool FSketchfabAssetData::GetTagValue<FText>(const FName InTagName, FText& OutTagValue) const
{
	return GetTagValueTextImpl(InTagName, OutTagValue);
}

template <>
inline bool FSketchfabAssetData::GetTagValue<FName>(const FName InTagName, FName& OutTagValue) const
{
	return GetTagValueNameImpl(InTagName, OutTagValue);
}

template <typename ValueType>
inline ValueType FSketchfabAssetData::GetTagValueRef(const FName InTagName) const
{
	ValueType TmpValue;
	FMemory::Memzero(&TmpValue, sizeof(ValueType));
	if (const FString* FoundValue = TagsAndValues.Find(InTagName))
	{
		Lex::FromString(TmpValue, **FoundValue);
	}
	return TmpValue;
}

template <>
inline FString FSketchfabAssetData::GetTagValueRef<FString>(const FName InTagName) const
{
	FString TmpValue;
	GetTagValueStringImpl(InTagName, TmpValue);
	return TmpValue;
}

template <>
inline FText FSketchfabAssetData::GetTagValueRef<FText>(const FName InTagName) const
{
	FText TmpValue;
	GetTagValueTextImpl(InTagName, TmpValue);
	return TmpValue;
}

template <>
inline FName FSketchfabAssetData::GetTagValueRef<FName>(const FName InTagName) const
{
	FName TmpValue;
	GetTagValueNameImpl(InTagName, TmpValue);
	return TmpValue;
}

/** A structure defining a thing that can be reference by something else in the asset registry. Represents either a package of a primary asset id */
struct FSketchfabAssetIdentifier
{
	/** The name of the package that is depended on, this is always set unless PrimaryAssetType is */
	FName PackageName;
	/** The primary asset type, if valid the ObjectName is the PrimaryAssetName */
	FPrimaryAssetType PrimaryAssetType;
	/** Specific object within a package. If empty, assumed to be the default asset */
	FName ObjectName;
	/** Name of specific value being referenced, if ObjectName specifies a type such as a UStruct */
	FName ValueName;

	/** Can be implicitly constructed from just the package name */
	FSketchfabAssetIdentifier(FName InPackageName, FName InObjectName = NAME_None, FName InValueName = NAME_None)
		: PackageName(InPackageName), PrimaryAssetType(NAME_None), ObjectName(InObjectName), ValueName(InValueName)
	{}

	/** Construct from a primary asset id */
	FSketchfabAssetIdentifier(const FPrimaryAssetId& PrimaryAssetId, FName InValueName = NAME_None)
		: PackageName(NAME_None), PrimaryAssetType(PrimaryAssetId.PrimaryAssetType), ObjectName(PrimaryAssetId.PrimaryAssetName), ValueName(InValueName)
	{}

	FSketchfabAssetIdentifier(UObject* SourceObject, FName InValueName)
	{
		if (SourceObject)
		{
			UPackage* Package = SourceObject->GetOutermost();
			PackageName = Package->GetFName();
			ObjectName = SourceObject->GetFName();
			ValueName = InValueName;
		}
	}

	FSketchfabAssetIdentifier()
		: PackageName(NAME_None), PrimaryAssetType(NAME_None), ObjectName(NAME_None), ValueName(NAME_None)
	{}

	/** Returns primary asset id for this identifier, if valid */
	FPrimaryAssetId GetPrimaryAssetId() const
	{
		if (PrimaryAssetType != NAME_None)
		{
			return FPrimaryAssetId(PrimaryAssetType, ObjectName);
		}
		return FPrimaryAssetId();
	}

	/** Returns true if this represents a package */
	bool IsPackage() const
	{
		return PackageName != NAME_None && !IsObject() && !IsValue();
	}

	/** Returns true if this represents an object, true for both package objects and PrimaryAssetId objects */
	bool IsObject() const
	{
		return ObjectName != NAME_None && !IsValue();
	}

	/** Returns true if this represents a specific value */
	bool IsValue() const
	{
		return ValueName != NAME_None;
	}

	/** Returns true if this is a valid non-null identifier */
	bool IsValid() const
	{
		return PackageName != NAME_None || GetPrimaryAssetId().IsValid();
	}

	/** Returns string version of this identifier in Package.Object::Name format */
	FString ToString() const
	{
		FString Result;
		if (PrimaryAssetType != NAME_None)
		{
			Result = GetPrimaryAssetId().ToString();
		}
		else
		{
			Result = PackageName.ToString();
			if (ObjectName != NAME_None)
			{
				Result += TEXT(".");
				Result += ObjectName.ToString();
			}
		}
		if (ValueName != NAME_None)
		{
			Result += TEXT("::");
			Result += ValueName.ToString();
		}
		return Result;
	}

	/** Converts from Package.Object::Name format */
	static FSketchfabAssetIdentifier FromString(const FString& String)
	{
		// To right of :: is value
		FString PackageString;
		FString ObjectString;
		FString ValueString;

		// Try to split value out
		if (!String.Split(TEXT("::"), &PackageString, &ValueString))
		{
			PackageString = String;
		}

		// Check if it's a valid primary asset id
		FPrimaryAssetId PrimaryId = FPrimaryAssetId::FromString(PackageString);

		if (PrimaryId.IsValid())
		{
			return USTRUCT()(PrimaryId, *ValueString);
		}

		// Try to split on first . , if it fails PackageString will stay the same
		FString(PackageString).Split(TEXT("."), &PackageString, &ObjectString);

		return FSketchfabAssetIdentifier(*PackageString, *ObjectString, *ValueString);
	}

	friend inline bool operator==(const FSketchfabAssetIdentifier& A, const FSketchfabAssetIdentifier& B)
	{
		return A.PackageName == B.PackageName && A.ObjectName == B.ObjectName && A.ValueName == B.ValueName;
	}

	friend inline uint32 GetTypeHash(const FSketchfabAssetIdentifier& Key)
	{
		uint32 Hash = 0;

		// Most of the time only packagename is set
		if (Key.ObjectName.IsNone() && Key.ValueName.IsNone())
		{
			return GetTypeHash(Key.PackageName);
		}

		Hash = HashCombine(Hash, GetTypeHash(Key.PackageName));
		Hash = HashCombine(Hash, GetTypeHash(Key.PrimaryAssetType));
		Hash = HashCombine(Hash, GetTypeHash(Key.ObjectName));
		Hash = HashCombine(Hash, GetTypeHash(Key.ValueName));
		return Hash;
	}

	/** Identifiers may be serialized as part of the registry cache, or in other contexts. If you make changes here you must also change FAssetRegistryVersion */
	friend FArchive& operator<<(FArchive& Ar, FSketchfabAssetIdentifier& AssetIdentifier)
	{
		// Serialize bitfield of which elements to serialize, in general many are empty
		uint8 FieldBits = 0;

		if (Ar.IsSaving())
		{
			FieldBits |= (AssetIdentifier.PackageName != NAME_None) << 0;
			FieldBits |= (AssetIdentifier.PrimaryAssetType != NAME_None) << 1;
			FieldBits |= (AssetIdentifier.ObjectName != NAME_None) << 2;
			FieldBits |= (AssetIdentifier.ValueName != NAME_None) << 3;
		}

		Ar << FieldBits;

		if (FieldBits & (1 << 0))
		{
			Ar << AssetIdentifier.PackageName;
		}
		if (FieldBits & (1 << 1))
		{
			FName TypeName = AssetIdentifier.PrimaryAssetType.GetName();
			Ar << TypeName;

			if (Ar.IsLoading())
			{
				AssetIdentifier.PrimaryAssetType = TypeName;
			}
		}
		if (FieldBits & (1 << 2))
		{
			Ar << AssetIdentifier.ObjectName;
		}
		if (FieldBits & (1 << 3))
		{
			Ar << AssetIdentifier.ValueName;
		}
		
		return Ar;
	}
};

