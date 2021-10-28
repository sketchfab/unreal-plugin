// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

template <typename OutputType, typename... InputTypes>
class TGLTFConverter
{
	typedef TTuple<InputTypes...> InputKeyType;

public:

	virtual ~TGLTFConverter() = default;

	OutputType Get(InputTypes&&... Inputs) const
	{
		Sanitize(Inputs...);
		const InputKeyType InputKey(Forward<InputTypes>(Inputs)...);
		if (OutputType* SavedOutput = SavedOutputs.Find(InputKey))
		{
			return *SavedOutput;
		}

		return {};
	}

	OutputType Add(InputTypes... Inputs)
	{
		Sanitize(Inputs...);
		const InputKeyType InputKey(Inputs...);
		OutputType NewOutput = Convert(Forward<InputTypes>(Inputs)...);

		SavedOutputs.Add(InputKey, NewOutput);
		return NewOutput;
	}

	OutputType GetOrAdd(InputTypes... Inputs)
	{
		Sanitize(Inputs...);
		const InputKeyType InputKey(Inputs...);
		if (OutputType* SavedOutput = SavedOutputs.Find(InputKey))
		{
			return *SavedOutput;
		}

		OutputType NewOutput = Convert(Forward<InputTypes>(Inputs)...);

		SavedOutputs.Add(InputKey, NewOutput);
		return NewOutput;
	}

protected:

	virtual void Sanitize(InputTypes&... Inputs) { }

	virtual OutputType Convert(InputTypes... Inputs) = 0;

private:

	TMap<InputKeyType, OutputType> SavedOutputs;
};
