cmake_minimum_required(VERSION 2.8)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(RELEASE_DIR ${CMAKE_SOURCE_DIR}/release/${CMAKE_SYSTEM_NAME})

set(3DSMAX_VERSION "2017" CACHE STRING "3dsMax version (year)")
set(MAYA_VERSION "2017" CACHE STRING "Maya version (year)")

IF(WIN32)
add_subdirectory(vray3dsMax)
ENDIF()

add_subdirectory(vray)

add_subdirectory(vraymaya)

add_subdirectory(standalone_pattern_editor)



# Copy V-Ray standalone to Maya plugin, when building for Maya
add_custom_target(vraymaya_copy_vrayplugin ALL
    DEPENDS ${RELEASE_DIR}/V-Ray\ for\ Maya${MAYA_VERSION}/vray_plugin/${CMAKE_SHARED_LIBRARY_PREFIX}thunderLoomVRay${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
add_custom_command(
    OUTPUT ${RELEASE_DIR}/V-Ray\ for\ Maya${MAYA_VERSION}/vray_plugin/${CMAKE_SHARED_LIBRARY_PREFIX}thunderLoomVRay${CMAKE_SHARED_LIBRARY_SUFFIX}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${RELEASE_DIR}/V-Ray/ ${RELEASE_DIR}/V-Ray\ for\ Maya${MAYA_VERSION}/vray_plugin/
    )
add_dependencies(vraymaya_copy_vrayplugin thunderLoomVRay)
add_dependencies(thunderLoomVRayMaya vraymaya_copy_vrayplugin)
