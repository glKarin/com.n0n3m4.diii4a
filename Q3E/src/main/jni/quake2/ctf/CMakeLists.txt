cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR) # 2.6
project(q2ctf C)

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

set(q2ctf_src
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
        src/menu/menu.c
        src/monster/move.c
        src/player/client.c
        src/player/hud.c
        src/player/trail.c
        src/player/view.c
        src/player/weapon.c
        src/shared/shared.c
)

add_library(q2ctf SHARED
        ${q2ctf_src}
        )
