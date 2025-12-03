cmake_minimum_required(VERSION 3.0)

# Enforce "Debug" as standard build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# CMake project configuration
project(yquake2-3zb2)

# Enforce compiler flags (GCC / Clang compatible, yquake2
# won't build with another compiler anyways)
#  -Wall                -> More warnings
#  -fno-strict-aliasing -> Quake 2 is far away from strict aliasing
#  -fwrapv              -> Make signed integer overflows defined
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fno-strict-aliasing -fwrapv")

# Use -O2 as maximum optimization level. -O3 has it's problems with yquake2.
string(REPLACE "-O3" "-O2" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

# Operating system
add_definitions(-DOSTYPE="${CMAKE_SYSTEM_NAME}")

# Architecture string
string(REGEX REPLACE "amd64" "x86_64" ARCH "${CMAKE_SYSTEM_PROCESSOR}")
string(REGEX REPLACE "i.86" "i386" ARCH "${ARCH}")
string(REGEX REPLACE "^arm.*" "arm" ARCH "${ARCH}")
add_definitions(-DARCH="${ARCH}")

# Linker Flags
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	list(APPEND 3zb2LinkerFlags "-lm")
else()
	list(APPEND 3zb2LinkerFlags "-lm -rdynamic")
endif()

set(3zb2-Source
	src/bot/fire.c
	src/bot/func.c
	src/bot/za.c
	src/bot/bot.c
	src/g_chase.c
	src/g_cmds.c
	src/g_combat.c
	src/g_ctf.c
	src/g_func.c
	src/g_items.c
	src/g_main.c
	src/g_misc.c
	src/g_monster.c
	src/g_phys.c
	src/g_save.c
	src/g_spawn.c
	src/g_svcmds.c
	src/g_target.c
	src/g_trigger.c
	src/g_utils.c
	src/g_weapon.c
	src/monster/move.c
	src/player/client.c
	src/player/hud.c
	src/player/menu.c
	src/player/trail.c
	src/player/view.c
	src/player/weapon.c
	src/shared/shared.c
	)

set(3zb2-Header
	src/header/bot.h
	src/header/botstr.h
	src/header/ctf.h
	src/header/game.h
	src/header/local.h
	src/header/menu.h
	src/header/player.h
	src/header/shared.h
	)

# Build the 3zb2 dynamic library
add_library(q23zb2 SHARED ${3zb2-Source} ${3zb2-Header})
#[[set_target_properties(game PROPERTIES
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug
	LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
)]]
target_link_libraries(q23zb2 ${3zb2LinkerFlags})
