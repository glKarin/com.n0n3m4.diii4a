cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR) # 2.6
project(rtcw)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 98)

add_definitions(-DNO_VM_COMPILED) # base
add_definitions(-DUSE_OPENAL -DUSE_OPENAL_DLOPEN) # OpenAL dlopen
add_definitions(-DUSE_INTERNAL_JPEG) # jpeg
add_definitions(-DUSE_OPENGLES) # OpenGLES 1.1
add_definitions(-DUSE_CODEC_VORBIS) # ogg
add_definitions(-DUSE_CURL -DUSE_CURL_DLOPEN) # curl

set(rtcwclient_libs android log GLESv1_CM EGL z)
set(rtcwclient_libs ${rtcwclient_libs} vorbisfile vorbis ogg)
set(rtcwclient_libs ${rtcwclient_libs} oboeaudio)
#set(rtcwclient_libs ${rtcwclient_libs} curl)
#message(FATAL_ERROR "STOP")
# -fshort-enums

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -fsigned-char -fPIC -DNDEBUG -O3 -fno-builtin-sin -fno-builtin-cos -ffast-math -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -fsigned-char -fPIC -DNDEBUG -O3 -fno-builtin-sin -fno-builtin-cos -ffast-math -fno-strict-aliasing")

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
add_definitions(-DARCH_STRING="${ARCH}")
add_definitions(-D_SIZEOFPTR=${CMAKE_SIZEOF_VOID_P})

set(rtcwcgame_src
        code/cgame/cg_main.c
        code/game/bg_animation.c
        code/game/bg_misc.c
        code/game/bg_pmove.c
        code/game/bg_slidemove.c
        code/game/bg_lib.c
        code/cgame/cg_consolecmds.c
        code/cgame/cg_draw.c
        code/cgame/cg_drawtools.c
        code/cgame/cg_effects.c
        code/cgame/cg_ents.c
        code/cgame/cg_event.c
        code/cgame/cg_flamethrower.c
        code/cgame/cg_info.c
        code/cgame/cg_localents.c
        code/cgame/cg_marks.c
        code/cgame/cg_newdraw.c
        code/cgame/cg_particles.c
        code/cgame/cg_players.c
        code/cgame/cg_playerstate.c
        code/cgame/cg_predict.c
        code/cgame/cg_scoreboard.c
        code/cgame/cg_servercmds.c
        code/cgame/cg_snapshot.c
        code/cgame/cg_sound.c
        code/cgame/cg_trails.c
        code/cgame/cg_view.c
        code/cgame/cg_weapons.c
        code/ui/ui_shared.c
        code/qcommon/q_math.c
        code/qcommon/q_shared.c
        code/cgame/cg_syscalls.c
        )

set(rtcwgame_src
        code/game/g_main.c
        code/game/ai_cast.c
        code/game/ai_cast_characters.c
        code/game/ai_cast_debug.c
        code/game/ai_cast_events.c
        code/game/ai_cast_fight.c
        code/game/ai_cast_func_attack.c
        code/game/ai_cast_func_boss1.c
        code/game/ai_cast_funcs.c
        code/game/ai_cast_script_actions.c
        code/game/ai_cast_script.c
        code/game/ai_cast_script_ents.c
        code/game/ai_cast_sight.c
        code/game/ai_cast_think.c
        code/game/ai_chat.c
        code/game/ai_cmd.c
        code/game/ai_dmnet.c
        code/game/ai_dmq3.c
        code/game/ai_main.c
        code/game/ai_team.c
        code/game/bg_animation.c
        code/game/bg_misc.c
        code/game/bg_pmove.c
        code/game/bg_slidemove.c
        code/game/bg_lib.c
        code/game/g_active.c
        code/game/g_alarm.c
        code/game/g_bot.c
        code/game/g_client.c
        code/game/g_cmds.c
        code/game/g_combat.c
        code/game/g_items.c
        code/game/g_mem.c
        code/game/g_misc.c
        code/game/g_missile.c
        code/game/g_mover.c
        code/game/g_props.c
        code/game/g_save.c
        code/game/g_script_actions.c
        code/game/g_script.c
        code/game/g_session.c
        code/game/g_spawn.c
        code/game/g_svcmds.c
        code/game/g_target.c
        code/game/g_team.c
        code/game/g_tramcar.c
        code/game/g_trigger.c
        code/game/g_utils.c
        code/game/g_weapon.c
        code/qcommon/q_math.c
        code/qcommon/q_shared.c
        code/game/g_syscalls.c
        )

set(rtcwui_src
        code/ui/ui_main.c
        code/ui/ui_atoms.c
        code/ui/ui_gameinfo.c
        code/ui/ui_players.c
        code/ui/ui_shared.c
        code/game/bg_misc.c
        code/game/bg_lib.c
        code/qcommon/q_math.c
        code/qcommon/q_shared.c
        code/ui/ui_syscalls.c
        )

