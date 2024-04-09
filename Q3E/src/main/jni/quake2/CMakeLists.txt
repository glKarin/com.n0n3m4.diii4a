cmake_minimum_required(VERSION 3.4.1)

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_subdirectory (src) # quake2 main

# mods
add_subdirectory (ctf)
add_subdirectory (rogue)
add_subdirectory (xatrix)
add_subdirectory (zaero)
