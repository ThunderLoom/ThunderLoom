default:osx

SOURCES = main.cpp macos_wrapper.mm ../dependencies/gl3w/GL/gl3w.c \
    ../dependencies/imgui/*.cpp
INCLUDES= -I../../src/ -I../dependencies/imgui -I../dependencies/gl3w -I/usr/local/include 
LIBS    = -L/usr/local/lib -lglfw -framework Cocoa -lstdc++
osx:
	clang -g $(SOURCES) $(INCLUDES) $(LIBS) -o pattern_editor
