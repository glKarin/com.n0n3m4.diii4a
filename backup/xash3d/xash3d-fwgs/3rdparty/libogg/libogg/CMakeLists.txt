cmake_minimum_required(VERSION 3.0)

project(fwgs-ogg VERSION ${CMAKE_MATCH_1} LANGUAGES C)

# Required modules
include(GNUInstallDirs)
include(CheckIncludeFiles)
include(CMakePackageConfigHelpers)

# Configure config_type.h
check_include_files(inttypes.h INCLUDE_INTTYPES_H)
check_include_files(stdint.h INCLUDE_STDINT_H)
check_include_files(sys/types.h INCLUDE_SYS_TYPES_H)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(SIZE16 int16_t)
set(USIZE16 uint16_t)
set(SIZE32 int32_t)
set(USIZE32 uint32_t)
set(SIZE64 int64_t)
set(USIZE64 uint64_t)

include(CheckSizes)

configure_file(include/ogg/config_types.h.in include/ogg/config_types.h @ONLY)

set(OGG_HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/include/ogg/config_types.h
    include/ogg/ogg.h
    include/ogg/os_types.h
)

set(OGG_SOURCES
    src/bitwise.c
    src/framing.c
    src/crctable.h
)

add_library(fwgs_ogg STATIC ${OGG_HEADERS} ${OGG_SOURCES})
target_include_directories(fwgs_ogg PUBLIC
${CMAKE_CURRENT_SOURCE_DIR}/include
${CMAKE_CURRENT_BINARY_DIR}/include
)
