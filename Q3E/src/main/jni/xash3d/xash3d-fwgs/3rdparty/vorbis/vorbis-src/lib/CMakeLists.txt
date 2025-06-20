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

add_library(fwgs_vorbis STATIC ${VORBIS_HEADERS} ${VORBIS_SOURCES})
add_library(fwgs_vorbisenc STATIC ${VORBISENC_SOURCES})
add_library(fwgs_vorbisfile STATIC ${VORBISFILE_SOURCES})

target_include_directories(fwgs_vorbis
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/../include
   PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(fwgs_vorbisenc
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/../include
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(fwgs_vorbisfile
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(fwgs_vorbis
	PUBLIC fwgs_ogg
	PRIVATE $<$<BOOL:${HAVE_LIBM}>:m>
)
target_link_libraries(fwgs_vorbisenc PUBLIC fwgs_vorbis)
target_link_libraries(fwgs_vorbisfile PUBLIC fwgs_vorbis)
