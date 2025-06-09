cmake_minimum_required(VERSION 2.8)
project(amp11lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math -Wall")

add_definitions(-DFASTBITS)
add_definitions(-DTIME)
add_definitions(-DUNIX)
add_definitions(-DLINUX)
add_definitions(-DOSS)
add_definitions(-DAMP11LIB_EXPORTS)

set(SOURCE_FILES
    binfile/binfarc.cpp
    binfile/binfarc.h
    binfile/binfile.cpp
    binfile/binfile.h
    binfile/binfstd.cpp
    binfile/binfstd.h
    binfile/ptypes.h
    amp11lib.cpp
    amp11lib.h
    amp1dec.cpp
    amp2dec.cpp
    amp3dec.cpp
    ampdec.cpp
    ampdec.h
    ampsynth.cpp)

add_library(amp11lib SHARED ${SOURCE_FILES})
