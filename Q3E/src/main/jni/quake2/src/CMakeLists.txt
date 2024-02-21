cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR) # 2.6
project(quake2 C)

set(CMAKE_C_STANDARD 99)

OPTION(WITH_OGG "# Enables OGG/Vorbis support. OGG/Vorbis files can be used as a substitute of CD audio playback. Adds dependencies to libogg, libvorbis and libvorbisfile." ON)
OPTION(WITH_OPENAL "Enables the optional OpenAL sound system." OFF)
OPTION(WITH_ZIP "Enables opening of ZIP files (also known as .pk3 paks)." ON)
OPTION(WITH_RETEXTURING "Enables retexturing support. Adds a dependency to libjpeg." ON)

set(client_libs android)
set(opengl_libs EGL GLESv1_CM android)
if(WITH_ZIP)
    message(STATUS "quake2 libz")
    add_definitions(-DZIP -DNOUNCRYPT)
    set(client_libs ${client_libs} z)
endif()
if(WITH_OPENAL)
    message(STATUS "quake2 OpenAL")
    add_definitions(-DUSE_OPENAL -DDEFAULT_OPENAL_DRIVER='"libopenal.so"')
endif()
if(WITH_OGG)
    message(STATUS "quake2 ogg/vorbis")
    add_definitions(-DOGG)
    set(client_libs ${client_libs} vorbisfile vorbis ogg)
endif()
if(WITH_RETEXTURING)
    message(STATUS "quake2 jpeg")
    add_definitions(-DRETEXTURE)
    include_directories("../../deplibs/libjpeg/libjpeg")
    set(opengl_libs ${opengl_libs} jpeg)
endif()
#message(FATAL_ERROR "STOP")

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

set(game_src
        common/shared/flash.c
        common/shared/rand.c
        common/shared/shared.c
        game/g_ai.c
        game/g_chase.c
        game/g_cmds.c
        game/g_combat.c
        game/g_func.c
        game/g_items.c
        game/g_main.c
        game/g_misc.c
        game/g_monster.c
        game/g_phys.c
        game/g_spawn.c
        game/g_svcmds.c
        game/g_target.c
        game/g_trigger.c
        game/g_turret.c
        game/g_utils.c
        game/g_weapon.c
        game/monster/berserker/berserker.c
        game/monster/boss2/boss2.c
        game/monster/boss3/boss3.c
        game/monster/boss3/boss31.c
        game/monster/boss3/boss32.c
        game/monster/brain/brain.c
        game/monster/chick/chick.c
        game/monster/flipper/flipper.c
        game/monster/float/float.c
        game/monster/flyer/flyer.c
        game/monster/gladiator/gladiator.c
        game/monster/gunner/gunner.c
        game/monster/hover/hover.c
        game/monster/infantry/infantry.c
        game/monster/insane/insane.c
        game/monster/medic/medic.c
        game/monster/misc/move.c
        game/monster/mutant/mutant.c
        game/monster/parasite/parasite.c
        game/monster/soldier/soldier.c
        game/monster/supertank/supertank.c
        game/monster/tank/tank.c
        game/player/client.c
        game/player/hud.c
        game/player/trail.c
        game/player/view.c
        game/player/weapon.c
        game/savegame/savegame.c
        )

