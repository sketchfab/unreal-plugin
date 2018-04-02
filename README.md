## About

An Sketchfab GLTF and Asset Browser plugin for Unreal Engine. Compatible with 4.18 onwards. Utilizes the tinygltf library for loading in the glTF 2.0 file format: https://github.com/syoyo/tinygltf

## Installation

1. Open the Unreal Editor.
1. Choose New Project.
1. Choose one of the example project types, such as "First Person". You can use either BluePrint or C++, it does not matter.
1. Choose the location and name of your project.
1. Press Create Project.
1. On your computer go to the location where your project was created.
1. In your project folder create a new folder called "Plugins".
1. Download this unreal-exporter project from github and extract into this folder.
1. You should now have a folder structure which looks something like "MyProject/Plugins/SketchfabUnrealPlugin/..."
1. Shut down the Unreal Editor if its still running.
1. Double click the MyProject.uproject file to run Unreal Editor again. ("MyProject/MyProject.uproject")

## How To Import A glTF File

1. In the Unreal Editor go to the Content Browser at the bottom.
1. Select Import.
1. Choose a glTF 2.0 file to load (*.gltf or *.glb).
1. Press Open.

- Note: Objects are currently scaled up by a factor of 100.

## Shipping The PreBuilt Plugin

If you wish to give this plugin to someone to use, but not pass on the source code, then just make a copy of this project and remove the "Source" folder. The plugin will still work as normal without this folder.

## What Is Currently Supported

- Only imports the first scene found in the glTF file
- Loading in a glTF, or glb, file as a static mesh
- PBR materials
- Specular workflows are limited.
	- Specular textures are currently not being used. 
	- Nothing is currently attached to the scalar specular input on a material. 
	- Alpha channel of the Glossy map is inverted and used as roughness.
- Supports ascii *.gltf and binary .glb files.
- Supports external textures and textures embedded in binary files. 
	- Does not support textures embedded as ascii inside gltf files.
- No sampler support (ie uv tiling).

## What's Not Supported And Will Not Be In Version 1.0

- No cameras
- No animation
- No skins
- No joints
- No morph targets

## How To Use In Visual Studio

1. First follow the same Installation steps above, but make sure to create a C++ project.
1. Open the Visual Studio solution file your project folder.
1. In your VS Project add folders under Plugins called "SketchfabAssetBrowser" and "SketchfabGLTF"
1. Under those add new folders called Private and Public.
1. Add all the files from the SketchfabUnrealPlugin/source/SketchfabAssetBrowser/Private to the corresponding private folder in VS.
1. Add all the files from the SketchfabUnrealPlugin/source/SketchfabAssetBrowser/Public to the corresponding public folder in VS.
1. Do the same for the SketchfabGLTF folder.
1. Make sure your configuration is set to DebugGame Editor.
1. Do a full recompile of your project.
1. Set a break point in the StaticMeshImporter.cpp -> FGLTFStaticMeshImporter::ImportStaticMesh(...) method.
1. Run the project and import a glTF file to hit the break point.

## Note About the ZipUtility module
This plugin includes a modified version of the ZipUtility plugin. The modifications made were to ensure the function calls are synchronous. 

The original project can be found here: https://github.com/getnamo/ZipUtility-ue4

