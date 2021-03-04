// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
public class GLTFMaterialBaking : ModuleRules
{
	public GLTFMaterialBaking(ReadOnlyTargetRules Target) : base(Target)
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

		// NOTE: avoid renaming all instaces of MATERIALBAKING_API by redirecting it to GLTFMATERIALBAKING_API
		PublicDefinitions.Add("MATERIALBAKING_API=GLTFMATERIALBAKING_API");
    }
}
