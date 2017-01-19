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

OSX: glfw can be obtained using homebrew. "brew install glfw"
The Makefile for the standalone pattern editor is set up for this.
Macports can also be used, just make sure to change 
/usr/local/lib to /opt/local/lib in the Makefile.

Linux: glfw and its devel files (glfw-devel) are usually available from package managers, dnf or apt-get. 
