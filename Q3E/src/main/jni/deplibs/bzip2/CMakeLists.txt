cmake_minimum_required(VERSION 2.8.9)
project(bzip2)

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(BIGFILES "-D_FILE_OFFSET_BITS=64")
set(CMAKE_C_FLAGS "-fpic -fPIC -Wall -Winline -O2 ${BIGFILES}") #  -g

#add_definitions( -DBZ_NO_STDIO )
add_library( bz2 SHARED
    blocksort.c
    bzlib.c
    compress.c
    crctable.c
    decompress.c
    huffman.c
    randtable.c
)
