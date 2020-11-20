cmake_minimum_required(VERSION 3.11)

project(vray_thunderloom)
set(CMAKE_MACOSX_RPATH 1)


IF (BUILD_VRAYSTANDALONE)
    list(APPEND VRAY_BUILD_VERSIONS "STANDALONE")
ENDIF()
IF (BUILD_MAYA)
	foreach(MAYA_VERSION ${MAYA_VERSIONS})
		list(APPEND VRAY_BUILD_VERSIONS "${MAYA_VERSION}")
	endforeach()
ENDIF()

message(WARNING ${VRAY_BUILD_VERSIONS})

foreach(VRAY_VERSION ${VRAY_BUILD_VERSIONS})

    IF(${VRAY_VERSION} STREQUAL "STANDALONE")
		IF (WIN32)
			set(VRAY_STANDALONE_ROOT "C:/Program Files/Chaos Group/V-Ray/Standalone for x64/" CACHE PATH "Path to V-Ray standalone root dir")
			set(VRAY_STANDALONE_INCLUDE "${VRAY_STANDALONE_ROOT}/include" CACHE PATH "Path to V-Ray standalone include dir")
			set(VRAY_STANDALONE_LIB "${VRAY_STANDALONE_ROOT}/lib" CACHE PATH "Path to V-Ray standalone lib dir")
		ENDIF()
		IF (APPLE)
			set(VRAY_STANDALONE_ROOT "/Applications/ChaosGroup/V-Ray/Standalone_for_mavericks_x64/" CACHE PATH "Path to V-Ray standalone root dir")
			set(VRAY_STANDALONE_INCLUDE "${VRAY_STANDALONE_ROOT}/include/" CACHE PATH "Path to V-Ray standalone include dir")
			set(VRAY_STANDALONE_LIB "${VRAY_STANDALONE_ROOT}/gcc-4.2-cpp/" CACHE PATH "Path to V-Ray standalone lib dir")
        ENDIF()
    ELSE()
		IF (WIN32)
			set(VRAY_MAYA${VRAY_VERSION}_ROOT "C:/Program Files/Chaos Group/V-Ray/Maya ${VRAY_VERSION}" CACHE PATH "Path to V-Ray for maya root dir")
			set(VRAY_MAYA${VRAY_VERSION}_INCLUDE "${VRAY_MAYA${VRAY_VERSION}_ROOT}/include" CACHE PATH "Path to V-Ray for maya include dir")
			set(VRAY_MAYA${VRAY_VERSION}_LIB "${VRAY_MAYA${VRAY_VERSION}_ROOT}/lib/" CACHE PATH "Path to V-Ray for maya lib dir")
		ENDIF()
		IF (APPLE)
			set(VRAY_MAYA${VRAY_VERSION}_ROOT "/Applications/ChaosGroup/V-Ray/Maya ${VRAY_VERSION}" CACHE PATH "Path to V-Ray for maya root dir")
			set(VRAY_MAYA${VRAY_VERSION}_INCLUDE "${VRAY_MAYA${VRAY_VERSION}_ROOT}/include" CACHE PATH "Path to V-Ray for maya include dir")
			set(VRAY_MAYA${VRAY_VERSION}_LIB "${VRAY_MAYA${VRAY_VERSION}_ROOT}/lib/mavericks_x64/gcc-4.2-cpp" CACHE PATH "Path to V-Ray for maya lib dir")
        ENDIF()
		set(VRAY_VERSION "MAYA${VRAY_VERSION}")
    ENDIF()

	# Additional includes
	set(SOURCES vray_thunderloom.cpp
				vray_thunderloom_updated.cpp
				vray_thunderloom.h)
	include_directories(${TL_H_SOURCE_DIR})
	include_directories(.)

	set(LIBS 
		"vray"
		"vutils_s"
		"rayserver_s"
		"putils_s"
		"plugman_s")

	set(INCLUDE_DIRS "")
	set(LIB_DIRS "")

	set(INCLUDE_DIRS ${INCLUDE_DIRS} ${VRAY_${VRAY_VERSION}_INCLUDE})
	set(LIB_DIRS ${LIB_DIRS} ${VRAY_${VRAY_VERSION}_LIB})

	# Platform specific libs, files configs
	IF (WIN32)
		ADD_DEFINITIONS(-DUNICODE)
		ADD_DEFINITIONS(-D_UNICODE)
		ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS) # supress MSVC warn C4996
	ENDIF (WIN32)
	IF(APPLE)
		set(INCLUDE_DIRS ${INCLUDE_DIRS} "/usr/local/include")
		set(LIBS ${LIBS} 
			"dl"
			"pthread"
			"c"
			"z"
			"-framework System"
			"-framework CoreServices"
			"-framework Carbon"
			"-framework Cocoa"
			"-framework ApplicationServices"
			"-framework IOKit"
			)
	ENDIF(APPLE)

	add_library(vray_thunderloom_${VRAY_VERSION} SHARED ${SOURCES})
	# Load external headers with SYSTEM flag to make supress these warnings.
	message(WARNING ${INCLUDE_DIRS})
	target_include_directories(vray_thunderloom_${VRAY_VERSION} SYSTEM PRIVATE ${INCLUDE_DIRS})

	# Load libraries
	target_link_directories(vray_thunderloom_${VRAY_VERSION} PRIVATE ${LIB_DIRS})

	# Build
	IF(APPLE)
		set_target_properties(vray_thunderloom_${VRAY_VERSION} PROPERTIES SUFFIX ".so")
		target_compile_definitions(vray_thunderloom_${VRAY_VERSION} PRIVATE
			REQUIRE_IOSTREAM=1
			HAVE_EXR=1
			_LIBRARY_ALEMBIC=1
			HAVE_OPENSUBDIV=1
			HAVE_EMBREE=1
			VRAY_NO_GUI=1
			Bits64_=1
			_REENTRANT=1
			VRAY_EXPORTS=1
			)
	ENDIF(APPLE)

	# Remove some warnings
	if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
		target_compile_options(vray_thunderloom_${VRAY_VERSION} PRIVATE 
			"-std=c++11"
			"-Wall"
			"-Wno-unknown-pragmas"
			"-Wno-unused-function"
			"-Wno-inconsistent-missing-override"
			"-Wno-writable-strings"
			)
	endif()

	# Link Libs
	target_link_libraries(vray_thunderloom_${VRAY_VERSION} ${LIBS})
	
    IF(${VRAY_VERSION} STREQUAL "STANDALONE")
		install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md" DESTINATION . RENAME "README_V-Ray_Standalone.md")
		install(TARGETS vray_thunderloom_STANDALONE DESTINATION "V-Ray Standalone")
	ENDIF()
	install(FILES $<TARGET_PDB_FILE:vray_thunderloom_${VRAY_VERSION}> DESTINATION pdbs OPTIONAL)
	# Installs for builds for maya are defined in vraymaya/CMakeLists.txt
endforeach(VRAY_VERSION)

