cmake_minimum_required(VERSION 3.9)
project (xash3d-fwgs-ref-gl)

add_definitions(-DREF_DLL=1)

file(GLOB FWGS_REFSOFT_INCLUDES *.h)
file(GLOB FWGS_REFSOFT_SOURCES *.c)

include_directories(. ../../common ../../pm_shared ../../engine ../../public ../../engine/common ../../filesystem ../../3rdparty/library_suffix/include)

add_library (xash3d_ref_soft SHARED ${FWGS_REFSOFT_SOURCES})
target_link_libraries(xash3d_ref_soft fwgs_public)

