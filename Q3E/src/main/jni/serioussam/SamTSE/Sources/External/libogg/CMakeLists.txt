cmake_minimum_required(VERSION 2.8.12)
project(samtse_ogg)

# Required modules
include(GNUInstallDirs)
include(CheckIncludeFiles)

# Build options
option(BUILD_SHARED_LIBS "Build shared library" ON) #k OFF

# Install options

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
)

add_library(samtse_ogg STATIC ${OGG_HEADERS} ${OGG_SOURCES})
target_include_directories(samtse_ogg PRIVATE include ${CMAKE_CURRENT_BINARY_DIR}/include)

# export to parent environment
set(SAMTSE_OGG_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/include" PARENT_SCOPE)
