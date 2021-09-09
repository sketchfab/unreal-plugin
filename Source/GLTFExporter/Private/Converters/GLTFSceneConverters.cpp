// Copyright Epic Games, Inc. All Rights Reserved.

#include "Converters/SKGLTFSceneConverters.h"
#include "Builders/SKGLTFContainerBuilder.h"
#include "Converters/SKGLTFConverterUtility.h"
#include "Converters/SKGLTFActorUtility.h"
#include "LevelVariantSetsActor.h"

FGLTFJsonSceneIndex FGLTFSceneConverter::Convert(const UWorld* World)
{
	FGLTFJsonScene Scene;
	World->GetName(Scene.Name);

	for (int32 Index = 0; Index < World->GetNumLevels(); ++Index)
	{
		ULevel* Level = World->GetLevel(Index);
		if (Level == nullptr)
		{
			continue;
		}

		// TODO: export Level->Model ?

		for (const AActor* Actor : Level->Actors)
		{
			// TODO: should a LevelVariantSet be exported even if not selected for export?
			if (const ALevelVariantSetsActor *LevelVariantSetsActor = Cast<ALevelVariantSetsActor>(Actor))
			{
				if (Builder.ExportOptions->bExportVariantSets)
				{
					if (const ULevelVariantSets* LevelVariantSets = const_cast<ALevelVariantSetsActor*>(LevelVariantSetsActor)->GetLevelVariantSets(true))
					{
						const FGLTFJsonVariationIndex VariationIndex = Builder.GetOrAddVariation(LevelVariantSets);
						if (VariationIndex != INDEX_NONE)
						{
							Scene.Variations.Add(VariationIndex);
						}
					}
				}
			}

			const FGLTFJsonNodeIndex NodeIndex = Builder.GetOrAddNode(Actor);
			if (NodeIndex != INDEX_NONE && FGLTFActorUtility::IsRootActor(Actor, Builder.bSelectedActorsOnly))
			{
				// TODO: to avoid having to add irrelevant actors/components let GLTFComponentConverter decide and add root nodes to scene. This change may require node converters to support cyclic calls.
				Scene.Nodes.Add(NodeIndex);
			}
		}
	}

	return Builder.AddScene(Scene);
}
