cmake_minimum_required(VERSION 2.8)

project(thunderloomVRayMaya)
set(CMAKE_MACOSX_RPATH 1)

foreach(MAYA_VERSION ${MAYA_VERSIONS})
	IF (WIN32)
		set(MAYASDK${MAYA_VERSION}_ROOT "C:/Program Files/Autodesk/Maya ${MAYA_VERSION}" CACHE PATH "Path to Maya")
		set(MAYASDK${MAYA_VERSION}_INCLUDE "${MAYASDK${MAYA_VERSION}_ROOT}/include" CACHE PATH "Path to Maya include dir. Will use MAYA_ROOT as starting point if not set.")
		set(MAYASDK${MAYA_VERSION}_LIB "${MAYASDK${MAYA_VERSION}_ROOT}/lib" CACHE PATH "Path to Maya library dir. Will use MAYA_ROOT as starting point if not set.")
	ENDIF(WIN32)
	IF(APPLE)
		set(MAYASDK${MAYA_VERSION}_ROOT "/Applications/Autodesk/maya${MAYA_VERSION}/" CACHE PATH "Path to Maya")
		set(MAYASDK${MAYA_VERSION}_INCLUDE "${MAYASDK${MAYA_VERSION}_ROOT}/include" CACHE PATH "Path to Maya include dir. Will use go from MAYA_ROOT if not set.")
		set(MAYASDK${MAYA_VERSION}_LIB "${MAYASDK${MAYA_VERSION}_ROOT}/Maya.app/Contents/MacOS" CACHE PATH "Path to Maya library dir. Will use go from MAYA_ROOT if not set.")
	ENDIF(APPLE)
		
	set(INCLUDE_DIRS "")
	set(LIB_DIRS "")

	set(SOURCES 
		"maya_pluginmain.cpp"
		"maya_thunderloom.h"
		"maya_thunderloom.cpp")
	
	add_library(thunderloomVRayMaya${MAYA_VERSION} MODULE ${SOURCES})

	target_include_directories(thunderloomVRayMaya${MAYA_VERSION} PRIVATE ${TL_H_SOURCE_DIR})

	set(INCLUDE_DIRS ${MAYASDK${MAYA_VERSION}_INCLUDE} ${VRAY_MAYA${MAYA_VERSION}_INCLUDE})

	#set(LIB_DIRS ${MAYASDK${MAYA_VERSION}_LIB} ${VRAY_MAYA${MAYA_VERSION}_LIB})
	set(LIB_DIRS ${MAYASDK${MAYA_VERSION}_LIB})

	set(LIBS 
		"OpenMaya"
		"Foundation"
		)
		
	# Platform specific libs, files configs
	IF (WIN32)
		ADD_DEFINITIONS(-DUNICODE)
		ADD_DEFINITIONS(-D_UNICODE)
		ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS) # supress MSVC warn C4996
	ENDIF (WIN32)
	IF(APPLE)
		set(INCLUDE_DIRS ${INCLUDE_DIRS} 
			"/usr/local/include"
			)

		set(LIBS ${LIBS} 
			"-framework System"
			"-framework SystemConfiguration"
			"-framework CoreServices"
			"-framework Carbon"
			"-framework Cocoa"
			"-framework ApplicationServices"
			"-framework IOKit"
			)
		ADD_DEFINITIONS(
			-DCC_GNU_
			-DOSMac_
			-DOSMacOSX_
			-DOSMac_MachO_
			-D_LANGUAGE_C_PLUS_PLUS
			)
	ENDIF(APPLE)

	# Load external headers with SYSTEM flag to make supress these warnings.
	target_include_directories(thunderloomVRayMaya${MAYA_VERSION} SYSTEM PRIVATE ${INCLUDE_DIRS})

	# Load libraries
	target_link_directories(thunderloomVRayMaya${MAYA_VERSION} PRIVATE ${LIB_DIRS})

	IF (WIN32)
		set_target_properties(thunderloomVRayMaya${MAYA_VERSION} PROPERTIES SUFFIX ".mll")
	ENDIF (WIN32)

	# Set warnings
	if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
		set_target_properties(thunderloomVRayMaya${MAYA_VERSION} PROPERTIES SUFFIX ".bundle")
		target_compile_options(thunderloomVRayMaya${MAYA_VERSION} PRIVATE 
			"-std=c++11"
			"-Wall"
			"-Wno-unknown-pragmas"
			"-Wno-unused-function"
			"-Wno-inconsistent-missing-override"
			"-Wno-writable-strings"
			)
	endif()

	# Link Libs
	target_link_libraries(thunderloomVRayMaya${MAYA_VERSION} ${LIBS})

	set(OUTPUT_MAYA_PATH "V-Ray for Maya ${MAYA_VERSION}/")
	install(FILES 
		"${CMAKE_CURRENT_SOURCE_DIR}/thunderloom.mod" 
		"${CMAKE_CURRENT_SOURCE_DIR}/vraythunderloommtl.txt"
		DESTINATION ${OUTPUT_MAYA_PATH})
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/AEVRayThunderLoomMtlTemplate.mel" DESTINATION ${OUTPUT_MAYA_PATH}/thunderloom_maya_module/scripts/)
	install(TARGETS thunderloomVRayMaya${MAYA_VERSION} DESTINATION ${OUTPUT_MAYA_PATH}/thunderloom_maya_module/plug-ins/)
	install(TARGETS vray_thunderloom_MAYA${MAYA_VERSION} DESTINATION ${OUTPUT_MAYA_PATH}/vray_plugin/)
	install(FILES $<TARGET_PDB_FILE:thunderloomVRayMaya${MAYA_VERSION}> DESTINATION pdbs OPTIONAL)
endforeach()
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/README.md" DESTINATION . RENAME "README_V-Ray_Maya.md")