set(client_src
        backends/generic/qal.c
        backends/generic/vid.c
        backends/sdl/cd.c
        backends/sdl/sound.c
        client/cl_cin.c
        client/cl_console.c
        client/cl_download.c
        client/cl_effects.c
        client/cl_entities.c
        client/cl_input.c
        client/cl_inventory.c
        client/cl_keyboard.c
        client/cl_lights.c
        client/cl_main.c
        client/cl_network.c
        client/cl_parse.c
        client/cl_particles.c
        client/cl_prediction.c
        client/cl_screen.c
        client/cl_tempentities.c
        client/cl_view.c
        client/menu/menu.c
        client/menu/qmenu.c
        client/menu/videomenu.c
        client/sound/snd_al.c
        client/sound/snd_dma.c
        client/sound/snd_mem.c
        client/sound/snd_mix.c
        client/sound/snd_vorbis.c
        client/sound/snd_wav.c
        common/argproc.c
        common/clientserver.c
        common/cmdparser.c
        common/collision.c
        common/crc.c
        common/cvar.c
        common/filesystem.c
        common/glob.c
        common/md4.c
        common/misc.c
        common/movemsg.c
        common/netchan.c
        common/pmove.c
        common/szone.c
        common/zone.c
        common/shared/flash.c
        common/shared/rand.c
        common/shared/shared.c
        common/unzip/ioapi.c
        common/unzip/unzip.c
        server/sv_cmd.c
        server/sv_conless.c
        server/sv_entities.c
        server/sv_game.c
        server/sv_init.c
        server/sv_main.c
        server/sv_save.c
        server/sv_send.c
        server/sv_user.c
        server/sv_world.c
        )

if(WIN32)
    set(win_client_src
            backends/windows/shared/mem.c
            backends/windows/network.c
            backends/windows/system.c
            )
    list(APPEND client_src ${win_client_src})
else()
    set(unix_client_src
            backends/unix/main.c
            backends/unix/network.c
            backends/unix/signalhandler.c
            backends/unix/system.c
            backends/unix/shared/hunk.c
            )
    list(APPEND client_src ${unix_client_src})
endif()

set(server_src
        common/argproc.c
        common/clientserver.c
        common/cmdparser.c
        common/collision.c
        common/crc.c
        common/cvar.c
        common/filesystem.c
        common/glob.c
        common/md4.c
        common/misc.c
        common/movemsg.c
        common/netchan.c
        common/pmove.c
        common/szone.c
        common/zone.c
        common/shared/rand.c
        common/shared/shared.c
        common/unzip/ioapi.c
        common/unzip/unzip.c
        server/sv_cmd.c
        server/sv_conless.c
        server/sv_entities.c
        server/sv_game.c
        server/sv_init.c
        server/sv_main.c
        server/sv_save.c
        server/sv_send.c
        server/sv_user.c
        server/sv_world.c
        )

if(WIN32)
    set(win_server_src
            backends/windows/shared/mem.c
            backends/windows/network.c
            backends/windows/system.c
            )
    list(APPEND server_src ${win_server_src})
else()
    set(unix_server_src
            backends/unix/main.c
            backends/unix/network.c
            backends/unix/signalhandler.c
            backends/unix/system.c
            backends/unix/shared/hunk.c
            )
    list(APPEND server_src ${unix_server_src})
endif()

set(opengl_src
        backends/generic/qgl.c
        backends/sdl/input.c
        backends/sdl/refresh.c
        refresh/r_draw.c
        refresh/r_image.c
        refresh/r_light.c
        refresh/r_lightmap.c
        refresh/r_main.c
        refresh/r_mesh.c
        refresh/r_misc.c
        refresh/r_model.c
        refresh/r_scrap.c
        refresh/r_surf.c
        refresh/r_warp.c
        refresh/files/md2.c
        refresh/files/pcx.c
        refresh/files/sp2.c
        refresh/files/tga.c
        refresh/files/jpeg.c
        refresh/files/wal.c
        common/shared/shared.c
        )

if(WIN32)
    set(win_opengl_src
            backends/windows/shared/mem.c
            )
    list(APPEND opengl_src ${win_opengl_src})
else()
    set(unix_opengl_src
            backends/unix/shared/hunk.c
            )
    list(APPEND opengl_src ${unix_opengl_src})
endif()

set(QUAKE2LIBRARY idtech2)
add_library(${QUAKE2LIBRARY} SHARED
        ${client_src}
        )
target_link_libraries(${QUAKE2LIBRARY}
        ${client_libs}
        )

#add_library(idtech2_ded SHARED ${server_src})

add_library(ref_gl SHARED
        ${opengl_src}
        )
target_link_libraries(ref_gl
        ${opengl_libs}
        )

add_library(q2game SHARED
        ${game_src}
        )
