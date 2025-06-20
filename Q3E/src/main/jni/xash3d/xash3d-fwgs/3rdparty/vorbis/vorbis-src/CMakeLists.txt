cmake_minimum_required(VERSION 2.8.12)
project(fwgs-vorbis)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Required modules
include(CTest)
include(GNUInstallDirs)
include(CheckIncludeFiles)
include(CheckLibraryExists)

# Find math library

check_library_exists(m floor "" HAVE_LIBM)
if(HAVE_LIBM)
    set(VORBIS_LIBS "-lm")
endif()

add_subdirectory(lib)
