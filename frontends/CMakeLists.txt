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
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/out/install/${CMAKE_SYSTEM_NAME} CACHE PATH "")


set(BUILD_MAYA FALSE CACHE BOOL "Build Maya frontend")
set(BUILD_3DSMAX FALSE CACHE BOOL "Build 3dsMax frontend")
set(BUILD_VRAYSTANDALONE FALSE CACHE BOOL "Build VRay Standalone frontend")

set(MAYA_VERSIONS 2019 CACHE STRING "Maya versions (year)")
set(3DSMAX_VERSIONS 2019 2020 2021 CACHE STRING "3dsMax versions (year)")

set(TL_H_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../src)

add_subdirectory(standalone_pattern_editor)

IF(WIN32)
    IF(BUILD_3DSMAX)
        add_subdirectory(installer)
        add_subdirectory(vray3dsMax)
    ENDIF()
ENDIF()

IF(BUILD_VRAYSTANDALONE OR BUILD_MAYA)
    add_subdirectory(vray)
ENDIF()

IF(BUILD_MAYA)
    add_subdirectory(vraymaya)
ENDIF()
