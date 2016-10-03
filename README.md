ThunderLoom
===
![teaser8](https://cloud.githubusercontent.com/assets/116268/18617674/92d092c4-7dd5-11e6-9e2d-04d64c712c40.png)
*A physically based shader for woven cloth*

This project consits of a core shading model with a simple API that can be used
to integrate into different rendering engines. The source code for this
core can be found in the 'src' folder. 
The file 'woven_cloth.h' describes the API of the shader.

The shader has been incorporated into two rendering engines.
The 'frontends' folder includes two plugins for the shader model.

* 'vray3dsMax' contains a V-Ray/3ds Max plugin that has been tested in 
3ds Max 2014,2015,2016 and V-Ray 3.x
* 'mitsuba' contains a plugin for mitsuba and patches for the mtsblend blender
addon.
