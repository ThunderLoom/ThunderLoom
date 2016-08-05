Practical offline rendering of woven cloth

STAG paper 16

Although the code will be released publicly within short, this is not the final
release, please only use it to evaluate the paper and do not share it with
anyone just yet.

We apologize for the current state of the code. Although it is 
functional, we plan to polish it a bit more in the comming weeks before
releasing it publicly.
There are still some bugs in the V-Ray implementation
and the code needs to be cleaned up and documented.

The code in the 'src' folder contains the core of the shading model, which should
be portable enough to be integrated with any rendering engine.
The file 'woven_cloth.h' describes the API of the shader.

The 'vray3dsMax' folder includes contains the V-Ray/3ds Max plugin which is
discussed in the paper. This plugin has been tested  3ds Max 2015/2016 and
V-Ray 3.x

The other folders contain the blender addon which, however, is not up to date
and currently not functional.

