cmake_minimum_required( VERSION 2.8.7 )

use_fast_math()
require_stricmp()
require_strnicmp()

if( ZD_CMAKE_COMPILER_IS_GNUC_COMPATIBLE )
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
endif()

include( CheckFunctionExists )

if( NOT WIN32 AND NOT APPLE )
	find_package( Threads )
endif()

find_package( ALSA )
if (WIN32 OR ALSA_FOUND)
    add_definitions( -DHAVE_SYSTEM_MIDI )
endif()

if( DYN_SNDFILE)
	add_definitions( -DHAVE_SNDFILE -DDYN_SNDFILE )
else()
	find_package( SndFile )

	if( SNDFILE_FOUND )
		add_definitions( -DHAVE_SNDFILE )
	endif()
endif()

if( DYN_MPG123)
	add_definitions( -DHAVE_MPG123 -DDYN_MPG123 )
else()
	find_package( MPG123 )

	if( MPG123_FOUND )
		add_definitions( -DHAVE_MPG123 )
	endif()
endif()

if( DYN_FLUIDSYNTH )
	add_definitions( -DHAVE_FLUIDSYNTH -DDYN_FLUIDSYNTH )
else()
	find_package( FluidSynth )

	if( FLUIDSYNTH_FOUND )
		add_definitions( -DHAVE_FLUIDSYNTH )
	endif()
endif()


if( WIN32 )
	add_definitions( -D_WIN32 )
	set( SYS_LIBS winmm )
else()
	set( SYS_LIBS ${CMAKE_DL_LIBS} )
endif() 

include_directories( "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/zmusic" "${CMAKE_CURRENT_SOURCE_DIR}/../include" "${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/dumb/include" "${ZLIB_INCLUDE_DIR}" "${ADL_INCLUDE_DIR}" "${OPN_INCLUDE_DIR}" "${TIMIDITYPP_INCLUDE_DIR}" "${TIMIDITY_INCLUDE_DIR}" "${WILDMIDI_INCLUDE_DIR}" "${OPLSYNTH_INCLUDE_DIR}" "${GME_INCLUDE_DIR}" )

if (WIN32)
    set( PLAT_SOURCES
        mididevices/music_win_mididevice.cpp
        musicformats/win32/i_cd.cpp
        musicformats/win32/helperthread.cpp
    )
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    if (ALSA_FOUND)
        set( PLAT_SOURCES
            mididevices/music_alsa_mididevice.cpp
            mididevices/music_alsa_state.cpp
        )
    endif()
endif()


