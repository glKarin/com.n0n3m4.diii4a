cmake_minimum_required(VERSION 3.4.1)

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

OPTION(YQUAKE2_BUILD_VULKAN_BACKEND "Build ref_vk.so" ON)

add_subdirectory (yquake2) # quake2 main and game

# mods
add_subdirectory (ctf) # https://github.com/yquake2/ctf/commit/a1ea241e4fd063bceacd14d8aa28ea8a04615e66
add_subdirectory (rogue) # https://github.com/yquake2/rogue/commit/587fe971e25578ed20f07a5113cefdfb57c86db0
add_subdirectory (xatrix) # https://github.com/yquake2/xatrix/commit/1d7128ee9db698b3c395f8fca520e179437721ee
add_subdirectory (zaero) # https://github.com/yquake2/zaero/commit/fb4142089d18ed95200ef76fadb808fcf0284d8d
add_subdirectory (3zb2) # https://github.com/yquake2/zaero/commit/47b93a284e20309fe7cbef7a616419fa90a41e61

if(YQUAKE2_BUILD_VULKAN_BACKEND)
add_subdirectory (ref_vk) # -1.0.8
endif()
