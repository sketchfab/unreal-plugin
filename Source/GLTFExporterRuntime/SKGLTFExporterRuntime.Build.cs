// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class SKGLTFExporterRuntime : ModuleRules
	{
		public SKGLTFExporterRuntime(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.NoPCHs;
			bPrecompile = true;

			PublicIncludePaths.AddRange(
				new string[] {
				}
				);

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
