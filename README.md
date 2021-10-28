"""
* Update images
"""

# Sketchfab plugin for Unreal Engine

**Unreal Engine plugin to export and import models to and from Sketchfab**

* [Installation](#installation)
* [Authentification](#authentification)
* [Importing models](#download-a-model-from-sketchfab)
* [Downloading models](#download-a-model-from-sketchfab)
* [Limitations](#known-issues-and-limitations)


## Installation

*Please note that Export is only available in the plugin starting from UE 4.25.*

Download the **.zip** attached to the [release matching your OS and UE version](https://github.com/sketchfab/unreal-plugin/releases) and unzip it into the `Plugins/` folder of your current project directory, or of the Engine directory structure.

You should end up with a structure such as below:

* Project installation: `PROJECT_DIRECTORY/Plugins/Sketchfab/[Zip content]`
* Engine installation: `UE_DIRECTORY/Engine/Plugins/Sketchfab/[Zip content]`

Once loaded, the Sketchfab plugins will be available under the `Window` menu, in the submenus `Asset Browser` (import) and `Exporter`:

![Screenshot-0](https://user-images.githubusercontent.com/52042414/118534086-20758380-b749-11eb-837c-d9c2b87cfbda.png)


### Latest releases

* [Release for 4.27 (Windows and MacOS)]()
* [Release for 5.0ea (Windows, Linux and MacOS)]()

The following releases are available for Windows only:

* [Release for 4.26](https://github.com/sketchfab/unreal-plugin/releases/tag/4.26-2.0.0) 
* [Release for 4.25](https://github.com/sketchfab/unreal-plugin/releases/tag/4.25-2.0.0)
* [Release for 4.21](https://github.com/sketchfab/unreal-plugin/releases/tag/4.21-1.1.1)
* [Release for 4.20](https://github.com/sketchfab/unreal-plugin/releases/tag/4.20-1.1.1)
* [Release for 4.19](https://github.com/sketchfab/unreal-plugin/releases/tag/4.19-1.1.1)


### Linux

The plugin is available for Linux, but only compatible as of today with UE5.0 early access.

Before using the plugin, you will need to manually install **minizip** (libminizip-dev) on your system. Adapt the following command to your package manager:

`sudo apt-get install libminizip-dev`

The linux plugin expects shared libraries to be located in `/usr/lib/x86_64-linux-gnu/libminizip.so`. If your local installation path varies, you can modify the path in [Source/GLTFExporter/SKGLTFExporter.Build.cs] before relaunching the project.

### MacOS

The plugin is available for MacOS, but only compatible with UE4.27 and 5.0ea.

A working installation of XCode **might** be needed to launch UE5.

You will also need to install minizip through a terminal. The prefered installation method is to use [Homebrew](https://brew.sh/) to install it.

If it is not already installed, you can install Homebrew by executing the following command in a terminal window:
`/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"`

Once homebrew is installed, you can install minizip through the following command:
`brew install minizip`

The MacOS plugin looks for dynamic libraries located in `/usr/local/lib/libminizip.dylib`. If your local installation path varies, you can modify the path in [Source/GLTFExporter/SKGLTFExporter.Build.cs] before relaunching the project.

## Authentication

*Please note that a Sketchfab account is required both to download and upload models.*

To authenticate yourself, click the "log in" button and enter your Sketchfab credentials. If you encounter errors at this step, please make sure to use the same method you use to connect to sketchfab.com (mail/password, Epic ID, Google, Facebook...).

*If you are using an email adress and password, the plugin will ask you to accept additional permissions necessary to download and upload models.*

![Authentication](https://user-images.githubusercontent.com/52042414/118534076-1eabc000-b749-11eb-97b8-d274f22f7200.png)

If you are a member of an organization, you can choose to use the plugin as an organization member instead of through your personal profile. You will then be able to import and export models from and to projects within your organization.

## Download a model from Sketchfab

The **Asset Browser** plugin allows you to seamlessly search and import:
* models licensed under Creative Commons licenses from Sketchfab
* models you published to Sketchfab - included private ones - if you have a [PRO plan](https://sketchfab.com/plans)
* models you purchased from the [Sketchfab Store](https://sketchfab.com/store)
* models from an organization account and associated projects (more info about [Sketchfab for Teams](https://sketchfab.com/3d-asset-management))

#### Browsing through available models

To use the **Asset Browser** interface, you will first have to select which kind of models you want to browse through (all CC models, purchases or your own models) in the dropdown at the left of the search bar.

Browsing through models is quite similar to [Sketchfab's search](https://sketchfab.com/search), and options to filter the results of your search query are available under the search bar:

![Screenshot-1](https://user-images.githubusercontent.com/52042414/118534080-1fdced00-b749-11eb-9870-5b28d2ea14f4.png)

Double clicking a model thumbnail will give you more information about the selected model and its associated license:

![Screenshot-2](https://user-images.githubusercontent.com/4066133/60019521-58bfd480-968e-11e9-902d-9fd3ae0247cf.JPG)

#### Importing models

You can import models either by selecting **Import model** on a specific model information window, or by selecting multiple thumbnails from the Asset Browser main window and clicking the **Download Selected** button.

A progress bar under the model's thumbnail will show the progress of the download.

Once a model has finished downloading, you can import it into your scene by **drag-and-dropping** the associated thumbnail into your project's **Content Browser**. 

A pop-up will then prompt you to accept the terms of the selected model's license (all freely downloadable models on Sketchfab are licensed under CC-attribution licenses, which are each associated with [various requirements](https://help.sketchfab.com/hc/en-us/articles/360038413232-Crediting-users-for-3D-model-downloads)).

![Screenshot-3](https://user-images.githubusercontent.com/4066133/60019527-5cebf200-968e-11e9-89df-5ba86f8c75c8.JPG)

Please note that downloaded models are cached in a directory on your computer, which you can manually clean or clear by clicking the **Clear Cache** button.

## Upload a model to Sketchfab

**(Beta - available for versions [4.25](https://github.com/sketchfab/unreal-plugin/releases/tag/4.25-2.0.0) and [4.26](https://github.com/sketchfab/unreal-plugin/releases/tag/4.26-2.0.0) only)**

The Sketchfab **Exporter** leverages recent [Unreal Engines GlTF export capabilities](https://www.unrealengine.com/marketplace/en-US/product/gltf-exporter) to allow direct upload of your 3D models from Unreal to Sketchfab.

The export process should be pretty straightforward if you have already uploaded a model to Sketchfab: fill in information about your model (title, description and tags), select if you want to upload the currently selected objects or the whole level, and set some publication options according to your needs (keep the model as a draft, set it as private or add an optionnal password protection for PRO users).

Depending on how the materials are set-up, you might want to choose to **bake** the models materials as textures, given that Unreal Engine node materials are not fully compatible with the GlTF file format and Sketchfab.

![Exporter](https://user-images.githubusercontent.com/52042414/118534085-20758380-b749-11eb-8983-21839500d74f.png)

You can find more information about the export capabilities - especially regarding materials - of the plugin [here](https://epicgames.ent.box.com/s/pd2h101708xqofbk4eqfzbko36iu2u7l). Please note that exporting animations and sequences to Sketchfab is not supported yet.

#### Note regarding upload size limit

Baking textures can often create large files depending on the number of materials and the chosen baking settings. As the maximum size of your uploads depends on your [Sketchfab plan](https://sketchfab.com/plans), an error message might pop up if this limit is exceeded. 

Depending on the characteristics of your models and materials, disabling texture baking - or lowering the baking resolution - as well as making sure that only selected objects are exported can be good ways to get your model to an appropriate size for your plan.






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

