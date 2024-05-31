cmake_minimum_required(VERSION 3.0)

# Print a message that using the Makefiles is recommended.
message(NOTICE: " The CMakeLists.txt is unmaintained. Use the Makefile if possible.")

# Enforce "Debug" as standard build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# CMake project configuration
project(yquake2-ctf)

# Enforce compiler flags (GCC / Clang compatible, yquake2
# won't build with another compiler anyways)
#  -Wall                -> More warnings
#  -fno-strict-aliasing -> Quake 2 is far away from strict aliasing
#  -fwrapv              -> Make signed integer overflows defined
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fno-strict-aliasing -fwrapv")

# Use -O2 as maximum optimization level. -O3 has it's problems with yquake2.
string(REPLACE "-O3" "-O2" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

# Linker Flags
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	list(APPEND CtfLinkerFlags "-lm")
else()
	list(APPEND CtfLinkerFlags "-lm -rdynamic")
endif()

set(Ctf-Source
	src/menu/menu.c
	src/monster/move.c
	src/player/client.c
	src/player/hud.c
	src/player/trail.c
	src/player/view.c
	src/player/weapon.c
	src/shared/shared.c
	src/g_ai.c
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
	)

set(Ctf-Header
	src/header/ctf.h
	src/header/game.h
	src/header/local.h
	src/header/menu.h
	src/header/shared.h
	src/monster/player.h
	)

# Build the ctf dynamic library
add_library(game SHARED ${Ctf-Source} ${Ctf-Header})
set_target_properties(game PROPERTIES
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug
	LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
)
target_link_libraries(game ${CtfLinkerFlags})
