cmake_minimum_required(VERSION 3.9)
project (xash3d-fwgs-ref-gl)

add_definitions(-DREF_DLL=1)

file(GLOB FWGS_REFGL_INCLUDES *.h)
file(GLOB FWGS_REFGL_SOURCES *.c gl2_shim/*.c)

include_directories(. ../../common ../../pm_shared ../../engine ../../public ../../engine/common ../../filesystem ../../3rdparty/library_suffix/include)

add_library (xash3d_ref_gles1 SHARED ${FWGS_REFGL_SOURCES})
target_link_libraries(xash3d_ref_gles1 fwgs_public fwgs_nanogl log)
set_target_properties(xash3d_ref_gles1 PROPERTIES COMPILE_DEFINITIONS "XASH_NANOGL=1")

#[[
add_library (xash3d_ref_gles2 SHARED ${FWGS_REFGL_SOURCES})
target_link_libraries(xash3d_ref_gles2 fwgs_gl-wes-v2 log)
set_target_properties(xash3d_ref_gles2 PROPERTIES COMPILE_DEFINITIONS "XASH_WES=1")
]]

add_library (xash3d_ref_gl4es SHARED ${FWGS_REFGL_SOURCES})
target_include_directories(xash3d_ref_gl4es PRIVATE ../../3rdparty/gl4es)
target_link_libraries(xash3d_ref_gl4es fwgs_public fwgs_GL)
set_target_properties(xash3d_ref_gl4es PROPERTIES COMPILE_DEFINITIONS "XASH_GL_STATIC=1;XASH_GL4ES=1")

add_library (xash3d_ref_gles3compat SHARED ${FWGS_REFGL_SOURCES})
target_link_libraries(xash3d_ref_gles3compat fwgs_public)
set_target_properties(xash3d_ref_gles3compat PROPERTIES COMPILE_DEFINITIONS "XASH_GLES3COMPAT=1")
