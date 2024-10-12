cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR) # 2.6
project(quake3)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 98)

set(IOQ3_VERSION "1.36")

option(USE_CODEC_OPUS "" ON)
option(USE_FREETYPE "" ON)
option(USE_INTERNAL_JPEG "" ON)
option(USE_INTERNAL_VORBIS "" ON)
option(USE_INTERNAL_OGG "" ON)
option(USE_INTERNAL_ZLIB "" OFF)
option(USE_VOIP "" OFF)
option(USE_MUMBLE "" OFF)

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_definitions(-DPRODUCT_VERSION=\"${IOQ3_VERSION}\")
add_definitions(-D_DIII4A) # android
add_definitions(-DNO_VM_COMPILED -DUSE_LOCAL_HEADERS) # base
add_definitions(-DUSE_OPENAL -DUSE_OPENAL_DLOPEN) # OpenAL dlopen
add_definitions(-DUSE_OPENGLES) # OpenGLES 1.1
add_definitions(-DUSE_CODEC_VORBIS) # ogg
add_definitions(-DUSE_CURL -DUSE_CURL_DLOPEN) # curl dlopen
if(USE_CODEC_OPUS)
    add_definitions(-DUSE_CODEC_OPUS -DOPUS_BUILD -DHAVE_LRINTF -DFLOATING_POINT -DFLOAT_APPROX -DUSE_ALLOCA) # opus
endif()
if(USE_VOIP)
add_definitions(-DUSE_VOIP)
endif()

set(q3client_libs android log GLESv1_CM EGL)
set(q3client_libs ${q3client_libs} oboeaudio)
if(USE_FREETYPE)
set(q3client_libs ${q3client_libs} freetype)
endif()
if(NOT USE_INTERNAL_OGG)
set(q3client_libs ${q3client_libs} ogg)
endif()
if(NOT USE_INTERNAL_VORBIS)
set(q3client_libs ${q3client_libs} vorbisfile vorbis)
endif()
if(NOT USE_INTERNAL_JPEG)
    set(q3client_libs ${q3client_libs} jpeg)
else()
    add_definitions(-DUSE_INTERNAL_JPEG) # jpeg
endif()
if(NOT USE_INTERNAL_ZLIB)
    set(q3client_libs ${q3client_libs} z)
else()
    add_definitions(-DNO_GZIP) # zlib
endif()
#set(q3client_libs ${q3client_libs} curl)
#message(FATAL_ERROR "STOP")
# -fshort-enums

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -fsigned-char -fPIC -DNDEBUG -O3 -fno-builtin-sin -fno-builtin-cos -ffast-math -fno-strict-aliasing -Wformat=2 -Wno-format-zero-length -Wformat-security -Wno-format-nonliteral -Wstrict-aliasing=2 -Wmissing-format-attribute -Wdisabled-optimization") # -Werror-implicit-function-declaration -Qunused-arguments
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -fsigned-char -fPIC -DNDEBUG -O3 -fno-builtin-sin -fno-builtin-cos -ffast-math -fno-strict-aliasing -Wformat=2 -Wno-format-zero-length -Wformat-security -Wno-format-nonliteral -Wstrict-aliasing=2 -Wmissing-format-attribute -Wdisabled-optimization") # -Werror-implicit-function-declaration -Qunused-arguments

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

set(q3cgame_src
        code/cgame/cg_main.c
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
        code/cgame/cg_info.c
        code/cgame/cg_localents.c
        code/cgame/cg_marks.c
        code/cgame/cg_particles.c
        code/cgame/cg_players.c
        code/cgame/cg_playerstate.c
        code/cgame/cg_predict.c
        code/cgame/cg_scoreboard.c
        code/cgame/cg_servercmds.c
        code/cgame/cg_snapshot.c
        code/cgame/cg_view.c
        code/cgame/cg_weapons.c
        code/qcommon/q_math.c
        code/qcommon/q_shared.c
        code/cgame/cg_syscalls.c
        )

set(q3mpcgame_src
        code/cgame/cg_main.c
        code/game/bg_misc.c
        code/game/bg_pmove.c
        code/game/bg_slidemove.c
        code/game/bg_lib.c
        code/cgame/cg_consolecmds.c
        code/cgame/cg_newdraw.c
        code/cgame/cg_draw.c
        code/cgame/cg_drawtools.c
        code/cgame/cg_effects.c
        code/cgame/cg_ents.c
        code/cgame/cg_event.c
        code/cgame/cg_info.c
        code/cgame/cg_localents.c
        code/cgame/cg_marks.c
        code/cgame/cg_particles.c
        code/cgame/cg_players.c
        code/cgame/cg_playerstate.c
        code/cgame/cg_predict.c
        code/cgame/cg_scoreboard.c
        code/cgame/cg_servercmds.c
        code/cgame/cg_snapshot.c
        code/cgame/cg_view.c
        code/cgame/cg_weapons.c
        code/ui/ui_shared.c
        code/qcommon/q_math.c
        code/qcommon/q_shared.c
        code/cgame/cg_syscalls.c
)

set(q3game_src
        code/game/g_main.c
        code/game/ai_chat.c
        code/game/ai_cmd.c
        code/game/ai_dmnet.c
        code/game/ai_dmq3.c
        code/game/ai_main.c
        code/game/ai_team.c
        code/game/ai_vcmd.c
        code/game/bg_misc.c
        code/game/bg_pmove.c
        code/game/bg_slidemove.c
        code/game/bg_lib.c
        code/game/g_active.c
        code/game/g_arenas.c
        code/game/g_bot.c
        code/game/g_client.c
        code/game/g_cmds.c
        code/game/g_combat.c
        code/game/g_items.c
        code/game/g_mem.c
        code/game/g_misc.c
        code/game/g_missile.c
        code/game/g_mover.c
        code/game/g_session.c
        code/game/g_spawn.c
        code/game/g_svcmds.c
        code/game/g_target.c
        code/game/g_team.c
        code/game/g_trigger.c
        code/game/g_utils.c
        code/game/g_weapon.c
        code/qcommon/q_math.c
        code/qcommon/q_shared.c
        code/game/g_syscalls.c
        )

set(q3mpgame_src
        code/game/g_main.c
        code/game/ai_chat.c
        code/game/ai_cmd.c
        code/game/ai_dmnet.c
        code/game/ai_dmq3.c
        code/game/ai_main.c
        code/game/ai_team.c
        code/game/ai_vcmd.c
        code/game/bg_misc.c
        code/game/bg_pmove.c
        code/game/bg_slidemove.c
        code/game/bg_lib.c
        code/game/g_active.c
        code/game/g_arenas.c
        code/game/g_bot.c
        code/game/g_client.c
        code/game/g_cmds.c
        code/game/g_combat.c
        code/game/g_items.c
        code/game/g_mem.c
        code/game/g_misc.c
        code/game/g_missile.c
        code/game/g_mover.c
        code/game/g_session.c
        code/game/g_spawn.c
        code/game/g_svcmds.c
        code/game/g_target.c
        code/game/g_team.c
        code/game/g_trigger.c
        code/game/g_utils.c
        code/game/g_weapon.c
        code/qcommon/q_math.c
        code/qcommon/q_shared.c
        code/game/g_syscalls.c
        )

set(q3ui_src
        code/q3_ui/ui_main.c
        code/game/bg_misc.c
        code/game/bg_lib.c
        code/q3_ui/ui_addbots.c
        code/q3_ui/ui_atoms.c
        code/q3_ui/ui_cdkey.c
        code/q3_ui/ui_cinematics.c
        code/q3_ui/ui_confirm.c
        code/q3_ui/ui_connect.c
        code/q3_ui/ui_controls2.c
        code/q3_ui/ui_credits.c
        code/q3_ui/ui_demo2.c
        code/q3_ui/ui_display.c
        code/q3_ui/ui_gameinfo.c
        code/q3_ui/ui_ingame.c
        code/q3_ui/ui_loadconfig.c
        code/q3_ui/ui_menu.c
        code/q3_ui/ui_mfield.c
        code/q3_ui/ui_mods.c
        code/q3_ui/ui_network.c
        code/q3_ui/ui_options.c
        code/q3_ui/ui_playermodel.c
        code/q3_ui/ui_players.c
        code/q3_ui/ui_playersettings.c
        code/q3_ui/ui_preferences.c
        code/q3_ui/ui_qmenu.c
        code/q3_ui/ui_removebots.c
        code/q3_ui/ui_saveconfig.c
        code/q3_ui/ui_serverinfo.c
        code/q3_ui/ui_servers2.c
        code/q3_ui/ui_setup.c
        code/q3_ui/ui_sound.c
        code/q3_ui/ui_sparena.c
        code/q3_ui/ui_specifyserver.c
        code/q3_ui/ui_splevel.c
        code/q3_ui/ui_sppostgame.c
        code/q3_ui/ui_spskill.c
        code/q3_ui/ui_startserver.c
        code/q3_ui/ui_team.c
        code/q3_ui/ui_teamorders.c
        code/q3_ui/ui_video.c
        code/qcommon/q_math.c
        code/qcommon/q_shared.c
        code/ui/ui_syscalls.c
        )

set(q3mpui_src
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

if(USE_INTERNAL_JPEG)
set(q3jpeglib_src
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
endif()

set(q3renderer_src
        code/renderergl1/tr_altivec.c
        code/renderergl1/tr_animation.c
        code/renderergl1/tr_backend.c
        code/renderergl1/tr_bsp.c
        code/renderergl1/tr_cmds.c
        code/renderergl1/tr_curve.c
        code/renderergl1/tr_flares.c
        code/renderercommon/tr_font.c
        code/renderergl1/tr_image.c
        code/renderercommon/tr_image_bmp.c
        code/renderercommon/tr_image_jpg.c
        code/renderercommon/tr_image_pcx.c
        code/renderercommon/tr_image_png.c
        code/renderercommon/tr_image_tga.c
        code/renderergl1/tr_init.c
        code/renderergl1/tr_light.c
        code/renderergl1/tr_main.c
        code/renderergl1/tr_marks.c
        code/renderergl1/tr_mesh.c
        code/renderergl1/tr_model.c
        code/renderergl1/tr_model_iqm.c
        code/renderercommon/tr_noise.c
        code/renderergl1/tr_scene.c
        code/renderergl1/tr_shade.c
        code/renderergl1/tr_shade_calc.c
        code/renderergl1/tr_shader.c
        code/renderergl1/tr_shadows.c
        code/renderergl1/tr_sky.c
        code/renderergl1/tr_surface.c
        code/renderergl1/tr_world.c
        code/android/android_glimp.c
        code/qcommon/q_shared.c
        code/qcommon/puff.c
        code/qcommon/q_math.c
        code/renderergl1/tr_subs.c
        )

set(q3client_src
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
        code/qcommon/unzip.c
        code/qcommon/ioapi.c
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
        code/android/android_input.c
        code/sys/con_log.c
        code/sys/sys_autoupdater.c
        code/android/android_main.c
        code/android/android_snd.c
        code/android/ifaddrs.c
        code/sys/con_passive.c
        code/sys/sys_unix.c
        )

if(USE_CODEC_OPUS)
set(q3_opus_dir "opus-1.2.1")
set(q3_opusfile_dir "opusfile-0.9")
set(q3_opus_src
        code/${q3_opus_dir}/src/opus.c
        code/${q3_opus_dir}/src/opus_decoder.c
        code/${q3_opus_dir}/src/opus_encoder.c
        code/${q3_opus_dir}/src/opus_multistream.c
        code/${q3_opus_dir}/src/opus_multistream_encoder.c
        code/${q3_opus_dir}/src/opus_multistream_decoder.c
        code/${q3_opus_dir}/src/repacketizer.c
        code/${q3_opus_dir}/src/analysis.c
        code/${q3_opus_dir}/src/mlp.c
        code/${q3_opus_dir}/src/mlp_data.c
        code/${q3_opus_dir}/celt/bands.c
        code/${q3_opus_dir}/celt/celt.c
        code/${q3_opus_dir}/celt/celt_encoder.c
        code/${q3_opus_dir}/celt/celt_decoder.c
        code/${q3_opus_dir}/celt/cwrs.c
        code/${q3_opus_dir}/celt/entcode.c
        code/${q3_opus_dir}/celt/entdec.c
        code/${q3_opus_dir}/celt/entenc.c
        code/${q3_opus_dir}/celt/kiss_fft.c
        code/${q3_opus_dir}/celt/laplace.c
        code/${q3_opus_dir}/celt/mathops.c
        code/${q3_opus_dir}/celt/mdct.c
        code/${q3_opus_dir}/celt/modes.c
        code/${q3_opus_dir}/celt/pitch.c
        code/${q3_opus_dir}/celt/celt_lpc.c
        code/${q3_opus_dir}/celt/quant_bands.c
        code/${q3_opus_dir}/celt/rate.c
        code/${q3_opus_dir}/celt/vq.c
        code/${q3_opus_dir}/silk/CNG.c
        code/${q3_opus_dir}/silk/code_signs.c
        code/${q3_opus_dir}/silk/init_decoder.c
        code/${q3_opus_dir}/silk/decode_core.c
        code/${q3_opus_dir}/silk/decode_frame.c
        code/${q3_opus_dir}/silk/decode_parameters.c
        code/${q3_opus_dir}/silk/decode_indices.c
        code/${q3_opus_dir}/silk/decode_pulses.c
        code/${q3_opus_dir}/silk/decoder_set_fs.c
        code/${q3_opus_dir}/silk/dec_API.c
        code/${q3_opus_dir}/silk/enc_API.c
        code/${q3_opus_dir}/silk/encode_indices.c
        code/${q3_opus_dir}/silk/encode_pulses.c
        code/${q3_opus_dir}/silk/gain_quant.c
        code/${q3_opus_dir}/silk/interpolate.c
        code/${q3_opus_dir}/silk/LP_variable_cutoff.c
        code/${q3_opus_dir}/silk/NLSF_decode.c
        code/${q3_opus_dir}/silk/NSQ.c
        code/${q3_opus_dir}/silk/NSQ_del_dec.c
        code/${q3_opus_dir}/silk/PLC.c
        code/${q3_opus_dir}/silk/shell_coder.c
        code/${q3_opus_dir}/silk/tables_gain.c
        code/${q3_opus_dir}/silk/tables_LTP.c
        code/${q3_opus_dir}/silk/tables_NLSF_CB_NB_MB.c
        code/${q3_opus_dir}/silk/tables_NLSF_CB_WB.c
        code/${q3_opus_dir}/silk/tables_other.c
        code/${q3_opus_dir}/silk/tables_pitch_lag.c
        code/${q3_opus_dir}/silk/tables_pulses_per_block.c
        code/${q3_opus_dir}/silk/VAD.c
        code/${q3_opus_dir}/silk/control_audio_bandwidth.c
        code/${q3_opus_dir}/silk/quant_LTP_gains.c
        code/${q3_opus_dir}/silk/VQ_WMat_EC.c
        code/${q3_opus_dir}/silk/HP_variable_cutoff.c
        code/${q3_opus_dir}/silk/NLSF_encode.c
        code/${q3_opus_dir}/silk/NLSF_VQ.c
        code/${q3_opus_dir}/silk/NLSF_unpack.c
        code/${q3_opus_dir}/silk/NLSF_del_dec_quant.c
        code/${q3_opus_dir}/silk/process_NLSFs.c
        code/${q3_opus_dir}/silk/stereo_LR_to_MS.c
        code/${q3_opus_dir}/silk/stereo_MS_to_LR.c
        code/${q3_opus_dir}/silk/check_control_input.c
        code/${q3_opus_dir}/silk/control_SNR.c
        code/${q3_opus_dir}/silk/init_encoder.c
        code/${q3_opus_dir}/silk/control_codec.c
        code/${q3_opus_dir}/silk/A2NLSF.c
        code/${q3_opus_dir}/silk/ana_filt_bank_1.c
        code/${q3_opus_dir}/silk/biquad_alt.c
        code/${q3_opus_dir}/silk/bwexpander_32.c
        code/${q3_opus_dir}/silk/bwexpander.c
        code/${q3_opus_dir}/silk/debug.c
        code/${q3_opus_dir}/silk/decode_pitch.c
        code/${q3_opus_dir}/silk/inner_prod_aligned.c
        code/${q3_opus_dir}/silk/lin2log.c
        code/${q3_opus_dir}/silk/log2lin.c
        code/${q3_opus_dir}/silk/LPC_analysis_filter.c
        code/${q3_opus_dir}/silk/LPC_inv_pred_gain.c
        code/${q3_opus_dir}/silk/table_LSF_cos.c
        code/${q3_opus_dir}/silk/NLSF2A.c
        code/${q3_opus_dir}/silk/NLSF_stabilize.c
        code/${q3_opus_dir}/silk/NLSF_VQ_weights_laroia.c
        code/${q3_opus_dir}/silk/pitch_est_tables.c
        code/${q3_opus_dir}/silk/resampler.c
        code/${q3_opus_dir}/silk/resampler_down2_3.c
        code/${q3_opus_dir}/silk/resampler_down2.c
        code/${q3_opus_dir}/silk/resampler_private_AR2.c
        code/${q3_opus_dir}/silk/resampler_private_down_FIR.c
        code/${q3_opus_dir}/silk/resampler_private_IIR_FIR.c
        code/${q3_opus_dir}/silk/resampler_private_up2_HQ.c
        code/${q3_opus_dir}/silk/resampler_rom.c
        code/${q3_opus_dir}/silk/sigm_Q15.c
        code/${q3_opus_dir}/silk/sort.c
        code/${q3_opus_dir}/silk/sum_sqr_shift.c
        code/${q3_opus_dir}/silk/stereo_decode_pred.c
        code/${q3_opus_dir}/silk/stereo_encode_pred.c
        code/${q3_opus_dir}/silk/stereo_find_predictor.c
        code/${q3_opus_dir}/silk/stereo_quant_pred.c
        code/${q3_opus_dir}/silk/LPC_fit.c
        code/${q3_opus_dir}/silk/float/apply_sine_window_FLP.c
        code/${q3_opus_dir}/silk/float/corrMatrix_FLP.c
        code/${q3_opus_dir}/silk/float/encode_frame_FLP.c
        code/${q3_opus_dir}/silk/float/find_LPC_FLP.c
        code/${q3_opus_dir}/silk/float/find_LTP_FLP.c
        code/${q3_opus_dir}/silk/float/find_pitch_lags_FLP.c
        code/${q3_opus_dir}/silk/float/find_pred_coefs_FLP.c
        code/${q3_opus_dir}/silk/float/LPC_analysis_filter_FLP.c
        code/${q3_opus_dir}/silk/float/LTP_analysis_filter_FLP.c
        code/${q3_opus_dir}/silk/float/LTP_scale_ctrl_FLP.c
        code/${q3_opus_dir}/silk/float/noise_shape_analysis_FLP.c
        code/${q3_opus_dir}/silk/float/process_gains_FLP.c
        code/${q3_opus_dir}/silk/float/regularize_correlations_FLP.c
        code/${q3_opus_dir}/silk/float/residual_energy_FLP.c
        code/${q3_opus_dir}/silk/float/warped_autocorrelation_FLP.c
        code/${q3_opus_dir}/silk/float/wrappers_FLP.c
        code/${q3_opus_dir}/silk/float/autocorrelation_FLP.c
        code/${q3_opus_dir}/silk/float/burg_modified_FLP.c
        code/${q3_opus_dir}/silk/float/bwexpander_FLP.c
        code/${q3_opus_dir}/silk/float/energy_FLP.c
        code/${q3_opus_dir}/silk/float/inner_product_FLP.c
        code/${q3_opus_dir}/silk/float/k2a_FLP.c
        code/${q3_opus_dir}/silk/float/LPC_inv_pred_gain_FLP.c
        code/${q3_opus_dir}/silk/float/pitch_analysis_core_FLP.c
        code/${q3_opus_dir}/silk/float/scale_copy_vector_FLP.c
        code/${q3_opus_dir}/silk/float/scale_vector_FLP.c
        code/${q3_opus_dir}/silk/float/schur_FLP.c
        code/${q3_opus_dir}/silk/float/sort_FLP.c
        code/${q3_opusfile_dir}/src/info.c
        code/${q3_opusfile_dir}/src/internal.c
        code/${q3_opusfile_dir}/src/opusfile.c
        code/${q3_opusfile_dir}/src/stream.c
        code/${q3_opusfile_dir}/src/http.c
        )
endif()

set(q3_ogg_dir "libogg-1.3.3")
if(USE_INTERNAL_OGG)
    set(q3_ogg_src
            code/${q3_ogg_dir}/src/bitwise.c
            code/${q3_ogg_dir}/src/framing.c
            )
endif()

set(q3_vorbis_dir "libvorbis-1.3.6")
if(USE_INTERNAL_VORBIS)
    set(q3_vorbis_src
            code/${q3_vorbis_dir}/lib/analysis.c
            code/${q3_vorbis_dir}/lib/bitrate.c
            code/${q3_vorbis_dir}/lib/block.c
            code/${q3_vorbis_dir}/lib/codebook.c
            code/${q3_vorbis_dir}/lib/envelope.c
            code/${q3_vorbis_dir}/lib/floor0.c
            code/${q3_vorbis_dir}/lib/floor1.c
            code/${q3_vorbis_dir}/lib/info.c
            code/${q3_vorbis_dir}/lib/lookup.c
            code/${q3_vorbis_dir}/lib/lpc.c
            code/${q3_vorbis_dir}/lib/lsp.c
            code/${q3_vorbis_dir}/lib/mapping0.c
            code/${q3_vorbis_dir}/lib/mdct.c
            code/${q3_vorbis_dir}/lib/psy.c
            code/${q3_vorbis_dir}/lib/registry.c
            code/${q3_vorbis_dir}/lib/res0.c
            code/${q3_vorbis_dir}/lib/sharedbook.c
            code/${q3_vorbis_dir}/lib/smallft.c
            code/${q3_vorbis_dir}/lib/synthesis.c
            code/${q3_vorbis_dir}/lib/vorbisenc.c
            code/${q3_vorbis_dir}/lib/vorbisfile.c
            code/${q3_vorbis_dir}/lib/window.c
            )
endif()

set(q3_zlib_dir "zlib")
if(USE_INTERNAL_ZLIB)
    set(q3_zlib_src
            code/${q3_zlib_dir}/adler32.c
            code/${q3_zlib_dir}/crc32.c
            code/${q3_zlib_dir}/inffast.c
            code/${q3_zlib_dir}/inflate.c
            code/${q3_zlib_dir}/inftrees.c
            code/${q3_zlib_dir}/zutil.c
            )
endif()

# struct define conflict with opus
add_library(q3_vorbis STATIC
        ${q3_vorbis_src}
)
set_target_properties(q3_vorbis PROPERTIES COMPILE_FLAGS "-fPIC")
target_include_directories(q3_vorbis PRIVATE "code/${q3_vorbis_dir}/include" "code/${q3_vorbis_dir}/lib")
target_include_directories(q3_vorbis PRIVATE "code/${q3_ogg_dir}/include")

add_library(q3renderer1 STATIC
        ${q3renderer_src}
)
set_target_properties(q3renderer1 PROPERTIES COMPILE_FLAGS "-fPIC")
target_include_directories(q3renderer1 PRIVATE "code/jpeg-8c")
target_include_directories(q3renderer1 PRIVATE "code/curl-7.54.0/include")

set(Q3LIBRARY ioquake3)
add_library(${Q3LIBRARY} SHARED
        ${q3client_src}
        ${q3jpeglib_src}
        ${q3_opus_src}
        ${q3_ogg_src}
        ${q3_zlib_src}
        )
set_target_properties(${Q3LIBRARY} PROPERTIES COMPILE_DEFINITIONS "BOTLIB")
target_link_libraries(${Q3LIBRARY}
        ${q3client_libs}
        q3_vorbis
		q3renderer1
        )
target_include_directories(${Q3LIBRARY} PRIVATE "code/curl-7.54.0/include")
target_include_directories(${Q3LIBRARY} PRIVATE "code/${q3_ogg_dir}/include")
target_include_directories(${Q3LIBRARY} PRIVATE "code/${q3_vorbis_dir}/include")
if(USE_CODEC_OPUS)
    target_include_directories(${Q3LIBRARY} PRIVATE "code/${q3_opus_dir}/include" "code/${q3_opus_dir}/celt" "code/${q3_opus_dir}/silk" "code/${q3_opus_dir}/silk/float" "code/${q3_opusfile_dir}/include")
endif()
target_include_directories(${Q3LIBRARY} PRIVATE "code/jpeg-8c")
target_include_directories(${Q3LIBRARY} PRIVATE "code")
target_include_directories(${Q3LIBRARY} PRIVATE "../common")
if(USE_FREETYPE)
    target_include_directories(${Q3LIBRARY} PRIVATE "../deplibs/freetype/include")
endif()
if(USE_INTERNAL_ZLIB)
    target_include_directories(${Q3LIBRARY} PRIVATE "code/${q3_zlib_dir}")
endif()

# Quake III Arena
add_library(q3ui SHARED
        ${q3ui_src}
        )
set_target_properties(q3ui PROPERTIES COMPILE_DEFINITIONS "UI")
set_target_properties(q3ui PROPERTIES COMPILE_FLAGS "-fPIC")
set_target_properties(q3ui PROPERTIES OUTPUT_NAME "ui")

add_library(q3cgame SHARED
        ${q3cgame_src}
        )
set_target_properties(q3cgame PROPERTIES COMPILE_DEFINITIONS "CGAMEDLL;CGAME")
set_target_properties(q3cgame PROPERTIES COMPILE_FLAGS "-fPIC")
set_target_properties(q3cgame PROPERTIES OUTPUT_NAME "cgame")

add_library(q3qagame SHARED
        ${q3game_src}
        )
set_target_properties(q3qagame PROPERTIES COMPILE_DEFINITIONS "GAMEDLL;QAGAME")
set_target_properties(q3qagame PROPERTIES COMPILE_FLAGS "-fPIC")
set_target_properties(q3qagame PROPERTIES OUTPUT_NAME "qagame")

# MissionPack
add_library(q3mpui SHARED
        ${q3mpui_src}
        )
set_target_properties(q3mpui PROPERTIES COMPILE_DEFINITIONS "UI;MISSIONPACK")
set_target_properties(q3mpui PROPERTIES COMPILE_FLAGS "-fPIC")
set_target_properties(q3mpui PROPERTIES OUTPUT_NAME "ui_mp")

add_library(q3mpcgame SHARED
        ${q3mpcgame_src}
        )
set_target_properties(q3mpcgame PROPERTIES COMPILE_DEFINITIONS "CGAMEDLL;CGAME;MISSIONPACK")
set_target_properties(q3mpcgame PROPERTIES COMPILE_FLAGS "-fPIC")
set_target_properties(q3mpcgame PROPERTIES OUTPUT_NAME "cgame_mp")

add_library(q3mpqagame SHARED
        ${q3mpgame_src}
        )
set_target_properties(q3mpqagame PROPERTIES COMPILE_DEFINITIONS "GAMEDLL;QAGAME;MISSIONPACK")
set_target_properties(q3mpqagame PROPERTIES COMPILE_FLAGS "-fPIC")
set_target_properties(q3mpqagame PROPERTIES OUTPUT_NAME "qagame_mp")