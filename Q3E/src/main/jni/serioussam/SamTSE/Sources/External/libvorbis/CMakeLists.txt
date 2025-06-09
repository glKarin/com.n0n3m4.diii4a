cmake_minimum_required(VERSION 2.8.12)
project(samtse_vorbis)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Required modules
include(GNUInstallDirs)
include(CheckIncludeFiles)
include(CheckLibraryExists)

# Build options

message(STATUS "Configuring ${PROJECT_NAME} ${PROJECT_VERSION}")

# Find math library

check_library_exists(m floor "" HAVE_LIBM)

add_subdirectory(lib)
