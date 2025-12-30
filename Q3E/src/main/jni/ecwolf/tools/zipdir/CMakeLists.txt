cmake_minimum_required( VERSION 3.5.0 )

if(NOT CMAKE_CROSSCOMPILING)
	add_executable( zipdir
		zipdir.c )
	target_link_libraries( zipdir ${ZLIB_LIBRARIES} ${BZIP2_LIBRARIES} lzma )
	target_include_directories( zipdir PRIVATE "${ZLIB_INCLUDE_DIR}" "${BZIP2_INCLUDE_DIR}" "${LZMA_INCLUDE_DIR}" )
	set( CROSS_EXPORTS ${CROSS_EXPORTS} zipdir PARENT_SCOPE )
endif()
