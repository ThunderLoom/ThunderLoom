cmake_minimum_required(VERSION 3.12)

project(thunderLoomVRay3dsMax)
IF(NOT WIN32)
    message(FATAL_ERROR "thunderLoom for V-Ray for 3dsMax can only be built on windows!")
ENDIF()

# Additional library directories
set(GLFW_LIB ${PROJECT_SOURCE_DIR}/../dependencies/glfw/lib-vc2015)
link_directories(
	${GLFW_LIB}
)

set(3DSMAX_DYNAMIC_LOAD false CACHE STRING "Compile to dynamic dll (for development)")

# Additional includes
set(TL_H_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../src)
set(GL3W_SRC ${PROJECT_SOURCE_DIR}/../dependencies/gl3w/src)
set(GL3W_INCLUDE ${PROJECT_SOURCE_DIR}/../dependencies/gl3w/include)
set(GLFW_INCLUDE ${PROJECT_SOURCE_DIR}/../dependencies/glfw/include)
set(IMGUI_INCLUDE ${PROJECT_SOURCE_DIR}/../dependencies/imgui)



set (SRC
	"3dsMax/3dsMaxThunderLoom.cpp"
	"3dsMax/vrayblinnmtl.rc"
	"3dsMax/DllEntry.cpp"
	"3dsMax/VrayThunderLoomBRDF.cpp"
	"3dsMax/plugin.def"
	"3dsMax/dynamic_load.c"
	)

set (DYNAMIC_LOAD_SOURCES
	"3dsMax/Eval.cpp"
	"${GL3W_SRC}/gl3w.c"
	"${IMGUI_INCLUDE}/imgui.cpp"
	"${IMGUI_INCLUDE}/imgui_draw.cpp"
	"${IMGUI_INCLUDE}/imgui_impl_glfw_gl3.cpp"
)


if(3DSMAX_DYNAMIC_LOAD)
	add_compile_definitions(MAX_DYNAMIC_LOAD)
else()
	list(APPEND SRC ${DYNAMIC_LOAD_SOURCES})
endif()

foreach(3DSMAX_VERSION ${3DSMAX_VERSIONS})

	set(3DSMAXSDK_ROOT${3DSMAX_VERSION} $ENV{ADSK_3DSMAX_SDK_${3DSMAX_VERSION}} )
	set(3DSMAXSDK_INCLUDE${3DSMAX_VERSION} "${3DSMAXSDK_ROOT${3DSMAX_VERSION}}/include/" )
	set(3DSMAXSDK_LIB${3DSMAX_VERSION} "${3DSMAXSDK_ROOT${3DSMAX_VERSION}}/lib/x64/Release/" )

	set(VRAY3DSMAX_ROOT${3DSMAX_VERSION} $ENV{VRAY_OSL_PATH_3DSMAX${3DSMAX_VERSION}}/../)
	set(VRAY3DSMAX_INCLUDE${3DSMAX_VERSION} "${VRAY3DSMAX_ROOT${3DSMAX_VERSION}}/include/" )
	set(VRAY3DSMAX_LIB${3DSMAX_VERSION} "${VRAY3DSMAX_ROOT${3DSMAX_VERSION}}/lib/" )

	ADD_DEFINITIONS(-DUNICODE)
	ADD_DEFINITIONS(-D_UNICODE)
	ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS) # supress MSVC warn C4996

	set(INCLUDES 
		${TL_H_SOURCE_DIR}
		${GL3W_INCLUDE}
		${GLFW_INCLUDE}
		${IMGUI_INCLUDE}
		${3DSMAXSDK_INCLUDE${3DSMAX_VERSION}}
		${VRAY3DSMAX_INCLUDE${3DSMAX_VERSION}}
	)

	set(LIBS 
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

	add_library(thunderLoomVRay3dsMax${3DSMAX_VERSION} MODULE ${SRC})
	target_link_libraries (thunderLoomVRay3dsMax${3DSMAX_VERSION} ${LIBS})
	target_include_directories ( thunderLoomVRay3dsMax${3DSMAX_VERSION} PUBLIC ${INCLUDES})

	set_target_properties(thunderLoomVRay3dsMax${3DSMAX_VERSION} PROPERTIES SUFFIX ".dlt")
	target_compile_definitions(thunderLoomVRay3dsMax${3DSMAX_VERSION} PUBLIC TL_MAX_VERSION="${3DSMAX_VERSION}")

	if(3DSMAX_DYNAMIC_LOAD)
		add_library(thunderLoomVRay3dsMax${3DSMAX_VERSION}_dynamic MODULE ${DYNAMIC_LOAD_SOURCES})
		target_link_libraries(thunderLoomVRay3dsMax${3DSMAX_VERSION}_dynamic ${LIBS})
		target_include_directories(thunderLoomVRay3dsMax${3DSMAX_VERSION}_dynamic PUBLIC ${INCLUDES})

		string(RANDOM PDB_ID)
		set_target_properties(thunderLoomVRay3dsMax${3DSMAX_VERSION}_dynamic PROPERTIES PDB_NAME "dynamic_${3DSMAX_VERSION}_${PDB_ID}")

		install(TARGETS thunderLoomVRay3dsMax${3DSMAX_VERSION}_dynamic DESTINATION ${DEST})
	endif()

	install(TARGETS thunderLoomVRay3dsMax${3DSMAX_VERSION} DESTINATION ${DEST})



endforeach(3DSMAX_VERSION)

#NOTE(Vidar): This file is created in the installer project
install(FILES "${CMAKE_BINARY_DIR}/PackageContents_3ds_max.xml" DESTINATION ${DEST} RENAME "PackageContents.xml")
