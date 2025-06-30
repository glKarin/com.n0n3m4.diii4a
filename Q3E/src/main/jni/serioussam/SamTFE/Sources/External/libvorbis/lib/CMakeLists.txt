set(VORBIS_PUBLIC_HEADERS
    ../include/vorbis/codec.h
    ../include/vorbis/vorbisenc.h
    ../include/vorbis/vorbisfile.h
)

set(VORBIS_HEADERS
    envelope.h
    lpc.h
    lsp.h
    codebook.h
    misc.h
    psy.h
    masking.h
    os.h
    mdct.h
    smallft.h
    highlevel.h
    registry.h
    scales.h
    window.h
    lookup.h
    lookup_data.h
    codec_internal.h
    backends.h
    bitrate.h
)

set(VORBIS_SOURCES
    mdct.c
    smallft.c
    block.c
    envelope.c
    window.c
    lsp.c
    lpc.c
    analysis.c
    synthesis.c
    psy.c
    info.c
    floor1.c
    floor0.c
    res0.c
    mapping0.c
    registry.c
    codebook.c
    sharedbook.c
    lookup.c
    bitrate.c
)

set(VORBISFILE_SOURCES
    vorbisfile.c
)

set(VORBISENC_SOURCES
    vorbisenc.c
)

if(WIN32)
    list(APPEND VORBIS_SOURCES vorbisenc.c)
endif()

if(WIN32)
    list(APPEND VORBIS_SOURCES ../win32/vorbis.def)
    list(APPEND VORBISENC_SOURCES ../win32/vorbisenc.def)
    list(APPEND VORBISFILE_SOURCES ../win32/vorbisfile.def)
endif()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
endif()

add_library(samtfe_vorbis STATIC ${VORBIS_PUBLIC_HEADERS} ${VORBIS_HEADERS} ${VORBIS_SOURCES} ${VORBISFILE_SOURCES} ${VORBISENC_SOURCES})
target_include_directories(samtfe_vorbis
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/../include
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/../../libogg/include
)
target_link_libraries(samtfe_vorbis samtfe_ogg)
