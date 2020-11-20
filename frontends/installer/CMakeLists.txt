cmake_minimum_required(VERSION 3.11)

project(installer)

set(PRODUCT_GUID_NAMESPACE "DF9F7430-86AE-47A7-A832-3C14DF03B3CB")
string(
    UUID PRODUCT_GUID 
    NAMESPACE ${PRODUCT_GUID_NAMESPACE} 
    NAME "${TL_VERSION_MAJOR}.${TL_VERSION_MINOR}.${TL_VERSION_PATCH}"
    TYPE SHA1 UPPER
)

set(WIX_BIN_PATH "C:/Program Files (x86)/WiX Toolset v3.11/bin" CACHE PATH "Path to WiX installer toolset bin dir.")

file(STRINGS "PackageContents_per_3ds_max_version.xml.in" PC_PER_3DS_MAX )
file(STRINGS "thunderloom_per_3ds_max_version.wxs.in" WX_PER_3DS_MAX )
foreach(3DSMAX_VERSION ${3DSMAX_VERSIONS})

string(CONFIGURE "${PC_PER_3DS_MAX}" PC_3DS_MAX_LINE)
string(REPLACE ";" "\n" PC_3DS_MAX_LINE "${PC_3DS_MAX_LINE}")
set(PC_3DS_MAX "${PC_3DS_MAX}\n${PC_3DS_MAX_LINE}")

string(CONFIGURE "${WX_PER_3DS_MAX}" WX_3DS_MAX_LINE)
string(REPLACE ";" "\n" WX_3DS_MAX_LINE "${WX_3DS_MAX_LINE}")
set(WX_3DS_MAX "${WX_3DS_MAX}\n${WX_3DS_MAX_LINE}")

endforeach(3DSMAX_VERSION)

configure_file (
    "make_installer.bat.in"
    "${PROJECT_BINARY_DIR}/make_installer.bat"
)
configure_file (
    "PackageContents_3ds_max.xml.in"
    "${CMAKE_BINARY_DIR}/PackageContents_3ds_max.xml"
)
configure_file (
    "thunderloom.wxs.in"
    "${PROJECT_BINARY_DIR}/thunderloom.wxs"
)
install(DIRECTORY data/ DESTINATION thunderloom_installer)
install(FILES "${PROJECT_BINARY_DIR}/thunderloom.wxs" DESTINATION thunderloom_installer)
install(FILES "${PROJECT_BINARY_DIR}/make_installer.bat" DESTINATION thunderloom_installer)
