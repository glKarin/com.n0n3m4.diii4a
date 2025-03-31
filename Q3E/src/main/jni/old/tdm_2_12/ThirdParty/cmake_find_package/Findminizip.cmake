include(${CMAKE_CURRENT_LIST_DIR}/tdm_find_package.cmake)

set(minizip_FOUND 1)
set(minizip_INCLUDE_DIRS "${ARTEFACTS_DIR}/minizip/include")
set(minizip_LIBRARY_DIR "${ARTEFACTS_DIR}/minizip/lib/${PACKAGE_PLATFORM}")
if(MSVC)
	set(minizip_LIBRARIES
		"${minizip_LIBRARY_DIR}/minizip.lib"
	)
else()
	set(minizip_LIBRARIES
		"${minizip_LIBRARY_DIR}/libminizip.a"
	)
endif()
