// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
public class SKGLTFMaterialBaking : ModuleRules
{
	public SKGLTFMaterialBaking(ReadOnlyTargetRules Target) : base(Target)
	{
        PrivateDependencyModuleNames.AddRange(
			new string [] {
				"Core",
				"CoreUObject",
				"Engine",
				"RenderCore",
                "RHI",                
                "UnrealEd",
                "MainFrame",
                "SlateCore",
                "Slate",
                "InputCore",
                "PropertyEditor",
                "EditorStyle",
                "Renderer",
                "MeshDescription",
				"StaticMeshDescription"
            }
		);

		// NOTE: ugly hack to access HLSLMaterialTranslator to bake shading model
        PrivateIncludePaths.Add(EngineDirectory + "/Source/Runtime/Engine/Private");

		// NOTE: avoid renaming all instaces of SKMATERIALBAKING_API by redirecting it to SKGLTFMATERIALBAKING_API
		PublicDefinitions.Add("SKMATERIALBAKING_API=SKGLTFMATERIALBAKING_API");

        PublicIncludePaths.Add(EngineDirectory + "/Shaders/Shared/");
        PrivateIncludePaths.Add(EngineDirectory + "/Shaders/Shared/");

    }
}
