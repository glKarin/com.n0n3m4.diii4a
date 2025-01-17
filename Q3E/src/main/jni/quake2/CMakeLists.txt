cmake_minimum_required(VERSION 3.4.1)

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_subdirectory (yquake2) # quake2 main and game

# mods
add_subdirectory (ctf) # https://github.com/yquake2/ctf/commit/6a2d5125f6ef4f2bdaa45e5d0979d47409535d84
add_subdirectory (rogue) # https://github.com/yquake2/rogue/commit/830ad45774336194ae77351865f9d5b991be2979
add_subdirectory (xatrix) # https://github.com/yquake2/xatrix/commit/d215802d1642d1a2a4850896f70d37cf94f81a9c
add_subdirectory (zaero) # https://github.com/yquake2/zaero/commit/c3322bec3fb9760fcdc615615a8dcab6d59b6bed
