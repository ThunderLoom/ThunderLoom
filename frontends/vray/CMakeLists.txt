cmake_minimum_required(VERSION 2.8)

set(CMAKE_MACOSX_RPATH 1)

IF(WIN32 AND (NOT (MSVC_VERSION STREQUAL "1700"))) #VS11
    message(SEND_WARNING "V-Ray requires compiler vc11 (Visual Studio 2012)!")
ENDIF()

project(vray_thunderLoom)

# Varibles
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(RELEASE_DIR ${CMAKE_SOURCE_DIR}/release/${CMAKE_SYSTEM_NAME} CACHE PATH "Release output")
set(LIBRARY_OUTPUT_PATH ${RELEASE_DIR}/V-Ray/)

# Additional includes
set(TL_H_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../src)

set(SOURCES "vray_thunderloom.cpp")
include_directories(${TL_H_SOURCE_DIR})

set(INCLUDE_DIRS "")

set(LIB_DIRS "")

set(LIBS 
    "vray"
    "vutils_s"
    "rayserver_s"
    "putils_s"
    "plugman_s")

# Platform specific libs, files configs
IF (WIN32)
    # "C:/Program Files/Chaos Group/V-Ray/Standalone for x64/include"
    set(INCLUDE_DIRS ${INCLUDE_DIRS} 
        "C:/Program Files/Chaos Group/V-Ray/Maya ${MAYA_VERSION} for x64/include"
    )

    # "C:/Program Files/Chaos Group/V-Ray/Maya 2017 for x64/lib/x64/vc11"
    set(LIB_DIRS ${LIB_DIRS} 
        "C:/Program Files/Chaos Group/V-Ray/Maya ${MAYA_VERSION} for x64/lib/x64/vc11"
    )

    ADD_DEFINITIONS(-DUNICODE)
    ADD_DEFINITIONS(-D_UNICODE)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS) # supress MSVC warn C4996
ENDIF (WIN32)
IF(APPLE)
    # -I/Applications/ChaosGroup/V-Ray/Standalone_for_mavericks_x64/include
    set(INCLUDE_DIRS ${INCLUDE_DIRS} 
        "/Applications/ChaosGroup/V-Ray/Maya${MAYA_VERSION}/include"
        "/usr/local/include")

    # -I"/Applications/ChaosGroup/V-Ray/Standalone_for_mavericks_x64/lib/mavericks_x64/gcc-4.2-cpp"
    set(LIB_DIRS ${LIB_DIRS} 
        "/Applications/ChaosGroup/V-Ray/Maya${MAYA_VERSION}/lib/mavericks_x64/gcc-4.2-cpp"
    )
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


# Load external headers with SYSTEM flag to make supress these warnings.
include_directories(SYSTEM ${INCLUDE_DIRS})

# Load libraries
link_directories(${LIB_DIRS})

# Build
IF (WIN32)
    add_library(vray_thunderLoom SHARED ${SOURCES})
    IF(MSVC)
        # Do not put resulting dll in VisualStudios Debug/Release folders
        set_target_properties( vray_thunderLoom PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH} )
        set_target_properties( vray_thunderLoom PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${LIBRARY_OUTPUT_PATH} )
        set_target_properties( vray_thunderLoom PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${LIBRARY_OUTPUT_PATH} )
        set_target_properties( vray_thunderLoom PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${LIBRARY_OUTPUT_PATH} )
    ENDIF(MSVC)
ENDIF (WIN32)
IF(APPLE)
    add_library(vray_thunderLoom SHARED ${SOURCES})
    set_target_properties(vray_thunderLoom PROPERTIES SUFFIX ".so")
    target_compile_definitions(vray_thunderLoom PRIVATE
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
    target_compile_options(vray_thunderLoom PRIVATE 
        "-std=c++11"
        "-Wall"
        "-Wno-unknown-pragmas"
        "-Wno-unused-function"
        "-Wno-inconsistent-missing-override"
        "-Wno-writable-strings"
        )
endif()

# Link Libs
target_link_libraries(vray_thunderLoom ${LIBS})

add_custom_command(TARGET vray_thunderLoom POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${PROJECT_SOURCE_DIR}/README.md
                ${RELEASE_DIR}/README_V-Ray_Standalone.md
                )
