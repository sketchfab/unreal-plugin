# Sketchfab plugin for Unreal Engine

**Unreal Engine plugin to export and import models to and from Sketchfab**

* [Installation](#installation)
* [Authentication](#authentication)
* [Importing models from Sketchfab](#download-a-model-from-sketchfab)
* [Exporting models to Sketchfab](#upload-a-model-to-sketchfab)
* [Known issues](#known-issues-and-limitations)

## Installation

Download the **.zip** attached to the [release matching your OS and UE version](https://github.com/sketchfab/unreal-plugin/releases) and unzip it into the `Plugins/` folder of your current project directory, or of the Engine directory structure.

You should end up with a structure such as below:

* Project installation: `PROJECT_DIRECTORY/Plugins/Sketchfab/[Zip content]`
* Engine installation: `UE_DIRECTORY/Engine/Plugins/Sketchfab/[Zip content]`

If you are using Linux or MacOS, please refer to [specific installation steps, issues and workarounds for MacOS and Linux](#macos-and-linux).

Once loaded, the Sketchfab plugins will be available under the `Window` menu, in the submenus `Asset Browser` (import) and `Exporter`:

![menu](https://user-images.githubusercontent.com/52042414/139413969-53e16a2a-beed-43a8-aec8-f28b5cbad41d.jpg)

### Latest release

* [Release 2.1.0 for UE 4.27 and UE 5.0ea](https://github.com/sketchfab/unreal-plugin/releases/tag/2.1.0) *(also available for 4.25 and 4.26, on Windows only)*.

### Older releases *(Windows only)*:

* [Release for 4.21](https://github.com/sketchfab/unreal-plugin/releases/tag/4.21-1.1.1)
* [Release for 4.20](https://github.com/sketchfab/unreal-plugin/releases/tag/4.20-1.1.1)
* [Release for 4.19](https://github.com/sketchfab/unreal-plugin/releases/tag/4.19-1.1.1)

## Authentication

*Please note that a Sketchfab account is required both to download and upload models.*

To authenticate yourself, click the "log in" button and enter your Sketchfab credentials. If you encounter errors at this step, please make sure to use the same method you use to connect to sketchfab.com (mail/password, Epic ID, Google, Facebook...).

*If you are using an email adress and password, the plugin will ask you to accept additional permissions necessary to download and upload models.*

![login](https://user-images.githubusercontent.com/52042414/139413967-bb7b8b1b-0b86-44ba-b535-e927568638cc.jpg)

If you are a member of an organization, you can choose to use the plugin as an organization member instead of through your personal profile. You will then be able to import and export models from and to projects within your organization.

## Download a model from Sketchfab

The **Asset Browser** plugin allows you to seamlessly search and import:
* models licensed under Creative Commons licenses from Sketchfab
* models you published to Sketchfab - included private ones - if you have a [PRO plan](https://sketchfab.com/plans)
* models you purchased from the [Sketchfab Store](https://sketchfab.com/store)
* models from an organization account and associated projects (more info about [Sketchfab for Teams](https://sketchfab.com/3d-asset-management))

### Browsing through available models

To use the **Asset Browser** interface, you will first have to select which kind of models you want to browse through (all CC models, purchases or your own models) in the dropdown at the left of the search bar.

Browsing through models is quite similar to [Sketchfab's search](https://sketchfab.com/search), and options to filter the results of your search query are available under the search bar:

![browser](https://user-images.githubusercontent.com/52042414/139413958-e59744d6-7696-4e85-991f-9d64b1381dc0.jpg)

Double clicking a model thumbnail will give you more information about the selected model and its associated license:

![modeldetails](https://user-images.githubusercontent.com/52042414/139413954-97fcbe31-6be1-4e52-952f-8d421a7572c2.jpg)

### Importing models

You can import models either by selecting **Import model** on a specific model information window, or by selecting multiple thumbnails from the Asset Browser main window and clicking the **Download Selected** button.

A progress bar under the model's thumbnail will show the progress of the download.

Once a model has finished downloading, you can import it into your scene by **drag-and-dropping** the associated thumbnail into your project's **Content Browser**. 

A pop-up will then allow you to modify some import options, as well as prompt you to accept the terms of the selected model's license (all freely downloadable models on Sketchfab are licensed under CC-attribution licenses, which are each associated with [various requirements](https://help.sketchfab.com/hc/en-us/articles/360038413232-Crediting-users-for-3D-model-downloads)). Here are the import options currently available:

* **Merge meshes**: This option will merge all primitives of the downloaded mesh together, resulting in a single Static Mesh. Unchecking it keeps all primitives separated, and can be useful to tweak a model, or to import asset packs for instance.
* **Apply World Transform**: Unchecking this option will result in the mesh transforms being ignored upon import.
* **Import Materials**: Unchecking this option will only import the geometry of the model, and will ignore materials and textures.
* **Import in New Folder**: Checking this option will import every meshes, materials and textures in a new folder of your Content Browser.

![importoptions](https://user-images.githubusercontent.com/52042414/139413966-421f2c31-cb34-41ce-abb1-873122113523.jpg)

*Please note that downloaded models are cached in a directory on your computer, which you can manually clear by clicking the **Clear Cache** button.*

### Importing local gltf models

The Sketchfab plugin allows you to directly import .gltf or .glb models, as well as .zip archives containing such files:

* Drag and drop the selected .gltf, .glb or .zip archive into the **Content Browser** of the Unreal Editor.
* Use the Content Browser "Import" button to select the file to import.

## Upload a model to Sketchfab

**Beta - available for Unreal Engine versions starting from 4.25 only**

The Sketchfab **Exporter** leverages the [GLTF-Exporter plugin](https://www.unrealengine.com/marketplace/en-US/product/gltf-exporter) to allow direct upload of your 3D models from Unreal to Sketchfab.

The export process should be pretty straightforward if you have already uploaded a model to Sketchfab: fill in information about your model (title, description and tags), select if you want to upload the currently selected objects or the whole level, and set some publication options according to your needs (keep the model as a draft, set it as private or add an optionnal password protection for PRO users).

Depending on how the materials are set-up, you might want to choose to **bake** the models materials as textures, given that Unreal Engine node materials are not fully compatible with the GlTF file format and Sketchfab.

![exporter](https://user-images.githubusercontent.com/52042414/139413961-a549b726-cb08-40d5-be7b-0c4bfbe25b38.jpg)

You can find more information about the export capabilities - especially regarding materials - of the plugin [here](https://epicgames.ent.box.com/s/pd2h101708xqofbk4eqfzbko36iu2u7l). Please note that exporting animations to Sketchfab is not supported yet.

### Note regarding upload size limit

Baking textures can often create large files depending on the number of materials and the chosen baking settings. As the maximum size of your uploads depends on your [Sketchfab plan](https://sketchfab.com/plans), an error message might pop up if this limit is exceeded. 

Depending on the characteristics of your models and materials, disabling texture baking - or lowering the baking resolution - as well as making sure that only selected objects are exported can be good ways to get your model to an appropriate size for your plan.

## Known issues and limitations

### General limitations

* The plugin currently only imports and exports **Static Meshes**, meaning animations will not be taken into account, and rigged models will be imported in a default pose, which might not match the corresponding model's pose on Sketchfab.
* The plugin leverages the [GlTF file format](https://www.khronos.org/gltf/) for import and export. Some material channels are therefore not supported given the file specification, as well as the differences of rendering technologies between Unreal Engine and Sketchfab (Anisotropy, Cavity, Refraction transparency, Sheen...).
* Due to visual optimizations performed on sketchfab.com, some depth sorting issues might arise with models using a texture as blending transparency. A possible workaround is to switch the material transparency to opaque if applicable, or manually edit the transparency textures and materials.
* If you encounter bugs or unexpected behaviour, do not hesitate to [report an issue](https://github.com/sketchfab/unreal-plugin/issues), attaching information about your OS, UE version, and screenshots or log reports if available.

### Login errors

If you don't manage to log in to Sketchfab through the plugin (generally due to O.S. errors or web browser incompatibilites, specifically on Linux and MacOS), you can still import Sketchfab models into your project.

To do so, manually download the .zip archive containing the GlTF archive directly from the model page on Sketchfab.com.

You will then need to follow instructions to [import a gltf file from your computer](#importing-local-gltf-models).

## MacOS and Linux

### MacOS

#### Minizip installation

Before [installing the plugin](#installation), you will need to install [minizip](http://www.winimage.com/zLibDll/minizip.html). The prefered installation method is to use [Homebrew](https://brew.sh/) to install it, which you can install by executing the following command in a terminal window:

```/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"```

Once homebrew is installed, you can install minizip through the following command:

```brew install minizip```

The MacOS plugin looks for dynamic libraries located in `/usr/local/lib/libminizip.dylib`. If your local minizip installation path varies or if you encounter C++ symbols errors, you can modify the path in [Source/GLTFExporter/SKGLTFExporter.Build.cs] before relaunching the project.

#### MacOS known issues

* The plugin was only tested with MacOS BigSur 11.5 and 11.6. Issues might arise on different versions. In particular, login issues happened while testing the plugin on MacOS Monterey with UE 5.0ea. If you encounter similar issues, please refer to [login errors workaround](#login-errors).
* The plugin was tested with Unreal Engine 4.27 and 5.0ea. You will need to recompile it if you wish to use it in previous versions. The plugin source code should allow it to run on such versions, but this was not tested and might not work as expected.
* Visual issues might arise regarding thumbnails and the login interface scaling on MacOS depending on your screen resolution and DPI settings.

### Linux

#### Minizip installation

Before [installing the plugin](#installation), you will need to install [minizip](http://www.winimage.com/zLibDll/minizip.html) (libminizip-dev) on your system, by adapting the following command to your package manager:

`sudo apt-get install libminizip-dev`

The linux plugin expects shared libraries to be located in `/usr/lib/x86_64-linux-gnu/libminizip.so`. If your local installation path varies  or if you encounter C++ symbols errors, you can modify the path in [Source/GLTFExporter/SKGLTFExporter.Build.cs] before relaunching the project.

#### Linux known issues

* For Unreal Engine 5.0 early access on Linux, special signs on keyboards other than QWERTY might not be recognized. In particular, you might need to switch your keyboard to QWERTY to input the `@` sign (Shift + 2) during the login process, which is for instance not recognized on a AZERTY keyboard.
* Login is impossible with Unreal Engine 4.27 due to the version of the internal web browser used by Unreal Engine (CSRF error). Please refer to  [login errors workaround](#login-errors) in order to import models.
