cmake_minimum_required(VERSION 2.8.9)
project(sqlite)

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(SQLITE_SOURCES
  "sqlite3.c"
  )

set(SQLITE_HEADERS
  "sqlite3"
  "sqlite3ext.h"
  )

add_library(sqlite SHARED ${SQLITE_SOURCES} ${SQLITE_HEADERS})
