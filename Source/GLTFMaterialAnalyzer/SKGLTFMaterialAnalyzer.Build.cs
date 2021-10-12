// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SKGLTFMaterialAnalyzer : ModuleRules
{
	public SKGLTFMaterialAnalyzer(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"RenderCore"
			}
		);

		PublicIncludePaths.AddRange(
				new string[] {
					"Editor/",
					"Runtime/",
					EngineDirectory + "/Shaders/Shared/"
				}

			);

		// NOTE: ugly hack to access HLSLMaterialTranslator to analyze materials
		PrivateIncludePaths.Add(EngineDirectory + "/Source/Runtime/Engine/Private");
		PrivateIncludePaths.Add(EngineDirectory + "/Source/Runtime/Engine/Private/Materials");

		PublicIncludePaths.Add(EngineDirectory + "/Shaders/Shared/");
		PrivateIncludePaths.Add(EngineDirectory + "/Shaders/Shared/");
	}
}
