# Uses Mayas Makefile scripts to set libs and other settings.

# Set Maya location
MAYA_LOCATION = /Applications/Autodesk/maya2017/Maya.app/Contents
export MAYA_LOCATION

# Set the DEVKIT_LOCATION environment variable to the folder at which you have
# unarchived your devkit package, that is, where the /include and /mkspecs 
# directories are located.
DEVKIT_LOCATION = /Applications/Autodesk/maya2017
export DEVKIT_LOCATION

ifndef INCL_BUILDRULES

# Include platform specific build settings from Maya devkit
TOP := $(DEVKIT_LOCATION)/devkit/plug-ins
include $(TOP)/buildrules

# Always build the local plug-in when make is invoked from the
# directory.
all : plugins

endif

SRCDIR := ../vraymaya
DSTDIR := .

flameShader_SOURCES  := maya_pluginmain.cpp maya_thunderloom.cpp
flameShader_OBJECTS  := maya_pluginmain.o maya_thunderloom.o
flameShader_PLUGIN   := maya_vraypluginmain.$(EXT)
flameShader_MAKEFILE := ./Makefile

#
# Include the optional per-plugin Makefile.inc
#
#    The file can contain macro definitions such as:
#       {pluginName}_EXTRA_CFLAGS
#       {pluginName}_EXTRA_C++FLAGS
#       {pluginName}_EXTRA_INCLUDES
#       {pluginName}_EXTRA_LIBS
#-include $(SRCDIR)/Makefile.inc

# Set target specific flags.
$(flameShader_OBJECTS): CFLAGS   := $(CFLAGS)   $(flameShader_EXTRA_CFLAGS)
$(flameShader_OBJECTS): C++FLAGS := $(C++FLAGS) $(flameShader_EXTRA_C++FLAGS)
$(flameShader_OBJECTS): INCLUDES := $(INCLUDES) $(flameShader_EXTRA_INCLUDES)

depend_flameShader:     INCLUDES := $(INCLUDES) $(flameShader_EXTRA_INCLUDES)

$(flameShader_PLUGIN):  LFLAGS   := $(LFLAGS) $(flameShader_EXTRA_LFLAGS) 
$(flameShader_PLUGIN):  LIBS     := $(LIBS)   -lOpenMaya -lFoundation $(flameShader_EXTRA_LIBS) 

# Rules definitions
.PHONY: depend_flameShader clean_flameShader Clean_flameShader

$(flameShader_PLUGIN): $(flameShader_OBJECTS) 
	-rm -f $@
	$(LD) -o $@ $(LFLAGS) $^ $(LIBS)

depend_flameShader :
	makedepend $(INCLUDES) $(MDFLAGS) -f$(DSTDIR)/Makefile $(flameShader_SOURCES)

clean_flameShader:
	-rm -f $(flameShader_OBJECTS)

Clean_flameShader:
	-rm -f $(flameShader_MAKEFILE).bak $(flameShader_OBJECTS) $(flameShader_PLUGIN)


plugins: $(flameShader_PLUGIN)
depend:	 depend_flameShader
clean:	 clean_flameShader
Clean:	 Clean_flameShader
