// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class GLTFExporter : ModuleRules
	{
		public GLTFExporter(ReadOnlyTargetRules Target) : base(Target)
		{
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
					"GLTFMaterialBaking",
					"GLTFMaterialAnalyzer"
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"GLTFExporterRuntime"
				}
				);
		}
	}
}
