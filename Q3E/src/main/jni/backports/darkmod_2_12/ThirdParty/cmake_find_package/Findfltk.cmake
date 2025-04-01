include(${CMAKE_CURRENT_LIST_DIR}/tdm_find_package.cmake)

set(fltk_FOUND 1)
set(fltk_INCLUDE_DIRS "${ARTEFACTS_DIR}/fltk/include")
set(fltk_LIBRARY_DIR "${ARTEFACTS_DIR}/fltk/lib/${PACKAGE_PLATFORM}")
if(MSVC)
	set(fltk_LIBRARIES
		"${fltk_LIBRARY_DIR}/fltk.lib"
		"${fltk_LIBRARY_DIR}/fltk_images.lib"
	)
else()
	set(fltk_LIBRARIES
		"${fltk_LIBRARY_DIR}/libfltk.a"
		"${fltk_LIBRARY_DIR}/libfltk_images.a"
	)
endif()
