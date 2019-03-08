cmake_minimum_required(VERSION 2.8)

project(raytracer LANGUAGES C CUDA)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src)
set(SOURCES main.c raytracer.cu)

add_executable(raytracer ${SOURCES})
