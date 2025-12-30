cmake_minimum_required( VERSION 2.8.12 )

if(LIBRETRO AND NOT LIBRETRO_STATIC)
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

if(POLICY CMP0054)
	cmake_policy(SET CMP0054 NEW)
endif()

if( "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang" )
	set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fomit-frame-pointer" )
endif()

add_definitions( -DBZ_NO_STDIO )
add_library( ${ECWOLF_PREFIX}bz2 STATIC
    blocksort.c
    bzlib.c
    compress.c
    crctable.c
    decompress.c
    huffman.c
    randtable.c )
target_link_libraries( ${ECWOLF_PREFIX}bz2 )
