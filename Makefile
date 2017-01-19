# run make (target) at project root

default: macos

macos : mitsuba_macos pattern_editor_macos_bundle
linux : mitsuba_linux pattern_editor_linux_bundle

mitsuba_macos:
	scons --directory=frontends/mitsuba/ release
	mkdir -p build/macos
	cp -r frontends/mitsuba/blender_mtsblend frontends/mitsuba/README.md build/macos/
mitsuba_linux:
	scons --directory=frontends/mitsuba/ release
	mkdir -p build/linux
	cp -r frontends/mitsuba/blender_mtsblend frontends/mitsuba/README.md build/linux/

pattern_editor_macos_bundle:
	(cd frontends/standalone_pattern_editor; make -f targets/macos/Makefile macos_bundle)
	mkdir -p build/macos
	cp -r frontends/standalone_pattern_editor/build/Release/StandalonePatternEditor.app build/macos/
pattern_editor_linux_bundle:
	(cd frontends/standalone_pattern_editor; make -f targets/linux/Makefile linux_bundle)
	mkdir -p build/linux
	cp -r frontends/standalone_pattern_editor/build/pattern_editor build/linux/
