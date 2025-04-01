include(${CMAKE_CURRENT_LIST_DIR}/tdm_find_package.cmake)

set(vorbis_FOUND 1)
set(vorbis_INCLUDE_DIRS "${ARTEFACTS_DIR}/vorbis/include")
set(vorbis_LIBRARY_DIR "${ARTEFACTS_DIR}/vorbis/lib/${PACKAGE_PLATFORM}")
if(MSVC)
	set(vorbis_LIBRARIES
		"${vorbis_LIBRARY_DIR}/vorbisfile.lib"
		"${vorbis_LIBRARY_DIR}/vorbis.lib"
		"${vorbis_LIBRARY_DIR}/vorbisenc.lib"
	)
else()
	set(vorbis_LIBRARIES
		"${vorbis_LIBRARY_DIR}/libvorbisfile.a"
		"${vorbis_LIBRARY_DIR}/libvorbis.a"
		"${vorbis_LIBRARY_DIR}/libvorbisenc.a"
	)
endif()
