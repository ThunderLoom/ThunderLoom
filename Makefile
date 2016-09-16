default: mitsuba

mitsuba:
	scons --directory=frontends/mitsuba/ release
	cp -r frontends/mitsuba/blender_mtsblend frontends/mitsuba/README.md build/macos/
	cp -r frontends/mitsuba/blender_mtsblend frontends/mitsuba/README.md build/linux/

3dsMax2016:
ifeq ($(OS),Windows_NT)
	msbuild frontends\vray3dsMax\vray3dsMaxThunderLoom.sln /p:Configuration="max 2016 release" /p:Platform="x64"
	rmdir /S /Q build\win32
	mkdir build\win32
	xcopy /Y frontends\vray3dsMax\3dsMax\build\3dsMax2016 build\win32
	copy frontends\vray3dsMax\README.md build\win32\3dsMax2016\
else
	echo "Build for 3dsMax2016 only supported on windows"
endif

#package_release:
