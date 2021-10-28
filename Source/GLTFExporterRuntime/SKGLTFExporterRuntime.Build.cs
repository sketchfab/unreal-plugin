// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class SKGLTFExporterRuntime : ModuleRules
	{
		public SKGLTFExporterRuntime(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.NoPCHs;
			bPrecompile = true;

			PublicIncludePaths.Add(
				EngineDirectory + "/Shaders/Shared/"
				);

			PublicIncludePaths.AddRange(
				new string[] {
					"Editor/",
					"Runtime/",
					EngineDirectory + "/Shaders/Shared/"
				}

			);

			PublicIncludePaths.Add(EngineDirectory + "/Shaders/Shared/");
			PrivateIncludePaths.Add(EngineDirectory + "/Shaders/Shared/");

			PrivateIncludePaths.AddRange(
				new string[] {
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"InputCore",
					"LevelSequence",
					"MovieScene"
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
				}
				);
		}
	}
}
