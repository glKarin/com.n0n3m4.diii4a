cmake_minimum_required(VERSION 3.9)
project (xash3d-fwgs-nonogl)

file(GLOB FWGS_NANOGL_INCLUDES *.h)
file(GLOB FWGS_NANOGL_SOURCES *.cpp)
add_library (fwgs_nanogl STATIC ${FWGS_NANOGL_SOURCES})
target_include_directories(fwgs_nanogl PRIVATE . GL)
target_link_libraries(fwgs_nanogl log)
set_target_properties(fwgs_nanogl PROPERTIES COMPILE_DEFINITIONS "__MULTITEXTURE_SUPPORT__=1;NANOGL_MANGLE_PREPEND=1;REF_DLL")
