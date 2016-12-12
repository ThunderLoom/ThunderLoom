default:osx

SOURCES = main.cpp ../dependencies/gl3w/src/gl3w.c \
    ../dependencies/imgui/*.cpp
INCLUDES= -I../../src/ -I../dependencies/imgui -I../dependencies/gl3w/include -I/opt/local/include
LIBS    = -L/opt/local/lib -lglfw -framework CoreFoundation -lstdc++
osx:
	clang -g $(SOURCES) $(INCLUDES) $(LIBS) -o pattern_editor
