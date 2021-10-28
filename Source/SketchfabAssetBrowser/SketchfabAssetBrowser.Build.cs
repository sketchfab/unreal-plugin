// Copyright 2018 Sketchfab, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class SketchfabAssetBrowser : ModuleRules
	{
		public SketchfabAssetBrowser(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

			PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
				}
	                );

			PrivateIncludePaths.AddRange(
				new string[] {
					"SketchfabAssetBrowser/Private",
					"SketchfabAssetBrowser/Private/Commons",
					"SketchfabAssetBrowser/Private/AssetBrowser",
					"SketchfabAssetBrowser/Private/Exporter",
					"GLTFExporter/Private/Builders"
					// ... add other private include paths required here ...
				}
			);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"EditorStyle",
					"EditorScriptingUtilities",
					"UnrealEd",
					"InputCore",
					"SlateCore",
					"PropertyEditor",
					"Slate",
					"RawMesh",
					"GeometryCache",
					"MeshUtilities",
					"RenderCore",
					"RHI",
					"MessageLog",
					"JsonUtilities",
					//"ZipUtility", //https://github.com/GameLogicDesign/ZipUtility-ue4
					"SketchfabGLTF",
					"SKGLTFExporter",
					"EditorStyle",
					"Http",
					"Json",
					"JsonUtilities",
					"WebBrowser",
					"XmlParser",
					"ImageWrapper",
					"Projects",  // Support IPluginManager
				}
	                );

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					// ... add private dependencies that you statically link with here ...
		                }
			);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
			);

			PrivatePCHHeaderFile = "Private/SketchfabAssetBrowserPrivatePCH.h";
		}
	}
}
