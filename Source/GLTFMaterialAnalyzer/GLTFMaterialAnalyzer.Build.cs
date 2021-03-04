// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GLTFMaterialAnalyzer : ModuleRules
{
	public GLTFMaterialAnalyzer(ReadOnlyTargetRules Target) : base(Target)
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

		// NOTE: ugly hack to access HLSLMaterialTranslator to analyze materials
		PrivateIncludePaths.Add(EngineDirectory + "/Source/Runtime/Engine/Private");
	}
}
