# Make file fro standalone_pattern_editor linux
#
# In frontends/standalone_pattern_editor/ run
#
# make -f targets/linux/Makefile

default: linux

SOURCES = main.cpp ../dependencies/gl3w/src/gl3w.c \
    ../dependencies/imgui/*.cpp
INCLUDES= -I../../src/ -I../dependencies/imgui -I../dependencies/gl3w/include -I/usr/include 
LIBS    = -L/usr/lib64 -lglfw -lstdc++ -ldl

linux:
	mkdir -p build/pattern_editor
	g++ -g $(SOURCES) $(INCLUDES) $(LIBS) -o build/pattern_editor/pattern_editor.bin
linux_bundle: clean linux
	mkdir -p build/pattern_editor/lib
	cp -P /usr/lib64/libglfw.so* build/pattern_editor/lib/
	echo "#!/bin/sh" >> build/pattern_editor/runpatterneditor.sh
	echo "LD_LIBRARY_PATH=\"\$LD_LIBRARY_PATH:./lib\" ./pattern_editor.bin" >> build/pattern_editor/runpatterneditor.sh
	chmod +x build/pattern_editor/runpatterneditor.sh
clean:
	rm -rfv ./build
