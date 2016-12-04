default: mitsuba

mitsuba:
	scons --directory=frontends/mitsuba/ release
	cp -r frontends/mitsuba/blender_mtsblend frontends/mitsuba/README.md build/macos/
	cp -r frontends/mitsuba/blender_mtsblend frontends/mitsuba/README.md build/linux/