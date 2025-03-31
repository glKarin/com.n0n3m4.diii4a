include(${CMAKE_CURRENT_LIST_DIR}/tdm_find_package.cmake)

set(zlib_FOUND 1)
set(zlib_INCLUDE_DIRS "${ARTEFACTS_DIR}/zlib/include")
set(zlib_LIBRARY_DIR "${ARTEFACTS_DIR}/zlib/lib/${PACKAGE_PLATFORM}")
if(MSVC)
	set(zlib_LIBRARIES
		"${zlib_LIBRARY_DIR}/zlib.lib"
	)
else()
	set(zlib_LIBRARIES
		"${zlib_LIBRARY_DIR}/libz.a"
	)
endif()
