# Make file fro standalone_pattern_editor macos
#
# In frontends/standalone_pattern_editor/ run
#
# make -f targets/macos/Makefile

default:macos

SOURCES = main.cpp macos_wrapper.mm ../dependencies/gl3w/GL/gl3w.c \
    ../dependencies/imgui/*.cpp
INCLUDES= -I../../src/ -I../dependencies/imgui -I../dependencies/gl3w -I/usr/local/include 
LIBS    = -L/usr/local/lib -lglfw -framework Cocoa -lstdc++

macos:
	clang -g $(SOURCES) $(INCLUDES) $(LIBS) -o build/pattern_editor

macos_bundle:
	## xcode SIGNING
	# CODE_SIGN_IDENTITY="macOS Developer: Mister Smith" 
	# PROVISIONING_PROFILE="XXXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXX"
	xcodebuild -project targets/macos/StandalonePatternEditor.xcodeproj \
		-target "StandalonePatternEditor" \
		-configuration Release \
		CODE_SIGN_IDENTITY="$(CODE_SIGN_IDENTITY)" \
		PROVISIONING_PROFILE="$(PROVISIONING_PROFILE)"
