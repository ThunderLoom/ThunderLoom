cmake_minimum_required(VERSION 3.11)

set(VERSION_MAJOR_REGEX "#define TL_VERSION_MAJOR[ \t]+(.+)")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../src/thunderloom.h"
    TL_VERSION_MAJOR REGEX ${VERSION_MAJOR_REGEX})
string(REGEX REPLACE ${VERSION_MAJOR_REGEX} "\\1" TL_VERSION_MAJOR "${TL_VERSION_MAJOR}")
set(VERSION_MINOR_REGEX "#define TL_VERSION_MINOR[ \t]+(.+)")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../src/thunderloom.h"
    TL_VERSION_MINOR REGEX ${VERSION_MINOR_REGEX})
string(REGEX REPLACE ${VERSION_MINOR_REGEX} "\\1" TL_VERSION_MINOR "${TL_VERSION_MINOR}")
set(VERSION_PATCH_REGEX "#define TL_VERSION_PATCH[ \t]+(.+)")
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../src/thunderloom.h"
    TL_VERSION_PATCH REGEX ${VERSION_PATCH_REGEX})
string(REGEX REPLACE ${VERSION_PATCH_REGEX} "\\1" TL_VERSION_PATCH "${TL_VERSION_PATCH}")

project(thunderloom_frontends)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(RELEASE_DIR ${CMAKE_SOURCE_DIR}/release/${CMAKE_SYSTEM_NAME})

set(MAYA_VERSION "2017" CACHE STRING "Maya version (year)")
set(3DSMAX_VERSIONS 2019 2020 2021 CACHE STRING "3dsMax versions (year)")

set(TL_H_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../src)

IF(WIN32)
add_subdirectory(installer)
add_subdirectory(vray3dsMax)
ENDIF()

add_subdirectory(vray)

add_subdirectory(vraymaya)

add_subdirectory(standalone_pattern_editor)

# Copy V-Ray standalone to Maya plugin, when building for Maya
add_custom_target(vraymaya_copy_vrayplugin ALL
    DEPENDS ${RELEASE_DIR}/V-Ray\ for\ Maya${MAYA_VERSION}/vray_plugin/${CMAKE_SHARED_LIBRARY_PREFIX}vray_thunderLoom${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
add_custom_command(
    OUTPUT ${RELEASE_DIR}/V-Ray\ for\ Maya${MAYA_VERSION}/vray_plugin/${CMAKE_SHARED_LIBRARY_PREFIX}vray_thunderLoom${CMAKE_SHARED_LIBRARY_SUFFIX}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${RELEASE_DIR}/V-Ray/ ${RELEASE_DIR}/V-Ray\ for\ Maya${MAYA_VERSION}/vray_plugin/
    )
add_dependencies(vraymaya_copy_vrayplugin vray_thunderLoom)
add_dependencies(thunderLoomVRayMaya vraymaya_copy_vrayplugin)
