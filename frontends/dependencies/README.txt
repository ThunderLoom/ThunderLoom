The pattern editor has these dependencies:
Dear imgui (included)
glfw
gl3w

How to obtain:

-- gl3w --
Download and run gl3w_gen.py from here: https://github.com/skaslev/gl3w
Put the resulting files into this folder, so that you get this structure:
-dependencies
	-gl3w
		-include
			-GL
		-src

-- glfw --
Windows: Download the pre-compiled binaries from here: http://www.glfw.org/download.html
Extract the files into a folder called glwf inside this folder

OSX: glfw can be obtained using macports. The Makefile for the standalone pattern editor is set up for this.