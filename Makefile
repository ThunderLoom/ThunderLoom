default: mitsuba

mitsuba:
	scons --directory=frontends/mitsuba/ release
	cp -r frontends/mitsuba/blender_mtsblend frontends/mitsuba/README.md build/macos/
	cp -r frontends/mitsuba/blender_mtsblend frontends/mitsuba/README.md build/linux/

3dsMax2016:
ifeq ($(OS),Windows_NT)
	msbuild frontends\vray3dsMax\vray3dsMaxThunderLoom.sln /p:Configuration="max 2016 release" /p:Platform="x64"
	xcopy /u /Y frontends\vray3dsMax\3dsMax\build\thunderLoom.dlt build\win32\3dsMax2016\thunderLoom.dlt*
else
	echo "Build for 3dsMax2016 only supported on windows"
endif

#package_release:
