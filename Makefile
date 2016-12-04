default: mitsuba

mitsuba:
	scons --directory=frontends/mitsuba/ release
	mkdir -p build/macos
	mkdir -p build/linux
	mkdir -p build/windows
	cp -r frontends/mitsuba/blender_mtsblend frontends/mitsuba/README.md build/macos/
	cp -r frontends/mitsuba/blender_mtsblend frontends/mitsuba/README.md build/linux/
	cp -r frontends/mitsuba/blender_mtsblend frontends/mitsuba/README.md build/windows/
