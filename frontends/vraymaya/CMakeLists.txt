cmake_minimum_required(VERSION 2.8)

set(CMAKE_MACOSX_RPATH 1)

IF(WIN32 AND (NOT (MSVC_VERSION STREQUAL "1700"))) #VS11
    message(SEND_WARNING "V-Ray requires compiler vc11 (Visual Studio 2012)!")
ENDIF()


project(thunderLoomVRayMaya)

# Varibles
set(RELEASE_DIR ${CMAKE_SOURCE_DIR}/release/${CMAKE_SYSTEM_NAME} CACHE PATH "Release output")
set(MAYA_VERSION "2017" CACHE STRING "Maya version (year)")
set(OUTPUT_MAYA_PATH "${RELEASE_DIR}/V-Ray for Maya${MAYA_VERSION}/")
IF (WIN32)
    set(MAYA_ROOT "C:/Program Files/Autodesk/Maya${MAYA_VERSION}" CACHE PATH "Path to Maya")
    set(MAYA_INCLUDE "${MAYA_ROOT}/include" CACHE PATH "Path to Maya include dir. Will use MAYA_ROOT as starting point if not set.")
    set(MAYA_LIB "${MAYA_ROOT}/lib" CACHE PATH "Path to Maya library dir. Will use MAYA_ROOT as starting point if not set.")
    
    set(VRAYMAYA_ROOT "C:/Program Files/Chaos Group/V-Ray/Maya ${MAYA_VERSION} for x64/" CACHE PATH "Path to V-Ray for Maya")
    set(VRAYMAYA_INCLUDE "${VRAYMAYA_ROOT}/include/" CACHE PATH "Path to V-Ray for Maya include dir. Will use VRAYMAYA_ROOT as starting point if not set.")
    set(VRAYMAYA_LIB "${VRAYMAYA_ROOT}/lib/x64/vc11/" CACHE PATH "Path to V-Ray for Maya Library dir. Will use VRAYMAYA_ROOT as starting point if not set.")
ENDIF(WIN32)
IF(APPLE)
    set(MAYA_ROOT "/Applications/Autodesk/maya${MAYA_VERSION}/include" CACHE PATH "Path to Maya")
    set(MAYA_INCLUDE "${MAYA_ROOT}/include" CACHE PATH "Path to Maya include dir. Will use go from MAYA_ROOT if not set.")
    set(MAYA_LIB "${MAYA_ROOT}/Maya.app/Contents/MacOS" CACHE PATH "Path to Maya library dir. Will use go from MAYA_ROOT if not set.")

    set(VRAYMAYA_ROOT "/Applications/ChaosGroup/V-Ray/Maya${MAYA_VERSION}/include" CACHE PATH "Path to V-Ray for Maya")
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


# Additional includes
set(TL_H_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../src)
set(MAYA_SDK_INCLUDE "C:/Program Files/Autodesk/Maya2017/include")

set(SOURCES 
    "maya_pluginmain.cpp"
    "maya_thunderloom.h"
    "maya_thunderloom.cpp")

set(INCLUDE_DIRS
    ${TL_H_SOURCE_DIR}
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
ENDIF (WIN32)
IF(APPLE)
    set(INCLUDE_DIRS
        ${INCLUDE_DIRS} 
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

include_directories (${INCLUDE_DIRS})
link_directories(${LIB_DIRS})

add_library(thunderLoomVRayMaya MODULE ${SOURCES})

IF (WIN32)
    set_target_properties(thunderLoomVRayMaya PROPERTIES SUFFIX ".mll")
    IF(MSVC)
        set_target_properties( thunderLoomVRayMaya PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH} )
        set_target_properties( thunderLoomVRayMaya PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG ${LIBRARY_OUTPUT_PATH} )
        set_target_properties( thunderLoomVRayMaya PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${LIBRARY_OUTPUT_PATH} )
        set_target_properties( thunderLoomVRayMaya PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${LIBRARY_OUTPUT_PATH} )
    ENDIF(MSVC)
ENDIF (WIN32)

# Remove some warnings
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(thunderLoomVRayMaya PRIVATE 
        "-std=c++11"
        "-Wno-inconsistent-missing-override"
        "-Wno-writable-strings"
        "-Wmissing-method-return-type"
        )
endif()

# Link Libs
target_link_libraries(thunderLoomVRayMaya ${LIBS})

# Setup build directory
add_custom_command(TARGET thunderLoomVRayMaya POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${PROJECT_SOURCE_DIR}/README.md
                ${RELEASE_DIR}/README_Maya.md
        COMMAND ${CMAKE_COMMAND} -E copy
                ${PROJECT_SOURCE_DIR}/thunderloom.mod
                ${OUTPUT_MAYA_PATH}/
        COMMAND ${CMAKE_COMMAND} -E copy
                ${PROJECT_SOURCE_DIR}/vraythunderloommtl.txt
                ${OUTPUT_MAYA_PATH}/
        COMMAND ${CMAKE_COMMAND} -E copy
                ${PROJECT_SOURCE_DIR}/AEVRayThunderLoomMtlTemplate.mel
                ${OUTPUT_MAYA_PATH}/thunderloom_maya_module/scripts/
                )

