cmake_minimum_required(VERSION 3.0)

# Print a message that using the Makefiles is recommended.
message(NOTICE: " The CMakeLists.txt is unmaintained. Use the Makefile if possible.")

# Enforce "Debug" as standard build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# CMake project configuration
project(q2rogue)

# Enforce compiler flags (GCC / Clang compatible, yquake2
# won't build with another compiler anyways)
#  -Wall                -> More warnings
#  -fno-strict-aliasing -> Quake 2 is far away from strict aliasing
#  -fwrapv              -> Make signed integer overflows defined
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fno-strict-aliasing -fwrapv")

# Use -O2 as maximum optimization level. -O3 has it's problems with yquake2.
string(REPLACE "-O3" "-O2" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

# Operating system
add_definitions(-DYQ2OSTYPE="${CMAKE_SYSTEM_NAME}")

# Architecture string
string(REGEX REPLACE "amd64" "x86_64" YQ2_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
string(REGEX REPLACE "i.86" "i386" YQ2_ARCH "${YQ2_ARCH}")
string(REGEX REPLACE "^arm.*" "arm" YQ2_ARCH "${YQ2_ARCH}")
add_definitions(-DYQ2ARCH="${YQ2_ARCH}")

# Linker Flags
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	list(APPEND RogueLinkerFlags "-lm")
else()
	list(APPEND RogueLinkerFlags "-lm -rdynamic")
endif()

set(Rogue-Source
	src/dm/ball.c
	src/dm/tag.c
	src/monster/berserker/berserker.c
	src/monster/boss2/boss2.c
	src/monster/boss3/boss3.c
	src/monster/boss3/boss31.c
	src/monster/boss3/boss32.c
	src/monster/brain/brain.c
	src/monster/carrier/carrier.c
	src/monster/chick/chick.c
	src/monster/flipper/flipper.c
	src/monster/float/float.c
	src/monster/flyer/flyer.c
	src/monster/gladiator/gladiator.c
	src/monster/gunner/gunner.c
	src/monster/hover/hover.c
	src/monster/infantry/infantry.c
	src/monster/insane/insane.c
	src/monster/medic/medic.c
	src/monster/misc/move.c
	src/monster/mutant/mutant.c
	src/monster/parasite/parasite.c
	src/monster/soldier/soldier.c
	src/monster/stalker/stalker.c
	src/monster/supertank/supertank.c
	src/monster/tank/tank.c
	src/monster/turret/turret.c
	src/monster/widow/widow.c
	src/monster/widow/widow2.c
	src/player/client.c
	src/player/hud.c
	src/player/trail.c
	src/player/view.c
	src/player/weapon.c
	src/savegame/savegame.c
	src/shared/flash.c
	src/shared/rand.c
	src/shared/shared.c
	src/g_ai.c
	src/g_chase.c
	src/g_cmds.c
	src/g_combat.c
	src/g_func.c
	src/g_items.c
	src/g_main.c
	src/g_misc.c
	src/g_monster.c
	src/g_newai.c
	src/g_newdm.c
	src/g_newfnc.c
	src/g_newtarg.c
	src/g_newtrig.c
	src/g_newweap.c
	src/g_phys.c
	src/g_spawn.c
	src/g_sphere.c
	src/g_svcmds.c
	src/g_target.c
	src/g_trigger.c
	src/g_turret.c
	src/g_utils.c
	src/g_weapon.c
	)

set(Rogue-Header
	src/header/game.h
	src/header/local.h
	src/header/shared.h
	src/monster/berserker/berserker.h
	src/monster/boss2/boss2.h
	src/monster/boss3/boss31.h
	src/monster/boss3/boss32.h
	src/monster/brain/brain.h
	src/monster/carrier/carrier.h
	src/monster/chick/chick.h
	src/monster/flipper/flipper.h
	src/monster/float/float.h
	src/monster/flyer/flyer.h
	src/monster/gladiator/gladiator.h
	src/monster/gunner/gunner.h
	src/monster/hover/hover.h
	src/monster/infantry/infantry.h
	src/monster/insane/insane.h
	src/monster/medic/medic.h
	src/monster/misc/player.h
	src/monster/mutant/mutant.h
	src/monster/parasite/parasite.h
	src/monster/soldier/soldier.h
	src/monster/stalker/stalker.h
	src/monster/supertank/supertank.h
	src/monster/tank/tank.h
	src/monster/turret/turret.h
	src/monster/widow/widow.h
	src/monster/widow/widow2.h
	src/savegame/tables/clientfields.h
	src/savegame/tables/fields.h
	src/savegame/tables/gamefunc_decs.h
	src/savegame/tables/gamefunc_list.h
	src/savegame/tables/gamemmove_decs.h
	src/savegame/tables/gamemmove_list.h
	src/savegame/tables/levelfields.h
	)

# Build the rogue dynamic library
add_library(q2rogue SHARED ${Rogue-Source} ${Rogue-Header})
#[[set_target_properties(q2rogue PROPERTIES
	PREFIX ""
	LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug
	LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release
)]]
target_link_libraries(q2rogue ${RogueLinkerFlags})
