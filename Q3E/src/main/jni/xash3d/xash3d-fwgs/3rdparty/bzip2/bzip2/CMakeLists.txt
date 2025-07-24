cmake_minimum_required(VERSION 3.12)

project(fwgs-bzip2
        VERSION 1.1.0
        DESCRIPTION "This Bzip2/libbz2 a program and library for lossless block-sorting data compression."
        LANGUAGES C)

# See versioning rule:
#  http://www.gnu.org/software/libtool/manual/html_node/Updating-version-info.html
#
# KEEP THESE IN SYNC WITH meson.build OR STUFF WILL BREAK!
set(LT_CURRENT  1)
set(LT_REVISION 9)
set(LT_AGE      0)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
include(Version)
include(SymLink)

set(BZ_VERSION ${PROJECT_VERSION})
configure_file (
    ${PROJECT_SOURCE_DIR}/bz_version.h.in
    ${PROJECT_BINARY_DIR}/bz_version.h
)
include_directories(${PROJECT_BINARY_DIR})

set(BIGFILES "-D_FILE_OFFSET_BITS=64")
set(CMAKE_C_FLAGS "-fpic -fPIC -Wall -Winline -O2 ${BIGFILES}") #  -g


#add_definitions( -DBZ_NO_STDIO )
add_library( fwgs_bzip2 STATIC
    blocksort.c
    bzlib.c
    compress.c
    crctable.c
    decompress.c
    huffman.c
    randtable.c
)
