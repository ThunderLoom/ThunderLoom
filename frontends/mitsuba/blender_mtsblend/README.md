Patching mtsblend fro ThunderLoom support
===
*ThunderLoom is a physically based shader for woven cloth. https://github.com/vidarn/ThunderLoom*
*These modifications allow the ThunderLoom mitsuba shader to be controlled through blender*

`mtsblend` is an addon which integrates mitsuba and its shaders into blender. 
These modifications to `mtsblend` are required to allow the thunderloom mitsuba shader to be controlled through blender.

Note: This frontend has limitations compared to the 3dsMax version. Individual control over different yarn types is currently not supported.

#Install
First you will need to download `mtsblend`. See https://www.mitsuba-renderer.org/plugins.html This has been verified to work on version 0.5.1.
Next install the plugin by moving the `mtsblend` folder into the blender `addons` folder.

Now you have two choices. You can either manually apply the changes or use the accompanying patch file.

##Option 1: Manual modification
1. Move the file `thunderloom_mitsuba.py` to `mtsblend/nodes/`.
2. Add `thunderloom_mitsuba` to the `from ..nodes import ()` call after `node_bsdf` in `mtsblend/core/__init__.py`.
3. Add `thunderloom` to the dict `mitsuba_plugin_tree` under `bsdf`.

##Option 2: Applying the patch
In the `mtsblend` directory, run `git apply path-to-patch-dir/0001-add-thunderLoom-to-mtsblend.patch`.
Thats it.