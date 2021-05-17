# Sketchfab plugin for Unreal Engine

**Unreal plugin Windows to export and import models to and from Sketchfab**

Latest releases:

* [Release for 4.26](https://github.com/sketchfab/unreal-plugin/releases/tag/4.26-2.0.0) 
* [Release for 4.25](https://github.com/sketchfab/unreal-plugin/releases/tag/4.25-2.0.0)
* [Release for 4.21](https://github.com/sketchfab/unreal-plugin/releases/tag/4.21-1.1.1)
* [Release for 4.20](https://github.com/sketchfab/unreal-plugin/releases/tag/4.20-1.1.1)
* [Release for 4.19](https://github.com/sketchfab/unreal-plugin/releases/tag/4.19-1.1.1)

Please note that the exporter is only available in the releases for Unreal Engine 4.25 and 4.26.


## Installation

Download the **.zip** attached to the [release matching your UE version](https://github.com/sketchfab/unreal-plugin/releases) and unzip it into a new plugin folder in your project directory.

If you already have plugins installed, you only need to add a new `Sketchfab` directory, otherwise you will need to create `Plugins` folder manually. You should end up with this structure:

`PROJECT_DIRECTORY/Plugins/Sketchfab/[Zip content]`

If Unreal Engine is running, you will need to close and reopen it in order to make it load the plugin.

Once loaded, the Sketchfab plugins will be available under the `Window` menu:

![Screenshot-0](https://user-images.githubusercontent.com/52042414/118534086-20758380-b749-11eb-837c-d9c2b87cfbda.png)


## Authentication

*Please note that a Sketchfab account is required both to download and upload models.*

To authenticate yourself, click the login button and enter your Sketchfab credentials before validating authentication.

If you encounter errors at this step, please make sure to use the same method you use to connect to sketchfab.com (mail/password, Google, Facebook...).

If you are using an email adress and password, the plugin will ask you to accept additional permissions necessary to download and upload models.

![Authentication](https://user-images.githubusercontent.com/52042414/118534076-1eabc000-b749-11eb-97b8-d274f22f7200.png)

## Download a model from Sketchfab

The **Asset Browser** plugin allows you to seamlessly search and import:
* models licensed under Creative Commons licenses from Sketchfab
* your purchases from the [Sketchfab Store](https://sketchfab.com/store)
* models you published to Sketchfab - included private ones - if you have a [PRO plan](https://sketchfab.com/plans)

#### Browsing through available models

To use the **Asset Browser** interface, you will first have to select which kind of models you want to browse through (all CC models, purchases or your own models) in the dropdown at the left of the search bar.

Browsing through models is quite similar to [Sketchfab's search](https://sketchfab.com/search), and options to filter the results of your search query are available under the search bar:

![Screenshot-1](https://user-images.githubusercontent.com/52042414/118534080-1fdced00-b749-11eb-9870-5b28d2ea14f4.png)

Double clicking a model thumbnail will give you more information about the selected model and its associated license:

![Screenshot-2](https://user-images.githubusercontent.com/4066133/60019521-58bfd480-968e-11e9-902d-9fd3ae0247cf.JPG)

#### Importing models

You can import models either by selecting **Import model** on a specific model information window, or by selecting multiple thumbnails from the Asset Browser main window and clicking the **Download Selected** button.

A progress bar under the model's thumbnail will show the progress of the download.

Once a model has finished downloading, you can import it into your scene by drag-and-dropping the associated thumbnail into your project's **Content Browser**. 

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