file( GLOB HEADER_FILES
	zmusic/*.h
	loader/*.h
	mididevices/*.h
	midisources/*.h
	musicformats/*.h
	musicformats/win32/*.h
	decoder/*.h
	streamsources/*.h
	../thirdparty/*.h
	../include/*.h
	)
set( all_files
	${HEADER_FILES}
	loader/i_module.cpp
	mididevices/music_base_mididevice.cpp
	mididevices/music_adlmidi_mididevice.cpp
	mididevices/music_opl_mididevice.cpp
	mididevices/music_opnmidi_mididevice.cpp
	mididevices/music_timiditypp_mididevice.cpp
	mididevices/music_fluidsynth_mididevice.cpp
	mididevices/music_softsynth_mididevice.cpp
	mididevices/music_timidity_mididevice.cpp
	mididevices/music_wildmidi_mididevice.cpp
	mididevices/music_wavewriter_mididevice.cpp
	midisources/midisource.cpp
	midisources/midisource_mus.cpp
	midisources/midisource_smf.cpp
	midisources/midisource_hmi.cpp
	midisources/midisource_xmi.cpp
	midisources/midisource_mids.cpp
	streamsources/music_dumb.cpp
	streamsources/music_gme.cpp
	streamsources/music_libsndfile.cpp
	streamsources/music_opl.cpp
	streamsources/music_xa.cpp
	musicformats/music_stream.cpp
	musicformats/music_midi.cpp
	musicformats/music_cd.cpp
	decoder/sounddecoder.cpp
	decoder/sndfile_decoder.cpp
	decoder/mpg123_decoder.cpp
	zmusic/configuration.cpp
	zmusic/zmusic.cpp
	zmusic/critsec.cpp
	loader/test.c
	${PLAT_SOURCES}
	)

option( BUILD_SHARED_LIBS "Build shared libraries" ON )

add_library( zmusic ${all_files} )
add_library( zmusiclite ${all_files} )
target_compile_definitions(zmusiclite PRIVATE ZMUSIC_LITE=1)

if( MSVC )
	option( GENERATE_MAPFILE "Generate .map file for debugging." OFF )

	if( GENERATE_MAPFILE )
		set( LINKERSTUFF "${LINKERSTUFF} /MAP" )
		set_target_properties(zmusic PROPERTIES LINK_FLAGS ${LINKERSTUFF})
		set_target_properties(zmusiclite PROPERTIES LINK_FLAGS ${LINKERSTUFF})
	endif()
 
endif()
	

target_link_libraries( zmusic "${SYS_LIBS}" adl dumb gme oplsynth opn timidity timidityplus wildmidi "${ZLIB_LIBRARIES}" )
target_link_libraries( zmusiclite "${SYS_LIBS}" dumb gme "${ZLIB_LIBRARIES}" )

if( NOT DYN_SNDFILE AND SNDFILE_FOUND )
	include_directories( "${SNDFILE_INCLUDE_DIRS}" )
	target_link_libraries( zmusic ${SNDFILE_LIBRARIES} )
	target_link_libraries( zmusiclite ${SNDFILE_LIBRARIES} )
endif()

if( NOT DYN_MPG123 AND MPG123_FOUND )
	include_directories( "${MPG123_INCLUDE_DIR}" )
	target_link_libraries( zmusic ${MPG123_LIBRARIES} )
	target_link_libraries( zmusiclite ${MPG123_LIBRARIES} )
endif()

if( NOT DYN_FLUIDSYNTH AND FLUIDSYNTH_FOUND )
	include_directories( "${FLUIDSYNTH_INCLUDE_DIR}" )
	target_link_libraries( zmusic ${FLUIDSYNTH_LIBRARIES} )
	target_link_libraries( zmusiclite ${FLUIDSYNTH_LIBRARIES} )
endif()

if(ALSA_FOUND)
	include_directories( "${ALSA_INCLUDE_DIR}" )
	target_link_libraries( zmusic ${ALSA_LIBRARIES} )
	target_link_libraries( zmusiclite ${ALSA_LIBRARIES} )
endif()

if(APPLE)
	set_target_properties(zmusic PROPERTIES MACOSX_RPATH TRUE)
	set_target_properties(zmusiclite PROPERTIES MACOSX_RPATH TRUE)
endif()

if(NOT ANDROID)
set_target_properties(zmusic zmusiclite PROPERTIES
	PUBLIC_HEADER ../include/zmusic.h
	VERSION ${ZMUSIC_VERSION}
	SOVERSION ${ZMUSIC_VERSION_MAJOR}
)

include(GNUInstallDirs)

install(TARGETS zmusic zmusiclite
	PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
else()
set_target_properties(zmusic zmusiclite PROPERTIES
	PUBLIC_HEADER ../include/zmusic.h
)
endif()

source_group("MIDI Devices" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/mididevices/.+")
source_group("MIDI Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/midisources/.+")
source_group("Music Formats" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/musicformats/.+")
source_group("Music Formats\\Win32" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/musicformats/win32/.+")
source_group("ZMusic Core" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/zmusic/.+")
source_group("Sound Decoding" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/decoder/.+")
source_group("Stream Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/streamsources/.+")
source_group("Third Party" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/.+")
source_group("Public Interface" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/../include/.+")
