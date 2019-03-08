cmake_minimum_required(VERSION 2.8)

option(USE_CUDA "Enable realtime preview using CUDA" OFF)
if(USE_CUDA)
project(patternEditor LANGUAGES C CXX CUDA)
else()
project(patternEditor LANGUAGES C CXX)
endif()

# Varibles
#set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
#set(RELEASE_DIR ${CMAKE_SOURCE_DIR}/release/${CMAKE_SYSTEM_NAME} CACHE PATH "Release output")
#set(EXECUTABLE_OUTPUT_PATH ${RELEASE_DIR}/PatternEditor/)

# Additional includes
set(TL_H_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../src)
set(GL3W_SRC ${PROJECT_SOURCE_DIR}/../dependencies/gl3w/src)
set(GL3W_INCLUDE ${PROJECT_SOURCE_DIR}/../dependencies/gl3w/include)
set(GLFW_INCLUDE ${PROJECT_SOURCE_DIR}/../dependencies/glfw/include)
set(IMGUI_INCLUDE ${PROJECT_SOURCE_DIR}/../dependencies/imgui)

if(USE_CUDA)
    set(RAYTRACER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../raytracer/raytracer.cu")
else()
    set(RAYTRACER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../raytracer/raytracer.c")
endif()
set(SOURCES
    "main.cpp"
    ${RAYTRACER_SRC}
    "${GL3W_SRC}/gl3w.c"
    "${IMGUI_INCLUDE}/imgui.cpp"
    "${IMGUI_INCLUDE}/imgui_draw.cpp"
    "${IMGUI_INCLUDE}/imgui_impl_glfw_gl3.cpp"
)

set(INCLUDE_DIRS ${TL_H_SOURCE_DIR}
    ${GL3W_INCLUDE}
    ${GLFW_INCLUDE}
    ${IMGUI_INCLUDE}
	${CMAKE_CURRENT_SOURCE_DIR}/../raytracer/
	)

set(LIB_DIRS "")

set(LIBS "")

# Platform specific libs and files
IF (WIN32)
    set(LIB_DIRS ${LIB_DIRS} 
        ${PROJECT_SOURCE_DIR}/../dependencies/glfw/lib-vc2012)
    set(LIBS ${LIBS} 
        "glfw3"
        "opengl32")
ENDIF (WIN32)
IF(APPLE)
    set(LIB_DIRS "/usr/local/lib")
    set(SOURCES ${SOURCES} macos_wrapper.mm)
    set(INCLUDE_DIRS ${INCLUDE_DIRS}
        "/usr/local/include")
    set(LIBS ${LIBS} 
        "glfw"
        "stdc++"
        "-framework Cocoa")
ENDIF(APPLE)

include_directories (${INCLUDE_DIRS})
link_directories(${LIB_DIRS})

# Build
IF (WIN32)
    add_executable(patternEditor WIN32 ${SOURCES})
ENDIF (WIN32)
IF(APPLE)
    add_executable(patternEditor MACOSX_BUNDLE ${SOURCES})
ENDIF(APPLE)

# Link Libs
target_link_libraries(patternEditor ${LIBS})

# Set warnings
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(patternEditor PRIVATE 
        "-Wall"
        "-Wno-unused-function"
        )
endif()

install(TARGETS patternEditor DESTINATION pattern_editor)
