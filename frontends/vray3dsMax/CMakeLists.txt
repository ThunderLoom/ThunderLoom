cmake_minimum_required(VERSION 3.12)

IF(NOT WIN32)
    message(FATAL_ERROR "thunderLoom for V-Ray for 3dsMax can only be built on windows!")
ENDIF()

# Additional library directories
set(GLFW_LIB ${PROJECT_SOURCE_DIR}/dependencies/glfw/lib-vc2015)
link_directories(
	${GLFW_LIB}
)

foreach(3DSMAX_VERSION ${3DSMAX_VERSIONS})

	set(3DSMAXSDK_ROOT${3DSMAX_VERSION} $ENV{ADSK_3DSMAX_SDK_${3DSMAX_VERSION}} )
	set(3DSMAXSDK_INCLUDE${3DSMAX_VERSION} "${3DSMAXSDK_ROOT${3DSMAX_VERSION}}/include/" )
	set(3DSMAXSDK_LIB${3DSMAX_VERSION} "${3DSMAXSDK_ROOT${3DSMAX_VERSION}}/lib/x64/Release/" )

	set(VRAY3DSMAX_ROOT${3DSMAX_VERSION} $ENV{VRAY_OSL_PATH_3DSMAX${3DSMAX_VERSION}}/../)
	set(VRAY3DSMAX_INCLUDE${3DSMAX_VERSION} "${VRAY3DSMAX_ROOT${3DSMAX_VERSION}}/include/" )
	set(VRAY3DSMAX_LIB${3DSMAX_VERSION} "${VRAY3DSMAX_ROOT${3DSMAX_VERSION}}/lib/" )

	# Additional includes
	set(TL_H_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../src)
	set(GL3W_SRC ${PROJECT_SOURCE_DIR}/dependencies/gl3w/src)
	set(GL3W_INCLUDE ${PROJECT_SOURCE_DIR}/dependencies/gl3w/include)
	set(GLFW_INCLUDE ${PROJECT_SOURCE_DIR}/dependencies/glfw/include)
	set(IMGUI_INCLUDE ${PROJECT_SOURCE_DIR}/dependencies/imgui)


	ADD_DEFINITIONS(-DUNICODE)
	ADD_DEFINITIONS(-D_UNICODE)
	ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS) # supress MSVC warn C4996

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

	add_library(thunderLoomVRay3dsMax${3DSMAX_VERSION} MODULE ${SOURCES})

	target_include_directories (
		thunderLoomVRay3dsMax${3DSMAX_VERSION}
		PUBLIC
		${TL_H_SOURCE_DIR}
		${GL3W_INCLUDE}
		${GLFW_INCLUDE}
		${IMGUI_INCLUDE}
		${3DSMAXSDK_INCLUDE${3DSMAX_VERSION}}
		${VRAY3DSMAX_INCLUDE${3DSMAX_VERSION}}
	)


	set_target_properties(thunderLoomVRay3dsMax${3DSMAX_VERSION} PROPERTIES SUFFIX ".dlt")

	# Properties->Linker->Input->Additional Dependencies
	target_link_libraries (thunderLoomVRay3dsMax${3DSMAX_VERSION}
		"${3DSMAXSDK_LIB${3DSMAX_VERSION}}Paramblk2.lib"
		"${3DSMAXSDK_LIB${3DSMAX_VERSION}}core.lib"
		"${3DSMAXSDK_LIB${3DSMAX_VERSION}}geom.lib"
		"${3DSMAXSDK_LIB${3DSMAX_VERSION}}maxutil.lib"
		"${3DSMAXSDK_LIB${3DSMAX_VERSION}}bmm.lib"
		"comctl32"
		"odbc32"
		"odbccp32"
		"${VRAY3DSMAX_LIB${3DSMAX_VERSION}}vray${3DSMAX_VERSION}.lib"
		"${VRAY3DSMAX_LIB${3DSMAX_VERSION}}vrender${3DSMAX_VERSION}.lib"

		"${VRAY3DSMAX_LIB${3DSMAX_VERSION}}vutils_s.lib"
		"${VRAY3DSMAX_LIB${3DSMAX_VERSION}}plugman_s.lib"
		"glfw3"
		"opengl32"
		)

	set(DEST thunderLoomVRay3dsMax)
	install(TARGETS thunderLoomVRay3dsMax${3DSMAX_VERSION} DESTINATION ${DEST})
endforeach(3DSMAX_VERSION)
install(FILES "${PROJECT_BINARY_DIR}/PackageContents.xml" DESTINATION ${DEST})
