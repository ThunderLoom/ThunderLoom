cmake_minimum_required(VERSION 3.11)

project(thunderloom_pattern_editor)

# Additional includes
set(TL_H_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../src)
set(GL3W_SRC ${PROJECT_SOURCE_DIR}/../dependencies/gl3w/src)
set(GL3W_INCLUDE ${PROJECT_SOURCE_DIR}/../dependencies/gl3w/include)
set(GLFW_INCLUDE ${PROJECT_SOURCE_DIR}/../dependencies/glfw/include)
set(IMGUI_INCLUDE ${PROJECT_SOURCE_DIR}/../dependencies/imgui)

set(SOURCES
    "main.cpp"
    "${GL3W_SRC}/gl3w.c"
    "${IMGUI_INCLUDE}/imgui.cpp"
    "${IMGUI_INCLUDE}/imgui_draw.cpp"
    "${IMGUI_INCLUDE}/imgui_impl_glfw_gl3.cpp")

set(INCLUDE_DIRS ${TL_H_SOURCE_DIR}
    ${GL3W_INCLUDE}
    ${GLFW_INCLUDE}
    ${IMGUI_INCLUDE})

set(LIB_DIRS "")

set(LIBS "")

# Platform specific libs and files
IF (WIN32)
    set(LIB_DIRS ${LIB_DIRS} 
        ${PROJECT_SOURCE_DIR}/../dependencies/glfw/lib-vc2015)
    set(LIBS ${LIBS} 
        "glfw3"
        "opengl32")
ENDIF (WIN32)
IF(APPLE)
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
    add_executable(thunderloom_pattern_editor WIN32 ${SOURCES})
ENDIF (WIN32)
IF(APPLE)
    add_executable(thunderloom_pattern_editor MACOSX_BUNDLE ${SOURCES})
ENDIF(APPLE)

# Link Libs
target_link_libraries(thunderloom_pattern_editor ${LIBS})

# Set warnings
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    target_compile_options(thunderloom_pattern_editor PRIVATE 
        "-Wall"
        "-Wno-unused-function"
        )
endif()

# Install
install(TARGETS thunderloom_pattern_editor  DESTINATION thunderloom_pattern_editor)
install(FILES $<TARGET_PDB_FILE:thunderloom_pattern_editor> DESTINATION pdbs OPTIONAL)
