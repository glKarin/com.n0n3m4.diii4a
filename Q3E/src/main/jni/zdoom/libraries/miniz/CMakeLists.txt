make_release_only()

if (MSVC)
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4244" )
endif()

add_library( ${ZDOOM_PREFIX}miniz miniz.c )
target_compile_definitions( ${ZDOOM_PREFIX}miniz PRIVATE -DMINIZ_NO_STDIO )
target_include_directories( ${ZDOOM_PREFIX}miniz INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>" )