default:osx

SOURCES = main.cpp ../dependencies/gl3w/GL/gl3w.c \
    ../../src/woven_cloth.cpp ../dependencies/imgui/*.cpp
INCLUDES= -I../../src/ -I../dependencies/imgui -I../dependencies/gl3w -I/opt/local/include
LIBS    = -L/opt/local/lib -lglfw -framework CoreFoundation -lstdc++
osx:
	clang -g $(SOURCES) $(INCLUDES) $(LIBS)
