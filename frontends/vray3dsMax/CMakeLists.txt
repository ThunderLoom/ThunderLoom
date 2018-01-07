cmake_minimum_required(VERSION 2.8)

IF(NOT WIN32)
    message(FATAL_ERROR "thunderLoom for V-Ray for 3dsMax can only be built on windows!")
ENDIF()
IF(WIN32 AND (NOT (MSVC_VERSION STREQUAL "1700"))) #VS11
    message(SEND_WARNING "V-Ray requires compiler vc11 (Visual Studio 2012)!")
ENDIF()

# Maps to Visual Studio solution file (Tutorial.sln)
# The solution will have all targets (exe, lib, dll) 
# as Visual Studio projects (.vcproj)
project(thunderLoomVRay3dsMax)

# Varibles
set(RELEASE_DIR ${CMAKE_SOURCE_DIR}/release/${CMAKE_SYSTEM_NAME} CACHE PATH "Release output")
set(3DSMAX_VERSION "2017" CACHE STRING "3dsMax version (year)")
set(OUTPUT_3DSMAX_PATH "${RELEASE_DIR}/V-Ray for 3dsMax${3DSMAX_VERSION}/")

set(3DSMAXSDK_ROOT $ENV{ADSK_3DSMAX_SDK_${3DSMAX_VERSION}} CACHE PATH "Path to 3dsMax SDK.")
set(3DSMAXSDK_INCLUDE "${3DSMAXSDK_ROOT}/include" CACHE PATH "Path to 3dsMax SDK include dir. Will use MAYA_ROOT as starting point if not set.")
set(3DSMAXSDK_LIB "${3DSMAXSDK_ROOT}/lib/x64/Release" CACHE PATH "Path to 3dsMax SDK library dir. Will use MAYA_ROOT as starting point if not set.")

set(VRAY3DSMAX_ROOT $ENV{VRAY_OSL_PATH_3DSMAX${3DSMAX_VERSION}_x64}/../ CACHE PATH "Path to V-Ray for 3dsMax")
set(VRAY3DSMAX_INCLUDE "${VRAY3DSMAX_ROOT}/include/" CACHE PATH "Path to V-Ray for 3dsMax include dir. Will use VRAYMAYA_ROOT as starting point if not set.")
set(VRAY3DSMAX_LIB "${VRAY3DSMAX_ROOT}/lib/x64/" CACHE PATH "Path to V-Ray for 3dsMax Library dir. Will use VRAYMAYA_ROOT as starting point if not set.")

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${OUTPUT_3DSMAX_PATH})

# Additional includes
set(TL_H_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../src)
set(GL3W_SRC ${PROJECT_SOURCE_DIR}/../dependencies/gl3w/src)
set(GL3W_INCLUDE ${PROJECT_SOURCE_DIR}/../dependencies/gl3w/include)
set(GLFW_INCLUDE ${PROJECT_SOURCE_DIR}/../dependencies/glfw/include)
set(IMGUI_INCLUDE ${PROJECT_SOURCE_DIR}/../dependencies/imgui)

# Additional library directories
set(GLFW_LIB ${PROJECT_SOURCE_DIR}/../dependencies/glfw/lib-vc2012)

ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)

set  (SOURCES
    "3dsMax/3dsMaxThunderLoom.h"
    "3dsMax/3dsMaxThunderLoom.cpp"
    "3dsMax/Eval.h"
    "3dsMax/Eval.cpp"
    "3dsMax/resource.h"
    "3dsMax/vrayblinnmtl.rc"
    "3dsMax/DllEntry.cpp"
    "3dsMax/VrayThunderLoomBRDF.h"
    "3dsMax/VrayThunderLoomBRDF.cpp"
    "3dsMax/plugin.def"
    "${GL3W_SRC}/gl3w.c"
    "${IMGUI_INCLUDE}/imgui.cpp"
    "${IMGUI_INCLUDE}/imgui_draw.cpp"
    "${IMGUI_INCLUDE}/imgui_impl_glfw_gl3.cpp")

# Properties->C/C++->General->Additional Include Directories
include_directories (${TL_H_SOURCE_DIR}
    ${GL3W_INCLUDE}
    ${GLFW_INCLUDE}
    ${IMGUI_INCLUDE}
    ${3DSMAXSDK_INCLUDE}
    ${VRAY3DSMAX_INCLUDE})

# Properties->Linker->General->Additional Library Directories
link_directories(${GLFW_LIB}
    ${3DSMAXSDK_LIB}
    ${VRAY3DSMAX_LIB}
    ${VRAY3DSMAX_LIB}/vc11)

# Set Properties->General->Configuration Type to Dynamic Library(.dll)
# Creates math.dll with the listed sources collected in the variables
# Also adds sources to the Solution Explorer
add_library(thunderLoomVRay3dsMax SHARED ${SOURCES})
set_target_properties(thunderLoomVRay3dsMax PROPERTIES SUFFIX ".dlt")
IF(MSVC)
    set_target_properties( thunderLoomVRay3dsMax PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH} )
    set_target_properties( thunderLoomVRay3dsMax PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${LIBRARY_OUTPUT_PATH} )
    set_target_properties( thunderLoomVRay3dsMax PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${LIBRARY_OUTPUT_PATH} )
    set_target_properties( thunderLoomVRay3dsMax PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${LIBRARY_OUTPUT_PATH} )
ENDIF(MSVC)

# Properties->Linker->Input->Additional Dependencies
target_link_libraries (thunderLoomVRay3dsMax
    "paramblk2"
    "core"
    "geom"
    "maxutil"
    "bmm"
    "comctl32"
    "odbc32"
    "odbccp32"
    "vray2017"
    "vutils_s"
    "plugman_s"
    "vrender2017"
    "glfw3"
    "opengl32")

add_custom_command(TARGET thunderLoomVRay3dsMax POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${PROJECT_SOURCE_DIR}/README.md
                ${RELEASE_DIR}/README_3dsMax.md
                )
