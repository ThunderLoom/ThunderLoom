cmake_minimum_required(VERSION 2.8)

project(thunderLoomVRayMaya)
set(CMAKE_MACOSX_RPATH 1)

# Varibles
set(RELEASE_DIR ${CMAKE_SOURCE_DIR}/release/${CMAKE_SYSTEM_NAME} CACHE PATH "Release output")
set(MAYA_VERSION "2018" CACHE STRING "Maya version (year)")
set(OUTPUT_MAYA_PATH "${RELEASE_DIR}/V-Ray for Maya${MAYA_VERSION}/")
IF (WIN32)
    set(MAYA_ROOT "C:/Program Files/Autodesk/Maya${MAYA_VERSION}" CACHE PATH "Path to Maya")
    set(MAYA_INCLUDE "${MAYA_ROOT}/include" CACHE PATH "Path to Maya include dir. Will use MAYA_ROOT as starting point if not set.")
    set(MAYA_LIB "${MAYA_ROOT}/lib" CACHE PATH "Path to Maya library dir. Will use MAYA_ROOT as starting point if not set.")
    
    set(VRAYMAYA_ROOT "C:/Program Files/Chaos Group/V-Ray/Maya ${MAYA_VERSION} for x64/" CACHE PATH "Path to V-Ray for Maya")
    set(VRAYMAYA_INCLUDE "${VRAYMAYA_ROOT}/include/" CACHE PATH "Path to V-Ray for Maya include dir. Will use VRAYMAYA_ROOT as starting point if not set.")
    set(VRAYMAYA_LIB "${VRAYMAYA_ROOT}/lib/x64/vc14/" CACHE PATH "Path to V-Ray for Maya Library dir. Will use VRAYMAYA_ROOT as starting point if not set.")
ENDIF(WIN32)
IF(APPLE)
    set(MAYA_ROOT "/Applications/Autodesk/maya${MAYA_VERSION}/" CACHE PATH "Path to Maya")
    set(MAYA_INCLUDE "${MAYA_ROOT}/include" CACHE PATH "Path to Maya include dir. Will use go from MAYA_ROOT if not set.")
    set(MAYA_LIB "${MAYA_ROOT}/Maya.app/Contents/MacOS" CACHE PATH "Path to Maya library dir. Will use go from MAYA_ROOT if not set.")

    set(VRAYMAYA_ROOT "/Applications/ChaosGroup/V-Ray/Maya${MAYA_VERSION}/" CACHE PATH "Path to V-Ray for Maya")
    set(VRAYMAYA_INCLUDE "${VRAYMAYA_ROOT}/include/" CACHE PATH "Path to V-Ray for Maya include dir. Will use VRAYMAYA_ROOT as starting point if not set.")
    set(VRAYMAYA_LIB "${VRAYMAYA_ROOT}/lib/mavericks_x64/gcc-4.2-cpp/" CACHE PATH "Path to V-Ray for Maya include dir. Will use VRAYMAYA_ROOT as starting point if not set.")
ENDIF(APPLE)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH "${OUTPUT_MAYA_PATH}/thunderloom_maya_module/plug-ins")

# Make maya module structure
file(MAKE_DIRECTORY "${OUTPUT_MAYA_PATH}")
file(MAKE_DIRECTORY "${OUTPUT_MAYA_PATH}/thunderloom_maya_module")
file(MAKE_DIRECTORY "${OUTPUT_MAYA_PATH}/thunderloom_maya_module/plug-ins")
file(MAKE_DIRECTORY "${OUTPUT_MAYA_PATH}/thunderloom_maya_module/scripts")
file(MAKE_DIRECTORY "${OUTPUT_MAYA_PATH}/vray_plugin")



set(SOURCES 
    "maya_pluginmain.cpp"
    "maya_thunderloom.h"
    "maya_thunderloom.cpp")

include_directories(${TL_H_SOURCE_DIR})

set(INCLUDE_DIRS
    ${MAYA_INCLUDE}
    ${VRAYMAYA_INCLUDE}
    )

set(LIB_DIRS
    ${MAYA_LIB}
    ${VRAYMAYA_LIB}
    )

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
include_directories(SYSTEM ${INCLUDE_DIRS})

# Load libraries
link_directories(${LIB_DIRS})

add_library(thunderLoomVRayMaya MODULE ${SOURCES})

IF (WIN32)
    set_target_properties(thunderLoomVRayMaya PROPERTIES SUFFIX ".mll")
    IF(MSVC)
        # Do not put resulting dll in VisualStudios Debug/Release folders
        set_target_properties( thunderLoomVRayMaya PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH} )
        set_target_properties( thunderLoomVRayMaya PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${LIBRARY_OUTPUT_PATH} )
        set_target_properties( thunderLoomVRayMaya PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${LIBRARY_OUTPUT_PATH} )
        set_target_properties( thunderLoomVRayMaya PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${LIBRARY_OUTPUT_PATH} )
    ENDIF(MSVC)
ENDIF (WIN32)

# Set warnings
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set_target_properties(thunderLoomVRayMaya PROPERTIES SUFFIX ".bundle")
    target_compile_options(thunderLoomVRayMaya PRIVATE 
        "-std=c++11"
        "-Wall"
        "-Wno-unknown-pragmas"
        "-Wno-unused-function"
        "-Wno-inconsistent-missing-override"
        "-Wno-writable-strings"
        )
endif()

# Link Libs
target_link_libraries(thunderLoomVRayMaya ${LIBS})

# Setup build directory
add_custom_command(TARGET thunderLoomVRayMaya POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/README.md
                ${RELEASE_DIR}/README_Maya.md
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/thunderloom.mod
                ${OUTPUT_MAYA_PATH}/
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/vraythunderloommtl.txt
                ${OUTPUT_MAYA_PATH}/
        COMMAND ${CMAKE_COMMAND} -E make_directory
                ${OUTPUT_MAYA_PATH}/thunderloom_maya_module/scripts
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_CURRENT_SOURCE_DIR}/AEVRayThunderLoomMtlTemplate.mel
                ${OUTPUT_MAYA_PATH}/thunderloom_maya_module/scripts/
                )

