cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR) # 2.6
project(q2zaero C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fno-strict-aliasing -fwrapv -fvisibility=hidden")
string(REPLACE "-O3" "-O2" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

# Switch off some annoying warnings
if (${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-braces")
elseif (${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
    if (CMAKE_C_COMPILER_VERSION GREATER 7.99)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-format-truncation -Wno-format-overflow")
    endif()
endif()

set(cpu ${CMAKE_SYSTEM_PROCESSOR})
if(cpu STREQUAL "powerpc")
    set(cpu "ppc")
elseif(cpu STREQUAL "aarch64")
    # "arm64" is more obvious, and some operating systems (like macOS) use it instead of "aarch64"
    set(cpu "arm64")
elseif(cpu MATCHES "[aA][mM][dD]64" OR cpu MATCHES "[xX].*64")
    set(cpu "x86_64")
elseif(cpu MATCHES "i.86" OR cpu MATCHES "[xX]86")
    set(cpu "i386")
elseif(cpu MATCHES "[aA][rR][mM].*") # some kind of arm..
    # On 32bit Raspbian gcc -dumpmachine returns sth starting with "arm-",
    # while clang -dumpmachine says "arm6k-..." - try to unify that to "arm"
    if(CMAKE_SIZEOF_VOID_P EQUAL 8) # sizeof(void*) == 8 => must be arm64
        set(cpu "arm64")
    else() # should be 32bit arm then (probably "armv7l" "armv6k" or sth like that)
        set(cpu "arm")
    endif()
endif()
set(ARCH "${cpu}")
add_definitions(-DYQ2ARCH="${ARCH}")

set(q2zaero_src
        src/g_ai.c
        src/g_cmds.c
        src/g_combat.c
        src/g_func.c
        src/g_items.c
        src/g_main.c
        src/g_misc.c
        src/g_monster.c
        src/g_phys.c
        src/g_spawn.c
        src/g_svcmds.c
        src/g_target.c
        src/g_trigger.c
        src/g_turret.c
        src/g_utils.c
        src/g_weapon.c
        src/monster/actor/actor.c
        src/monster/berserker/berserker.c
        src/monster/boss/boss.c
        src/monster/boss2/boss2.c
        src/monster/boss3/boss3.c
        src/monster/boss3/boss31.c
        src/monster/boss3/boss32.c
        src/monster/brain/brain.c
        src/monster/chick/chick.c
        src/monster/flipper/flipper.c
        src/monster/float/float.c
        src/monster/flyer/flyer.c
        src/monster/gladiator/gladiator.c
        src/monster/gunner/gunner.c
        src/monster/handler/handler.c
        src/monster/hound/hound.c
        src/monster/hover/hover.c
        src/monster/infantry/infantry.c
        src/monster/insane/insane.c
        src/monster/medic/medic.c
        src/monster/misc/move.c
        src/monster/mutant/mutant.c
        src/monster/parasite/parasite.c
        src/monster/sentien/sentien.c
        src/monster/soldier/soldier.c
        src/monster/supertank/supertank.c
        src/monster/tank/tank.c
        src/player/client.c
        src/player/hud.c
        src/player/trail.c
        src/player/view.c
        src/player/weapon.c
        src/savegame/savegame.c
        src/shared/flash.c
        src/shared/shared.c
        src/zaero/acannon.c
        src/zaero/ai.c
        src/zaero/anim.c
        src/zaero/camera.c
        src/zaero/frames.c
        src/zaero/item.c
        src/zaero/mtest.c
        src/zaero/spawn.c
        src/zaero/trigger.c
        src/zaero/weapon.c
        )

add_library(q2zaero SHARED
        ${q2zaero_src}
        )
