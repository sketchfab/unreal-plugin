// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"

struct FGLTFActorUtility
{
	static bool IsRootActor(const AActor* Actor, bool bSelectedOnly);

	static UBlueprint* GetBlueprintFromActor(const AActor* Actor);

	static bool IsSkySphereBlueprint(const UBlueprint* Blueprint);

	static bool IsHDRIBackdropBlueprint(const UBlueprint* Blueprint);

	template <class ValueType>
	static bool TryGetPropertyValue(const UObject* Object, const TCHAR* PropertyName, ValueType& Value)
	{
		FProperty* Property = Object->GetClass()->FindPropertyByName(PropertyName);
		if (Property == nullptr)
		{
			return false;
		}

		const ValueType* ValuePtr = Property->ContainerPtrToValuePtr<ValueType>(Object);
		if (ValuePtr == nullptr)
		{
			return false;
		}

		Value = *ValuePtr;
		return true;
	}
};
