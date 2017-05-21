#default:build run

SOURCES  = vray_thunderloom.cpp 
INCLUDES = -I/usr/local/include -I../../../src/ -I/Applications/ChaosGroup/V-Ray/Maya2017/include #-I/Applications/ChaosGroup/V-Ray/Standalone_for_mavericks_x64/include
LDFLAGS  = -L/usr/local/lib 
LDFLAGS += -L/Applications/ChaosGroup/V-Ray/Maya2017/lib/mavericks_x64/gcc-4.2-cpp -lvray -ldl -lplugman_s -lvutils_s -lputils_s -lrayserver_s
WARNS    = -Wno-inconsistent-missing-override -Wno-writable-strings

.PHONY: build

all: libvray_thunderloom.so

libvray_thunderloom.so: plugin.obj
	g++ $(WARNS) -g -lpthread -L/Applications/ChaosGroup/V-Ray/Maya2017/lib/mavericks_x64/gcc-4.2-cpp -lc -lz -msse -msse2 -arch x86_64 -fvisibility=hidden -headerpad_max_install_names -framework System -framework CoreServices -framework Carbon -framework Cocoa -framework ApplicationServices -framework IOKit -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk -mmacosx-version-min=10.9 -lvutils_s  -ldl -lvray -lplugman_s -dynamic -bundle -flat_namespace -o "./libvray_thunderloom.so" -O "./plugin.obj"

plugin.obj: vray_thunderloom.cpp
	g++ $(WARNS) -g -std=c++11 -I../../src -I/Applications/ChaosGroup/V-Ray/Maya2017/include -DENABLE_DEBUG_PRINTF -msse -msse2 -arch x86_64 -Wreturn-type -Werror=return-type -DREQUIRE_IOSTREAM -DHAVE_EXR -D_LIBRARY_ALEMBIC -DHAVE_OPENSUBDIV -DHAVE_EMBREE -DVRAY_NO_GUI -D__VRAY4MAYA30__ -DBits64_ -D_REENTRANT  -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk -mmacosx-version-min=10.9 -fvisibility=hidden -DVRAY_EXPORTS -fexceptions -D "NDEBUG" -c -o "./plugin.obj" $(SOURCES)

clean:
	rm plugin.obj libvray_plugintest.so


#VRAY_FOR_MAYA2017_PLUGINS_x64=/Users/peter/Projects/active/ThunderLoom/frontends/vray /Applications/Autodesk/maya2017/Maya.app/Contents/MacOS/Maya ~/Documents/maya/projects/mayavraytest/mayatestscene.mb

#VRAY_FOR_MAYA2017_PLUGINS_x64=/Users/peter/Downloads/domemaster-stereo-shader-master/vray/source/maya\ and\ standalone/vray\ 3.0\ for\ Maya/vray_DomemasterStereo /Applications/Autodesk/maya2016/Maya.app/Contents/MacOS/Maya ~/Documents/maya/projects/mayavraytest/mayatestscene.mb

# VRAY_PLUGINS_x64=/Users/peter/Projects/active/ThunderLoom/frontends/vray /Applications/ChaosGroup/V-Ray/Standalone_for_mavericks_x64/bin/mavericks_x64/gcc-4.2/vray -sceneFile=/Users/peter/Documents/maya/projects/mayavraytest/vrayscene_mod.vrscene
# lldbx /Applications/ChaosGroup/V-Ray/Standalone_for_mavericks_x64/bin/mavericks_x64/gcc-4.2/vray.bin -- --sceneFile=/Users/peter/Documents/maya/projects/mayavraytest/vrayscene_mod.vrscene
