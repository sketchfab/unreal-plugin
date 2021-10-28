// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class SKGLTFExporter : ModuleRules
	{
		public SKGLTFExporter(ReadOnlyTargetRules Target) : base(Target)
		{

			PublicIncludePaths.AddRange(
				new string[] {
					"Editor/",
					"Runtime/",
					EngineDirectory + "/Shaders/Shared/"
				}
				
			);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"UnrealEd",
					"MeshDescription",
					"StaticMeshDescription",
					"MeshUtilities",
					"MessageLog",
					"Json",
					"MaterialEditor",
					"Slate",
					"SlateCore",
					"Mainframe",
					"InputCore",
					"EditorStyle",
					"Projects",
					"RenderCore",
					"RHI",
					"DesktopPlatform",
					"LevelSequence",
					"MovieScene",
					"MovieSceneTracks",
					"VariantManagerContent",
					"MeshMergeUtilities",
					"zlib",
					"SKGLTFMaterialBaking",
					"SKGLTFMaterialAnalyzer"
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"SKGLTFExporterRuntime",
					"zlib"
				}
			);

			if (Target.Platform == UnrealTargetPlatform.Linux)
			{
				PublicAdditionalLibraries.Add("/usr/lib/x86_64-linux-gnu/libminizip.so");
			}
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				PublicAdditionalLibraries.Add("/usr/local/lib/libminizip.dylib");
			}
        }
	}
}
