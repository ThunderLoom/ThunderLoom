ThunderLoom
===
*A physically based shader for woven cloth*

The code in the 'src' folder contains the core of the shading model, which should
be portable enough to be integrated with any rendering engine.
The file 'woven_cloth.h' describes the API of the shader.

The 'frontend' folder includes two frontends for the shader model.
'vray3dsMax' contains a V-Ray/3ds Max plugin that has been tested in 3ds Max 2014,2015,2016 and
V-Ray 3.x

The 'mitsuba' folder contains a plugin for mitsuba and a blender addon.
