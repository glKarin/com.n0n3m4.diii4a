make_release_only()

if (MSVC)
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4244" )
endif()

add_library( ${ZMUSIC_PREFIX}miniz OBJECT miniz.c )
target_compile_definitions( ${ZMUSIC_PREFIX}miniz PRIVATE -DMINIZ_NO_STDIO )
target_include_directories( ${ZMUSIC_PREFIX}miniz INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>" )