set(rtcwjpeglib_src
        code/jpeg-8c/jaricom.c
        code/jpeg-8c/jcapimin.c
        code/jpeg-8c/jcapistd.c
        code/jpeg-8c/jcarith.c
        code/jpeg-8c/jccoefct.c
        code/jpeg-8c/jccolor.c
        code/jpeg-8c/jcdctmgr.c
        code/jpeg-8c/jchuff.c
        code/jpeg-8c/jcinit.c
        code/jpeg-8c/jcmainct.c
        code/jpeg-8c/jcmarker.c
        code/jpeg-8c/jcmaster.c
        code/jpeg-8c/jcomapi.c
        code/jpeg-8c/jcparam.c
        code/jpeg-8c/jcprepct.c
        code/jpeg-8c/jcsample.c
        code/jpeg-8c/jctrans.c
        code/jpeg-8c/jdapimin.c
        code/jpeg-8c/jdapistd.c
        code/jpeg-8c/jdarith.c
        code/jpeg-8c/jdatadst.c
        code/jpeg-8c/jdatasrc.c
        code/jpeg-8c/jdcoefct.c
        code/jpeg-8c/jdcolor.c
        code/jpeg-8c/jddctmgr.c
        code/jpeg-8c/jdhuff.c
        code/jpeg-8c/jdinput.c
        code/jpeg-8c/jdmainct.c
        code/jpeg-8c/jdmarker.c
        code/jpeg-8c/jdmaster.c
        code/jpeg-8c/jdmerge.c
        code/jpeg-8c/jdpostct.c
        code/jpeg-8c/jdsample.c
        code/jpeg-8c/jdtrans.c
        code/jpeg-8c/jerror.c
        code/jpeg-8c/jfdctflt.c
        code/jpeg-8c/jfdctfst.c
        code/jpeg-8c/jfdctint.c
        code/jpeg-8c/jidctflt.c
        code/jpeg-8c/jidctfst.c
        code/jpeg-8c/jidctint.c
        code/jpeg-8c/jmemmgr.c
        code/jpeg-8c/jmemnobs.c
        code/jpeg-8c/jquant1.c
        code/jpeg-8c/jquant2.c
        code/jpeg-8c/jutils.c
        )

set(rtcwbotlib_src
        code/src/botlib/be_aas_bspq3.c
        code/src/botlib/be_aas_cluster.c
        code/src/botlib/be_aas_debug.c
        code/src/botlib/be_aas_entity.c
        code/src/botlib/be_aas_file.c
        code/src/botlib/be_aas_main.c
        code/src/botlib/be_aas_move.c
        code/src/botlib/be_aas_optimize.c
        code/src/botlib/be_aas_reach.c
        code/src/botlib/be_aas_route.c
        code/src/botlib/be_aas_routealt.c
        code/src/botlib/be_aas_routetable.c
        code/src/botlib/be_aas_sample.c
        code/src/botlib/be_ai_char.c
        code/src/botlib/be_ai_chat.c
        code/src/botlib/be_ai_gen.c
        code/src/botlib/be_ai_goal.c
        code/src/botlib/be_ai_move.c
        code/src/botlib/be_ai_weap.c
        code/src/botlib/be_ai_weight.c
        code/src/botlib/be_ea.c
        code/src/botlib/be_interface.c
        code/src/botlib/l_crc.c
        code/src/botlib/l_libvar.c
        code/src/botlib/l_log.c
        code/src/botlib/l_memory.c
        code/src/botlib/l_precomp.c
        code/src/botlib/l_script.c
        code/src/botlib/l_struct.c
)

set(rtcwrenderer_src
        code/renderer/tr_altivec.c
        code/renderer/tr_animation.c
        code/renderer/tr_backend.c
        code/renderer/tr_bsp.c
        code/renderer/tr_cmds.c
        code/renderer/tr_cmesh.c
        code/renderer/tr_curve.c
        code/renderer/tr_flares.c
        code/renderer/tr_font.c
        code/renderer/tr_image.c
        code/renderer/tr_image_bmp.c
        code/renderer/tr_image_jpg.c
        code/renderer/tr_image_pcx.c
        code/renderer/tr_image_png.c
        code/renderer/tr_image_tga.c
        code/renderer/tr_init.c
        code/renderer/tr_light.c
        code/renderer/tr_main.c
        code/renderer/tr_marks.c
        code/renderer/tr_mesh.c
        code/renderer/tr_model.c
        code/renderer/tr_model_iqm.c
        code/renderer/tr_noise.c
        code/renderer/tr_scene.c
        code/renderer/tr_shade.c
        code/renderer/tr_shade_calc.c
        code/renderer/tr_shader.c
        code/renderer/tr_shadows.c
        code/renderer/tr_sky.c
        code/renderer/tr_surface.c
        code/renderer/tr_world.c
        )

set(rtcwclient_src
        code/client/cl_cgame.c
        code/client/cl_cin.c
        code/client/cl_console.c
        code/client/cl_input.c
        code/client/cl_keys.c
        code/client/cl_main.c
        code/client/cl_net_chan.c
        code/client/cl_parse.c
        code/client/cl_scrn.c
        code/client/cl_ui.c
        code/client/cl_avi.c
        code/qcommon/cm_load.c
        code/qcommon/cm_patch.c
        code/qcommon/cm_polylib.c
        code/qcommon/cm_test.c
        code/qcommon/cm_trace.c
        code/qcommon/cmd.c
        code/qcommon/common.c
        code/qcommon/cvar.c
        code/qcommon/files.c
        code/qcommon/md4.c
        code/qcommon/md5.c
        code/qcommon/msg.c
        code/qcommon/net_chan.c
        code/qcommon/net_ip.c
        code/qcommon/huffman.c
        code/client/snd_altivec.c
        code/client/snd_adpcm.c
        code/client/snd_dma.c
        code/client/snd_mem.c
        code/client/snd_mix.c
        code/client/snd_wavelet.c
        code/client/snd_main.c
        code/client/snd_codec.c
        code/client/snd_codec_wav.c
        code/client/snd_codec_ogg.c
        code/client/snd_codec_opus.c
        code/client/qal.c
        code/client/snd_openal.c
        code/client/cl_curl.c
        code/server/sv_bot.c
        code/server/sv_ccmds.c
        code/server/sv_client.c
        code/server/sv_game.c
        code/server/sv_init.c
        code/server/sv_main.c
        code/server/sv_net_chan.c
        code/server/sv_snapshot.c
        code/server/sv_world.c
        code/qcommon/q_math.c
        code/qcommon/q_shared.c
        code/zlib-1.2.11/unzip.c
        code/zlib-1.2.11/ioapi.c
        code/qcommon/puff.c
        code/qcommon/vm.c
        code/qcommon/vm_interpreted.c
        code/botlib/be_aas_bspq3.c
        code/botlib/be_aas_cluster.c
        code/botlib/be_aas_debug.c
        code/botlib/be_aas_entity.c
        code/botlib/be_aas_file.c
        code/botlib/be_aas_main.c
        code/botlib/be_aas_move.c
        code/botlib/be_aas_optimize.c
        code/botlib/be_aas_reach.c
        code/botlib/be_aas_route.c
        code/botlib/be_aas_routealt.c
        code/botlib/be_aas_routetable.c
        code/botlib/be_aas_sample.c
        code/botlib/be_ai_char.c
        code/botlib/be_ai_chat.c
        code/botlib/be_ai_gen.c
        code/botlib/be_ai_goal.c
        code/botlib/be_ai_move.c
        code/botlib/be_ai_weap.c
        code/botlib/be_ai_weight.c
        code/botlib/be_ea.c
        code/botlib/be_interface.c
        code/botlib/l_crc.c
        code/botlib/l_libvar.c
        code/botlib/l_log.c
        code/botlib/l_memory.c
        code/botlib/l_precomp.c
        code/botlib/l_script.c
        code/botlib/l_struct.c
        code/splines/math_angles.cpp
        code/splines/math_matrix.cpp
        code/splines/math_quaternion.cpp
        code/splines/math_vector.cpp
        code/splines/q_parse.cpp
        code/splines/splines.cpp
        code/splines/util_str.cpp
        code/android/android_input.c
        code/android/android_glimp.c
        code/android/android_snd.c
        code/android/android_main.c
        code/android/ifaddrs.c
        code/sys/sys_unix.c
        code/sys/con_log.c
        code/sys/con_passive.c
        )

set(RTCWLIBRARY idtech3_rtcw)
add_library(${RTCWLIBRARY} SHARED
        ${rtcwrenderer_src}
        ${rtcwclient_src}
        ${rtcwjpeglib_src}
        )
set_target_properties(${RTCWLIBRARY} PROPERTIES COMPILE_DEFINITIONS "BOTLIB")
target_link_libraries(${RTCWLIBRARY}
        ${rtcwclient_libs}
        )
target_include_directories(${RTCWLIBRARY} PRIVATE "code/curl-7.60.0/include")
target_include_directories(${RTCWLIBRARY} PRIVATE "code/libogg-1.3.3/include")
target_include_directories(${RTCWLIBRARY} PRIVATE "code/libvorbis-1.3.6/include")
target_include_directories(${RTCWLIBRARY} PRIVATE "code/jpeg-8c")
target_include_directories(${RTCWLIBRARY} PRIVATE "code")
target_include_directories(${RTCWLIBRARY} PRIVATE "../../common")

add_library(rtcwui SHARED
        ${rtcwui_src}
        )
set_target_properties(rtcwui PROPERTIES COMPILE_DEFINITIONS "UI")
set_target_properties(rtcwui PROPERTIES COMPILE_FLAGS "-fPIC")

add_library(rtcwcgame SHARED
        ${rtcwcgame_src}
        )
set_target_properties(rtcwcgame PROPERTIES COMPILE_DEFINITIONS "CGAMEDLL;CGAME")
set_target_properties(rtcwcgame PROPERTIES COMPILE_FLAGS "-fPIC")

add_library(rtcwqagame SHARED
        ${rtcwgame_src}
        )
set_target_properties(rtcwqagame PROPERTIES COMPILE_DEFINITIONS "GAMEDLL;QAGAME")
set_target_properties(rtcwqagame PROPERTIES COMPILE_FLAGS "-fPIC")